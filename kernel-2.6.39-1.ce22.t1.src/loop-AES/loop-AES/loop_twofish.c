/*
 *  Copyright 2002 by Free Software Foundation
 *  Redistribution of this file is permitted under the GNU Public License.
 *
 *  IV is now passed as (512 byte) sector number by default.
 *  Jari Ruusu, March 5 2002
 *
 *  Added support for MD5 IV computation and multi-key operation.
 *  Jari Ruusu, October 22 2003
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/string.h> 
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/slab.h>
#if LINUX_VERSION_CODE >= 0x20600
# include <linux/bio.h>
# include <linux/blkdev.h>
#endif
#include <linux/loop.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>

#define ROL(x,c) (((x) << (c)) | ((x) >> (32-(c))))
#define ROR(x,c) (((x) >> (c)) | ((x) << (32-(c))))
#define Bswap(x) __le32_to_cpu(x)

#define DWORD __u32
#define BYTE unsigned char

typedef struct fish2_key
{ int keyLen;       /* Key Length in Bit */
  DWORD sboxKeys[4];
  DWORD subKeys[40];
  BYTE  key[32];
  DWORD sbox_full[1024];  /* This have to be 1024 DWORDs */
} fish2_key;


/* Mul_5B[i] is  0x5B * i   in GF(256), whatever that means... */

static const unsigned char Mul_5B[256] = {
    0x00,0x5B,0xB6,0xED,0x05,0x5E,0xB3,0xE8,
    0x0A,0x51,0xBC,0xE7,0x0F,0x54,0xB9,0xE2,
    0x14,0x4F,0xA2,0xF9,0x11,0x4A,0xA7,0xFC,
    0x1E,0x45,0xA8,0xF3,0x1B,0x40,0xAD,0xF6,
    0x28,0x73,0x9E,0xC5,0x2D,0x76,0x9B,0xC0,
    0x22,0x79,0x94,0xCF,0x27,0x7C,0x91,0xCA,
    0x3C,0x67,0x8A,0xD1,0x39,0x62,0x8F,0xD4,
    0x36,0x6D,0x80,0xDB,0x33,0x68,0x85,0xDE,
    0x50,0x0B,0xE6,0xBD,0x55,0x0E,0xE3,0xB8,
    0x5A,0x01,0xEC,0xB7,0x5F,0x04,0xE9,0xB2,
    0x44,0x1F,0xF2,0xA9,0x41,0x1A,0xF7,0xAC,
    0x4E,0x15,0xF8,0xA3,0x4B,0x10,0xFD,0xA6,
    0x78,0x23,0xCE,0x95,0x7D,0x26,0xCB,0x90,
    0x72,0x29,0xC4,0x9F,0x77,0x2C,0xC1,0x9A,
    0x6C,0x37,0xDA,0x81,0x69,0x32,0xDF,0x84,
    0x66,0x3D,0xD0,0x8B,0x63,0x38,0xD5,0x8E,
    0xA0,0xFB,0x16,0x4D,0xA5,0xFE,0x13,0x48,
    0xAA,0xF1,0x1C,0x47,0xAF,0xF4,0x19,0x42,
    0xB4,0xEF,0x02,0x59,0xB1,0xEA,0x07,0x5C,
    0xBE,0xE5,0x08,0x53,0xBB,0xE0,0x0D,0x56,
    0x88,0xD3,0x3E,0x65,0x8D,0xD6,0x3B,0x60,
    0x82,0xD9,0x34,0x6F,0x87,0xDC,0x31,0x6A,
    0x9C,0xC7,0x2A,0x71,0x99,0xC2,0x2F,0x74,
    0x96,0xCD,0x20,0x7B,0x93,0xC8,0x25,0x7E,
    0xF0,0xAB,0x46,0x1D,0xF5,0xAE,0x43,0x18,
    0xFA,0xA1,0x4C,0x17,0xFF,0xA4,0x49,0x12,
    0xE4,0xBF,0x52,0x09,0xE1,0xBA,0x57,0x0C,
    0xEE,0xB5,0x58,0x03,0xEB,0xB0,0x5D,0x06,
    0xD8,0x83,0x6E,0x35,0xDD,0x86,0x6B,0x30,
    0xD2,0x89,0x64,0x3F,0xD7,0x8C,0x61,0x3A,
    0xCC,0x97,0x7A,0x21,0xC9,0x92,0x7F,0x24,
    0xC6,0x9D,0x70,0x2B,0xC3,0x98,0x75,0x2E };


/* Mul_EF[i] is  0xEF * i   in GF(256), whatever that means... */

static const unsigned char Mul_EF[256] = {
    0x00,0xEF,0xB7,0x58,0x07,0xE8,0xB0,0x5F,
    0x0E,0xE1,0xB9,0x56,0x09,0xE6,0xBE,0x51,
    0x1C,0xF3,0xAB,0x44,0x1B,0xF4,0xAC,0x43,
    0x12,0xFD,0xA5,0x4A,0x15,0xFA,0xA2,0x4D,
    0x38,0xD7,0x8F,0x60,0x3F,0xD0,0x88,0x67,
    0x36,0xD9,0x81,0x6E,0x31,0xDE,0x86,0x69,
    0x24,0xCB,0x93,0x7C,0x23,0xCC,0x94,0x7B,
    0x2A,0xC5,0x9D,0x72,0x2D,0xC2,0x9A,0x75,
    0x70,0x9F,0xC7,0x28,0x77,0x98,0xC0,0x2F,
    0x7E,0x91,0xC9,0x26,0x79,0x96,0xCE,0x21,
    0x6C,0x83,0xDB,0x34,0x6B,0x84,0xDC,0x33,
    0x62,0x8D,0xD5,0x3A,0x65,0x8A,0xD2,0x3D,
    0x48,0xA7,0xFF,0x10,0x4F,0xA0,0xF8,0x17,
    0x46,0xA9,0xF1,0x1E,0x41,0xAE,0xF6,0x19,
    0x54,0xBB,0xE3,0x0C,0x53,0xBC,0xE4,0x0B,
    0x5A,0xB5,0xED,0x02,0x5D,0xB2,0xEA,0x05,
    0xE0,0x0F,0x57,0xB8,0xE7,0x08,0x50,0xBF,
    0xEE,0x01,0x59,0xB6,0xE9,0x06,0x5E,0xB1,
    0xFC,0x13,0x4B,0xA4,0xFB,0x14,0x4C,0xA3,
    0xF2,0x1D,0x45,0xAA,0xF5,0x1A,0x42,0xAD,
    0xD8,0x37,0x6F,0x80,0xDF,0x30,0x68,0x87,
    0xD6,0x39,0x61,0x8E,0xD1,0x3E,0x66,0x89,
    0xC4,0x2B,0x73,0x9C,0xC3,0x2C,0x74,0x9B,
    0xCA,0x25,0x7D,0x92,0xCD,0x22,0x7A,0x95,
    0x90,0x7F,0x27,0xC8,0x97,0x78,0x20,0xCF,
    0x9E,0x71,0x29,0xC6,0x99,0x76,0x2E,0xC1,
    0x8C,0x63,0x3B,0xD4,0x8B,0x64,0x3C,0xD3,
    0x82,0x6D,0x35,0xDA,0x85,0x6A,0x32,0xDD,
    0xA8,0x47,0x1F,0xF0,0xAF,0x40,0x18,0xF7,
    0xA6,0x49,0x11,0xFE,0xA1,0x4E,0x16,0xF9,
    0xB4,0x5B,0x03,0xEC,0xB3,0x5C,0x04,0xEB,
    0xBA,0x55,0x0D,0xE2,0xBD,0x52,0x0A,0xE5 };

static inline DWORD mds_mul(BYTE *y)
{ DWORD z;

  z=Mul_EF[y[0]] ^ y[1] ^ Mul_EF[y[2]] ^ Mul_5B[y[3]];
  z<<=8;
  z|=Mul_EF[y[0]] ^ Mul_5B[y[1]] ^ y[2] ^ Mul_EF[y[3]];
  z<<=8;
  z|=Mul_5B[y[0]] ^ Mul_EF[y[1]] ^ Mul_EF[y[2]] ^ y[3];
  z<<=8;
  z|=y[0] ^ Mul_EF[y[1]] ^ Mul_5B[y[2]] ^ Mul_5B[y[3]];
  
  return z;
}  

/* q0 and q1 are the lookup substitutions done in twofish */

static const unsigned char q0[256] =
{	0xA9, 0x67, 0xB3, 0xE8, 0x04, 0xFD, 0xA3, 0x76, 
	0x9A, 0x92, 0x80, 0x78, 0xE4, 0xDD, 0xD1, 0x38, 
	0x0D, 0xC6, 0x35, 0x98, 0x18, 0xF7, 0xEC, 0x6C, 
	0x43, 0x75, 0x37, 0x26, 0xFA, 0x13, 0x94, 0x48, 
	0xF2, 0xD0, 0x8B, 0x30, 0x84, 0x54, 0xDF, 0x23, 
	0x19, 0x5B, 0x3D, 0x59, 0xF3, 0xAE, 0xA2, 0x82, 
	0x63, 0x01, 0x83, 0x2E, 0xD9, 0x51, 0x9B, 0x7C, 
	0xA6, 0xEB, 0xA5, 0xBE, 0x16, 0x0C, 0xE3, 0x61, 
	0xC0, 0x8C, 0x3A, 0xF5, 0x73, 0x2C, 0x25, 0x0B, 
	0xBB, 0x4E, 0x89, 0x6B, 0x53, 0x6A, 0xB4, 0xF1, 
	0xE1, 0xE6, 0xBD, 0x45, 0xE2, 0xF4, 0xB6, 0x66, 
	0xCC, 0x95, 0x03, 0x56, 0xD4, 0x1C, 0x1E, 0xD7, 
	0xFB, 0xC3, 0x8E, 0xB5, 0xE9, 0xCF, 0xBF, 0xBA, 
	0xEA, 0x77, 0x39, 0xAF, 0x33, 0xC9, 0x62, 0x71, 
	0x81, 0x79, 0x09, 0xAD, 0x24, 0xCD, 0xF9, 0xD8, 
	0xE5, 0xC5, 0xB9, 0x4D, 0x44, 0x08, 0x86, 0xE7, 
	0xA1, 0x1D, 0xAA, 0xED, 0x06, 0x70, 0xB2, 0xD2, 
	0x41, 0x7B, 0xA0, 0x11, 0x31, 0xC2, 0x27, 0x90, 
	0x20, 0xF6, 0x60, 0xFF, 0x96, 0x5C, 0xB1, 0xAB, 
	0x9E, 0x9C, 0x52, 0x1B, 0x5F, 0x93, 0x0A, 0xEF, 
	0x91, 0x85, 0x49, 0xEE, 0x2D, 0x4F, 0x8F, 0x3B, 
	0x47, 0x87, 0x6D, 0x46, 0xD6, 0x3E, 0x69, 0x64, 
	0x2A, 0xCE, 0xCB, 0x2F, 0xFC, 0x97, 0x05, 0x7A, 
	0xAC, 0x7F, 0xD5, 0x1A, 0x4B, 0x0E, 0xA7, 0x5A, 
	0x28, 0x14, 0x3F, 0x29, 0x88, 0x3C, 0x4C, 0x02, 
	0xB8, 0xDA, 0xB0, 0x17, 0x55, 0x1F, 0x8A, 0x7D, 
	0x57, 0xC7, 0x8D, 0x74, 0xB7, 0xC4, 0x9F, 0x72, 
	0x7E, 0x15, 0x22, 0x12, 0x58, 0x07, 0x99, 0x34, 
	0x6E, 0x50, 0xDE, 0x68, 0x65, 0xBC, 0xDB, 0xF8, 
	0xC8, 0xA8, 0x2B, 0x40, 0xDC, 0xFE, 0x32, 0xA4, 
	0xCA, 0x10, 0x21, 0xF0, 0xD3, 0x5D, 0x0F, 0x00, 
	0x6F, 0x9D, 0x36, 0x42, 0x4A, 0x5E, 0xC1, 0xE0};
	
static const unsigned char q1[256] = 
{	0x75, 0xF3, 0xC6, 0xF4, 0xDB, 0x7B, 0xFB, 0xC8, 
	0x4A, 0xD3, 0xE6, 0x6B, 0x45, 0x7D, 0xE8, 0x4B, 
	0xD6, 0x32, 0xD8, 0xFD, 0x37, 0x71, 0xF1, 0xE1, 
	0x30, 0x0F, 0xF8, 0x1B, 0x87, 0xFA, 0x06, 0x3F, 
	0x5E, 0xBA, 0xAE, 0x5B, 0x8A, 0x00, 0xBC, 0x9D, 
	0x6D, 0xC1, 0xB1, 0x0E, 0x80, 0x5D, 0xD2, 0xD5, 
	0xA0, 0x84, 0x07, 0x14, 0xB5, 0x90, 0x2C, 0xA3, 
	0xB2, 0x73, 0x4C, 0x54, 0x92, 0x74, 0x36, 0x51, 
	0x38, 0xB0, 0xBD, 0x5A, 0xFC, 0x60, 0x62, 0x96, 
	0x6C, 0x42, 0xF7, 0x10, 0x7C, 0x28, 0x27, 0x8C, 
	0x13, 0x95, 0x9C, 0xC7, 0x24, 0x46, 0x3B, 0x70, 
	0xCA, 0xE3, 0x85, 0xCB, 0x11, 0xD0, 0x93, 0xB8, 
	0xA6, 0x83, 0x20, 0xFF, 0x9F, 0x77, 0xC3, 0xCC, 
	0x03, 0x6F, 0x08, 0xBF, 0x40, 0xE7, 0x2B, 0xE2, 
	0x79, 0x0C, 0xAA, 0x82, 0x41, 0x3A, 0xEA, 0xB9, 
	0xE4, 0x9A, 0xA4, 0x97, 0x7E, 0xDA, 0x7A, 0x17, 
	0x66, 0x94, 0xA1, 0x1D, 0x3D, 0xF0, 0xDE, 0xB3, 
	0x0B, 0x72, 0xA7, 0x1C, 0xEF, 0xD1, 0x53, 0x3E, 
	0x8F, 0x33, 0x26, 0x5F, 0xEC, 0x76, 0x2A, 0x49, 
	0x81, 0x88, 0xEE, 0x21, 0xC4, 0x1A, 0xEB, 0xD9, 
	0xC5, 0x39, 0x99, 0xCD, 0xAD, 0x31, 0x8B, 0x01, 
	0x18, 0x23, 0xDD, 0x1F, 0x4E, 0x2D, 0xF9, 0x48, 
	0x4F, 0xF2, 0x65, 0x8E, 0x78, 0x5C, 0x58, 0x19, 
	0x8D, 0xE5, 0x98, 0x57, 0x67, 0x7F, 0x05, 0x64, 
	0xAF, 0x63, 0xB6, 0xFE, 0xF5, 0xB7, 0x3C, 0xA5, 
	0xCE, 0xE9, 0x68, 0x44, 0xE0, 0x4D, 0x43, 0x69, 
	0x29, 0x2E, 0xAC, 0x15, 0x59, 0xA8, 0x0A, 0x9E, 
	0x6E, 0x47, 0xDF, 0x34, 0x35, 0x6A, 0xCF, 0xDC, 
	0x22, 0xC9, 0xC0, 0x9B, 0x89, 0xD4, 0xED, 0xAB, 
	0x12, 0xA2, 0x0D, 0x52, 0xBB, 0x02, 0x2F, 0xA9, 
	0xD7, 0x61, 0x1E, 0xB4, 0x50, 0x04, 0xF6, 0xC2, 
	0x16, 0x25, 0x86, 0x56, 0x55, 0x09, 0xBE, 0x91
	};


static DWORD f32(DWORD x, const DWORD * k32, int keyLen)
{
  BYTE b[4];
  
  /* Run each byte thru 8x8 S-boxes, xoring with key byte at each stage. */
  /* Note that each byte goes through a different combination of S-boxes. */

  *((DWORD *) b) = Bswap(x);	/* make b[0] = LSB, b[3] = MSB */
  
  switch (((keyLen + 63) / 64) & 3)
  {
  case 0:			/* 256 bits of key */
    b[0] = q1[b[0]];
    b[1] = q0[b[1]];
    b[2] = q0[b[2]];
    b[3] = q1[b[3]];
    
    *((DWORD *) b) ^= k32[3];
    
    /* fall thru, having pre-processed b[0]..b[3] with k32[3] */
  case 3:			/* 192 bits of key */
    b[0] = q1[b[0]];
    b[1] = q1[b[1]];
    b[2] = q0[b[2]];
    b[3] = q0[b[3]];
    
    *((DWORD *) b) ^= k32[2];
    
    /* fall thru, having pre-processed b[0]..b[3] with k32[2] */
  case 2:			/* 128 bits of key */
    b[0] = q0[b[0]];
    b[1] = q1[b[1]];
    b[2] = q0[b[2]];
    b[3] = q1[b[3]];

    *((DWORD *) b) ^= k32[1];
  
    b[0] = q0[b[0]];
    b[1] = q0[b[1]];
    b[2] = q1[b[2]];
    b[3] = q1[b[3]];

    *((DWORD *) b) ^= k32[0];
  
    b[0] = q1[b[0]];
    b[1] = q0[b[1]];
    b[2] = q1[b[2]];
    b[3] = q0[b[3]];
  }

  /* Now perform the MDS matrix multiply inline. */
  return mds_mul(b);
}


static void init_sbox(fish2_key *key)
{ DWORD x,*sbox,z,*k32;
  int i,keyLen;
  BYTE b[4];
    
  k32=key->sboxKeys;
  keyLen=key->keyLen;
  sbox=key->sbox_full;
  
  x=0;
  for (i=0;i<256;i++,x+=0x01010101)
  { 
    *((DWORD *) b) = Bswap(x);	/* make b[0] = LSB, b[3] = MSB */
  
    switch (((keyLen + 63) / 64) & 3)
    {
    case 0:			/* 256 bits of key */
      b[0] = q1[b[0]];
      b[1] = q0[b[1]];
      b[2] = q0[b[2]];
      b[3] = q1[b[3]];
    
      *((DWORD *) b) ^= k32[3];
    
      /* fall thru, having pre-processed b[0]..b[3] with k32[3] */
    case 3:			/* 192 bits of key */
      b[0] = q1[b[0]];
      b[1] = q1[b[1]];
      b[2] = q0[b[2]];
      b[3] = q0[b[3]];
    
      *((DWORD *) b) ^= k32[2];
    
      /* fall thru, having pre-processed b[0]..b[3] with k32[2] */
    case 2:			/* 128 bits of key */
      b[0] = q0[b[0]];
      b[1] = q1[b[1]];
      b[2] = q0[b[2]];
      b[3] = q1[b[3]];

      *((DWORD *) b) ^= k32[1];
  
      b[0] = q0[b[0]];
      b[1] = q0[b[1]];
      b[2] = q1[b[2]];
      b[3] = q1[b[3]];

      *((DWORD *) b) ^= k32[0];
  
      b[0] = q1[b[0]];
      b[1] = q0[b[1]];
      b[2] = q1[b[2]];
      b[3] = q0[b[3]];
    }

    z=Mul_EF[b[0]];
    z<<=8;
    z|=Mul_EF[b[0]];
    z<<=8;
    z|=Mul_5B[b[0]];
    z<<=8;
    z|=b[0];

    sbox[i]=z;

    z=b[1];
    z<<=8;
    z|=Mul_5B[b[1]];
    z<<=8;
    z|=Mul_EF[b[1]];
    z<<=8;
    z|=Mul_EF[b[1]];

    sbox[i+256]=z;
    
    z=Mul_EF[b[2]];
    z<<=8;
    z|=b[2];
    z<<=8;
    z|=Mul_EF[b[2]];
    z<<=8;
    z|=Mul_5B[b[2]];
    
    sbox[i+512]=z;

    z=Mul_5B[b[3]];
    z<<=8;
    z|=Mul_EF[b[3]];
    z<<=8;
    z|=b[3];
    z<<=8;
    z|=Mul_5B[b[3]];
   
    sbox[i+768]=z;
  }
}

 
/* Reed-Solomon code parameters: (12,8) reversible code
   g(x) = x**4 + (a + 1/a) x**3 + a x**2 + (a + 1/a) x + 1
   where a = primitive root of field generator 0x14D */
#define RS_GF_FDBK      0x14D   /* field generator */
#define RS_rem(x) \
    { BYTE  b  =   x >> 24; \
      DWORD g2 = ((b << 1) ^ ((b & 0x80) ? RS_GF_FDBK : 0 )) & 0xFF; \
      DWORD g3 = ((b >> 1) & 0x7F) ^ ((b & 1) ? RS_GF_FDBK >> 1 : 0 ) ^ g2 ; \
      x = (x << 8) ^ (g3 << 24) ^ (g2 << 16) ^ (g3 << 8) ^ b; \
    }

static DWORD rs_mds(DWORD k0, DWORD k1)
{
  int i, j;
  DWORD r;

  for (i = r = 0; i < 2; i++)
  {
    r ^= (i) ? k0 : k1;     /* merge in 32 more key bits */
    for (j = 0; j < 4; j++) /* shift one byte at a time */
      RS_rem(r);
  }
  return r;
}


#define		INPUT_WHITEN		0	/* subkey array indices */
#define		OUTPUT_WHITEN		4
#define		ROUND_SUBKEYS		8	/* use 2 * (# rounds) */
#define		TOTAL_SUBKEYS		40

static void init_key(fish2_key * key)
{
  int i, k64Cnt;
  int keyLen = key->keyLen;
  int subkeyCnt = TOTAL_SUBKEYS;
  DWORD A, B;
  DWORD k32e[4], k32o[4];	/* even/odd key dwords */

  k64Cnt = (keyLen + 63) / 64;	/* round up to next multiple of 64 bits */
  for (i = 0; i < k64Cnt; i++)
  {				/* split into even/odd key dwords */
    k32e[i] = ((DWORD *)key->key)[2 * i];
    k32o[i] = ((DWORD *)key->key)[2 * i + 1];
    /* compute S-box keys using (12,8) Reed-Solomon code over GF(256) */
    /* store in reverse order */
    key->sboxKeys[k64Cnt - 1 - i] = 
      Bswap(rs_mds(Bswap(k32e[i]), Bswap(k32o[i])));
    
  }

  for (i = 0; i < subkeyCnt / 2; i++)	/* compute round subkeys for PHT */
  {
    A = f32(i * 0x02020202, k32e, keyLen);		/* A uses even key dwords */
    B = f32(i * 0x02020202 + 0x01010101, k32o, keyLen);	/* B uses odd  key
							   dwords */
    B = ROL(B, 8);
    key->subKeys[2 * i] = A + B;	/* combine with a PHT */
    key->subKeys[2 * i + 1] = ROL(A + 2 * B, 9);
  }
  
  init_sbox(key);
}


static inline DWORD f32_sbox(DWORD x,DWORD *sbox)
{ 
  /* Run each byte thru 8x8 S-boxes, xoring with key byte at each stage. */
  /* Note that each byte goes through a different combination of S-boxes. */

  return (sbox[        (x)     &0xff]^ 
          sbox[256 + (((x)>> 8)&0xff)]^ 
          sbox[512 + (((x)>>16)&0xff)]^ 
          sbox[768 + (((x)>>24)&0xff)]);
}


#if LINUX_VERSION_CODE >= 0x20600
typedef sector_t TransferSector_t;
# define LoopInfo_t struct loop_info64
#else
typedef int TransferSector_t;
# define LoopInfo_t struct loop_info
#endif

#if !defined(LOOP_MULTI_KEY_SETUP)
# define LOOP_MULTI_KEY_SETUP 0x4C4D
#endif
#if !defined(LOOP_MULTI_KEY_SETUP_V3)
# define LOOP_MULTI_KEY_SETUP_V3 0x4C4E
#endif

typedef struct {
    fish2_key   *keyPtr[64];
    unsigned    keyMask;
    u_int32_t   partialMD5[4];
} TwofishMultiKey;

static TwofishMultiKey *allocMultiKey(void)
{
    TwofishMultiKey *m;
    fish2_key *a;
    int x, n;

    m = (TwofishMultiKey *) kmalloc(sizeof(TwofishMultiKey), GFP_KERNEL);
    if(!m) return 0;
    memset(m, 0, sizeof(TwofishMultiKey));

    n = PAGE_SIZE / sizeof(fish2_key);
    if(!n) n = 1;

    a = (fish2_key *) kmalloc(sizeof(fish2_key) * n, GFP_KERNEL);
    if(!a) {
        kfree(m);
        return 0;    
    }

    x = 0;
    while((x < 64) && n) {
        m->keyPtr[x] = a;
        a++;
        x++;
        n--;
    }
    return m;
}

static void clearAndFreeMultiKey(TwofishMultiKey *m)
{
    fish2_key *a;
    int x, n;

    n = PAGE_SIZE / sizeof(fish2_key);
    if(!n) n = 1;

    x = 0;
    while(x < 64) {
        a = m->keyPtr[x];
        if(!a) break;
        memset(a, 0, sizeof(fish2_key) * n);
        kfree(a);
        x += n;
    }

    memset(m, 0, sizeof(TwofishMultiKey));
    kfree(m);
}

static int multiKeySetup(struct loop_device *lo, unsigned char *k, int version3)
{
    TwofishMultiKey *m;
    fish2_key *a;
    int x, y, n;
    union {
        u_int32_t     w[16];
        unsigned char b[64];
    } un;
    extern void md5_transform_CPUbyteorder_C(u_int32_t *, u_int32_t const *);

#if LINUX_VERSION_CODE >= 0x20200
#if LINUX_VERSION_CODE >= 0x30600
    if(!uid_eq(lo->lo_key_owner, current_uid()) && !capable(CAP_SYS_ADMIN))
        return -EPERM;
#elif LINUX_VERSION_CODE >= 0x2061c
    if(lo->lo_key_owner != current_uid() && !capable(CAP_SYS_ADMIN))
        return -EPERM;
#else
    if(lo->lo_key_owner != current->uid && !capable(CAP_SYS_ADMIN))
        return -EPERM;
#endif
#endif

    m = (TwofishMultiKey *)lo->key_data;
    if(!m) return -ENXIO;

    n = PAGE_SIZE / sizeof(fish2_key);
    if(!n) n = 1;

    x = 0;
    while(x < 64) {
        if(!m->keyPtr[x]) {
            a = (fish2_key *) kmalloc(sizeof(fish2_key) * n, GFP_KERNEL);
            if(!a) return -ENOMEM;
            y = x;
            while((y < (x + n)) && (y < 64)) {
                m->keyPtr[y] = a;
                a++;
                y++;
            }
        }
        if(copy_from_user(&un.b[0], k, 32)) return -EFAULT;

        a = m->keyPtr[x];
        memset(a, 0, sizeof(fish2_key));
        a->keyLen = lo->lo_encrypt_key_size << 3;
        memcpy(a->key, &un.b[0], lo->lo_encrypt_key_size);
        init_key(a);

        k += 32;
        x++;
    }

    m->partialMD5[0] = 0x67452301;
    m->partialMD5[1] = 0xefcdab89;
    m->partialMD5[2] = 0x98badcfe;
    m->partialMD5[3] = 0x10325476;
    if(version3) {
        /* only first 128 bits of iv-key is used */
        if(copy_from_user(&un.b[0], k, 16)) return -EFAULT;
#if defined(__BIG_ENDIAN)
        un.w[0] = cpu_to_le32(un.w[0]);
        un.w[1] = cpu_to_le32(un.w[1]);
        un.w[2] = cpu_to_le32(un.w[2]);
        un.w[3] = cpu_to_le32(un.w[3]);
#endif
        memset(&un.b[16], 0, 48);
        md5_transform_CPUbyteorder_C(&m->partialMD5[0], &un.w[0]);
        lo->lo_flags |= 0x080000;  /* multi-key-v3 (info exported to user space) */
    }

    m->keyMask = 0x3F;          /* range 0...63 */
    lo->lo_flags |= 0x100000;   /* multi-key (info exported to user space) */
    memset(&un.b[0], 0, 32);
    return 0;
}

#if defined(__BIG_ENDIAN)
/* twofish specific -- returns ivout[] data in CPU byte order */
static void twofish_compute_md5_iv_v3(TransferSector_t devSect, u_int32_t *ivout, u_int32_t *data)
{
    int         x, y, e;
    u_int32_t   buf[16];
    extern void md5_transform_CPUbyteorder(u_int32_t *, u_int32_t const *);

    y = 7;
    e = 16;
    do {
        if (!y) {
            e = 12;
            /* md5_transform_CPUbyteorder wants data in CPU byte order */
            /* devSect is already in CPU byte order -- no need to convert */
            if(sizeof(TransferSector_t) == 8) {
                /* use only 56 bits of sector number */
                buf[12] = devSect;
                buf[13] = (((u_int64_t)devSect >> 32) & 0xFFFFFF) | 0x80000000;
            } else {
                /* 32 bits of sector number + 24 zero bits */
                buf[12] = devSect;
                buf[13] = 0x80000000;
            }
            /* 4024 bits == 31 * 128 bit plaintext blocks + 56 bits of sector number */
            buf[14] = 4024;
            buf[15] = 0;
        }
        x = 0;
        do {
            buf[x    ] = cpu_to_le32(data[0]);
            buf[x + 1] = cpu_to_le32(data[1]);
            buf[x + 2] = cpu_to_le32(data[2]);
            buf[x + 3] = cpu_to_le32(data[3]);
            x += 4;
            data += 4;
        } while (x < e);
        md5_transform_CPUbyteorder(&ivout[0], &buf[0]);
    } while (--y >= 0);
    /* caller wants ivout[] data in CPU byte order -- no conversion needed here */
}
#else
/* on little endian boxes loop_compute_md5_iv_v3() returns ivout[] data in little  */
/* endian byte order which happens to be same as CPU byte order, so we use that */
extern void loop_compute_md5_iv_v3(TransferSector_t, u_int32_t *, u_int32_t *);
#define twofish_compute_md5_iv_v3(a,b,c) loop_compute_md5_iv_v3((a),(b),(c))
#endif


#define roundE_m(x0,x1,x2,x3,rnd) \
      t0 = f32_sbox( x0, key->sbox_full ) ; \
      t1 = f32_sbox( ROL(x1,8), key->sbox_full ); \
      x2 ^= t0 + t1 + key->subKeys[2*rnd+8]; \
      x3 = ROL(x3,1); \
      x3 ^= t0 + 2*t1 + key->subKeys[2*rnd+9]; \
      x2 = ROR(x2,1);

static void blockEncrypt_CBC(fish2_key *key,BYTE *src,BYTE *dst,DWORD iv0,DWORD iv1,DWORD iv2,DWORD iv3)
{ DWORD xx0,xx1,xx2,xx3,t0,t1;
  int len;

  for (len=512;len>=16;len-=16)
  { 
    xx0=Bswap(((DWORD *)src)[0]) ^ key->subKeys[0] ^ iv0;
    xx1=Bswap(((DWORD *)src)[1]) ^ key->subKeys[1] ^ iv1;
    xx2=Bswap(((DWORD *)src)[2]) ^ key->subKeys[2] ^ iv2;
    xx3=Bswap(((DWORD *)src)[3]) ^ key->subKeys[3] ^ iv3;
    
    src+=16;

    roundE_m(xx0,xx1,xx2,xx3,0);
    roundE_m(xx2,xx3,xx0,xx1,1);
    roundE_m(xx0,xx1,xx2,xx3,2);
    roundE_m(xx2,xx3,xx0,xx1,3);
    roundE_m(xx0,xx1,xx2,xx3,4);
    roundE_m(xx2,xx3,xx0,xx1,5);
    roundE_m(xx0,xx1,xx2,xx3,6);
    roundE_m(xx2,xx3,xx0,xx1,7);
    roundE_m(xx0,xx1,xx2,xx3,8);
    roundE_m(xx2,xx3,xx0,xx1,9);
    roundE_m(xx0,xx1,xx2,xx3,10);
    roundE_m(xx2,xx3,xx0,xx1,11);
    roundE_m(xx0,xx1,xx2,xx3,12);
    roundE_m(xx2,xx3,xx0,xx1,13);
    roundE_m(xx0,xx1,xx2,xx3,14);
    roundE_m(xx2,xx3,xx0,xx1,15);
    
    iv0=xx2 ^ key->subKeys[4];
    iv1=xx3 ^ key->subKeys[5];
    iv2=xx0 ^ key->subKeys[6];
    iv3=xx1 ^ key->subKeys[7];
    
    ((DWORD *)dst)[0] = Bswap(iv0);
    ((DWORD *)dst)[1] = Bswap(iv1);
    ((DWORD *)dst)[2] = Bswap(iv2);
    ((DWORD *)dst)[3] = Bswap(iv3);
    dst+=16;
  }
}

#define roundD_m(x0,x1,x2,x3,rnd) \
      t0 = f32_sbox( x0, key->sbox_full); \
      t1 = f32_sbox( ROL(x1,8),key->sbox_full); \
      x2 = ROL(x2,1); \
      x3 ^= t0 + 2*t1 + key->subKeys[rnd*2+9]; \
      x3 = ROR(x3,1); \
      x2 ^= t0 + t1 + key->subKeys[rnd*2+8];

static void blockDecrypt_CBC(fish2_key *key,BYTE *src,BYTE *dst,int len,DWORD iv0,DWORD iv1,DWORD iv2,DWORD iv3)
{ DWORD xx0,xx1,xx2,xx3,t0,t1,lx0,lx1,lx2,lx3;

  for (;len>=16;len-=16)
  { 
    lx0=iv0;iv0=Bswap(((DWORD *)src)[0]);xx0=iv0 ^ key->subKeys[4];
    lx1=iv1;iv1=Bswap(((DWORD *)src)[1]);xx1=iv1 ^ key->subKeys[5];
    lx2=iv2;iv2=Bswap(((DWORD *)src)[2]);xx2=iv2 ^ key->subKeys[6];
    lx3=iv3;iv3=Bswap(((DWORD *)src)[3]);xx3=iv3 ^ key->subKeys[7];
    src+=16;
    
    roundD_m(xx0,xx1,xx2,xx3,15);
    roundD_m(xx2,xx3,xx0,xx1,14);
    roundD_m(xx0,xx1,xx2,xx3,13);
    roundD_m(xx2,xx3,xx0,xx1,12);
    roundD_m(xx0,xx1,xx2,xx3,11);
    roundD_m(xx2,xx3,xx0,xx1,10);
    roundD_m(xx0,xx1,xx2,xx3,9);
    roundD_m(xx2,xx3,xx0,xx1,8);
    roundD_m(xx0,xx1,xx2,xx3,7);
    roundD_m(xx2,xx3,xx0,xx1,6);
    roundD_m(xx0,xx1,xx2,xx3,5);
    roundD_m(xx2,xx3,xx0,xx1,4);
    roundD_m(xx0,xx1,xx2,xx3,3);
    roundD_m(xx2,xx3,xx0,xx1,2);
    roundD_m(xx0,xx1,xx2,xx3,1);
    roundD_m(xx2,xx3,xx0,xx1,0);

    ((DWORD *)dst)[0] = Bswap(xx2 ^ key->subKeys[0] ^ lx0);
    ((DWORD *)dst)[1] = Bswap(xx3 ^ key->subKeys[1] ^ lx1);
    ((DWORD *)dst)[2] = Bswap(xx0 ^ key->subKeys[2] ^ lx2);
    ((DWORD *)dst)[3] = Bswap(xx1 ^ key->subKeys[3] ^ lx3);
    dst+=16;
  }
}

static int transfer_fish2(struct loop_device *lo, int cmd, char *raw_buf,
                  char *loop_buf, int size, TransferSector_t devSect)
{
    TwofishMultiKey *m;
    fish2_key *a;
    u_int32_t iv[4];
    int sectInc = 1;
    unsigned y;

    if (lo->lo_init[0] == 1) sectInc = devSect = 0; /* "-o loinit=1" means SuSE compatible */
    if (size & 0x1FF) return -1;
    m = (TwofishMultiKey *)lo->key_data;
    y = m->keyMask;
    if (cmd == READ) {
        while(size > 0) {
            a = m->keyPtr[((unsigned)devSect) & y];
            if(y) {
                iv[0] = Bswap(((u_int32_t *)raw_buf)[0]);
                iv[1] = Bswap(((u_int32_t *)raw_buf)[1]);
                iv[2] = Bswap(((u_int32_t *)raw_buf)[2]);
                iv[3] = Bswap(((u_int32_t *)raw_buf)[3]);
                blockDecrypt_CBC(a, raw_buf+16, loop_buf+16, 496, iv[0], iv[1], iv[2], iv[3]);
                memcpy(&iv[0], &m->partialMD5[0], 16);
                twofish_compute_md5_iv_v3(devSect, &iv[0], (u_int32_t *)(&loop_buf[16]));
                blockDecrypt_CBC(a, raw_buf, loop_buf, 16, iv[0], iv[1], iv[2], iv[3]);
            } else {
                if(sizeof(TransferSector_t) == 8) {
                    blockDecrypt_CBC(a, raw_buf, loop_buf, 512, devSect, (__u64)devSect>>32, 0, 0);
                } else {
                    blockDecrypt_CBC(a, raw_buf, loop_buf, 512, devSect, 0, 0, 0);
                }
            }
#if LINUX_VERSION_CODE >= 0x20600
            cond_resched();
#elif LINUX_VERSION_CODE >= 0x20400
            if(current->need_resched) {set_current_state(TASK_RUNNING);schedule();}
#else
            if(current->need_resched) {current->state=TASK_RUNNING;schedule();}
#endif
            raw_buf += 512;
            loop_buf += 512;
            size -= 512;
            devSect += sectInc;
        }
    } else {
        while(size > 0) {
            a = m->keyPtr[((unsigned)devSect) & y];
            if(y) {
#if LINUX_VERSION_CODE < 0x20400
                /* on 2.2 and older kernels, real raw_buf may be doing */
                /* writes at any time, so this needs to be stack buffer */
                u_int32_t tmp_raw_buf[128];
                char *TMP_RAW_BUF = (char *)(&tmp_raw_buf[0]);
#else
                /* on 2.4 and later kernels, real raw_buf is not doing */
                /* any writes now so it can be used as temp buffer */
# define TMP_RAW_BUF raw_buf
#endif
                memcpy(TMP_RAW_BUF, loop_buf, 512);
                memcpy(&iv[0], &m->partialMD5[0], 16);
                twofish_compute_md5_iv_v3(devSect, &iv[0], (u_int32_t *)(&TMP_RAW_BUF[16]));
                blockEncrypt_CBC(a, TMP_RAW_BUF, raw_buf, iv[0], iv[1], iv[2], iv[3]);
            } else {
                if(sizeof(TransferSector_t) == 8) {
                    blockEncrypt_CBC(a, loop_buf, raw_buf, devSect, (__u64)devSect>>32, 0, 0);
                } else {
                    blockEncrypt_CBC(a, loop_buf, raw_buf, devSect, 0, 0, 0);
                }
            }
#if LINUX_VERSION_CODE >= 0x20600
            cond_resched();
#elif LINUX_VERSION_CODE >= 0x20400
            if(current->need_resched) {set_current_state(TASK_RUNNING);schedule();}
#else
            if(current->need_resched) {current->state=TASK_RUNNING;schedule();}
#endif
            raw_buf += 512;
            loop_buf += 512;
            size -= 512;
            devSect += sectInc;
        }
    }
    return 0;
}

static int fish2_init(struct loop_device *lo, LoopInfo_t *info)
{
    TwofishMultiKey *m;
    fish2_key *a;
  
    if (info->lo_encrypt_key_size<16 || info->lo_encrypt_key_size>32)
        return -EINVAL;
    lo->key_data = m = allocMultiKey();
    if(!m) return(-ENOMEM);

    a = m->keyPtr[0];  
    memset(a, 0, sizeof(fish2_key));
    a->keyLen = info->lo_encrypt_key_size << 3;
    memcpy(a->key, info->lo_encrypt_key, info->lo_encrypt_key_size);
    init_key(a);

    memset(&info->lo_encrypt_key[0], 0, sizeof(info->lo_encrypt_key));
    return 0;
}

static int fish2_release(struct loop_device *lo)
{
    if(lo->key_data) {
        clearAndFreeMultiKey((TwofishMultiKey *)lo->key_data);
        lo->key_data = 0;
    }
    return(0);
}

static int handleIoctl_fish2(struct loop_device *lo, int cmd, unsigned long arg)
{
    int err;

    switch (cmd) {
    case LOOP_MULTI_KEY_SETUP:
        err = multiKeySetup(lo, (unsigned char *)arg, 0);
        break;
    case LOOP_MULTI_KEY_SETUP_V3:
        err = multiKeySetup(lo, (unsigned char *)arg, 1);
        break;
    default:
        err = -EINVAL;
    }
    return err;
}

#if LINUX_VERSION_CODE < 0x20600
static void fish2_lock(struct loop_device *lo)
{
    MOD_INC_USE_COUNT;
}
static void fish2_unlock(struct loop_device *lo)
{
    MOD_DEC_USE_COUNT;
}   
#endif

static struct loop_func_table fish2_funcs = {
  number: 3, /* 3 == LO_CRYPT_FISH2 */
  transfer: (void *) transfer_fish2,
  init: (void *) fish2_init,
  release: fish2_release,
#if LINUX_VERSION_CODE >= 0x20600
  owner: THIS_MODULE,
#else
  lock: fish2_lock,
  unlock: fish2_unlock,
#endif
  ioctl: (void *) handleIoctl_fish2
};

#if LINUX_VERSION_CODE >= 0x20600
# define loop_twofish_init  __init loop_twofish_initfn
# define loop_twofish_exit  loop_twofish_exitfn
#else
# define loop_twofish_init  init_module
# define loop_twofish_exit  cleanup_module
#endif

int loop_twofish_init(void)
{ 
  if (loop_register_transfer(&fish2_funcs)) {
    printk(KERN_WARNING "loop: unable to register twofish transfer\n");
    return -EIO;
  }
  printk(KERN_INFO "loop: registered twofish encryption\n");
  return 0;
}

void loop_twofish_exit(void)
{ 
  if (loop_unregister_transfer(fish2_funcs.number)) {
    printk(KERN_WARNING "loop: unable to unregister twofish transfer\n");
    return;
  }
  printk(KERN_INFO "loop: unregistered twofish encryption\n");
}

#if LINUX_VERSION_CODE >= 0x20600
module_init(loop_twofish_initfn);
module_exit(loop_twofish_exitfn);
#endif

#if defined(MODULE_LICENSE)
MODULE_LICENSE("GPL");
#endif
