#ifndef _PIC24_H_
#define _PIC24_H_

#define THECUS_PIC24FJ128_SYS		0
#define THECUS_PIC24FJ128_LED_SATA_FAIL	1
#define THECUS_PIC24FJ128_LED_SYS_FAIL	2
#define THECUS_PIC24FJ128_LED_SYS_BUSY	3
#define THECUS_PIC24FJ128_BUZZER		4
#define THECUS_PIC24FJ128_PWR_STATUS	5
#define THECUS_PIC24FJ128_VERSION	6
#define THECUS_PIC24FJ128_LCM_DISPLAY	7
#define THECUS_PIC24FJ128_LCM_HOSTNAME	8
#define THECUS_PIC24FJ128_LCM_WAN_IP	9
#define THECUS_PIC24FJ128_LCM_LAN_IP	10
#define THECUS_PIC24FJ128_LCM_RAID	11
#define THECUS_PIC24FJ128_LCM_FAN	12
#define THECUS_PIC24FJ128_LCM_TEMP	13
#define THECUS_PIC24FJ128_LCM_DATE	14
#define THECUS_PIC24FJ128_LCM_UPTIME	15
#define THECUS_PIC24FJ128_LCM_MSG_UPPER	16
#define THECUS_PIC24FJ128_LCM_MSG_LOWER	17
#define THECUS_PIC24FJ128_LCM_MSG_TIME	18
#define THECUS_PIC24FJ128_LCM_USB	19
#define THECUS_PIC24FJ128_LCM_ESATA	20
#define THECUS_PIC24FJ128_LCM_USER1	21
#define THECUS_PIC24FJ128_LCM_USER2	22
#define THECUS_PIC24FJ128_LCM_USER3	23
#define THECUS_PIC24FJ128_IR		24
#define THECUS_PIC24FJ128_INT_STATUS	25
#define THECUS_PIC24FJ128_BTN_SW	26
#define THECUS_PIC24FJ128_BTN_OP	27
#define THECUS_PIC24FJ128_FACTORY	28
#define THECUS_PIC24FJ128_LED_OSD	29
#define THECUS_PIC24FJ128_AC_POWER	30
#define THECUS_PIC24FJ128_MODEL		31
#define THECUS_PIC24FJ128_PCI_EXIST	32
#define THECUS_PIC24FJ128_LCM_BANNER	33

#define THECUS_PIC24FJ128_SYS_PWR_OFF	0
#define THECUS_PIC24FJ128_SYS_PWR_HRESET	1
#define THECUS_PIC24FJ128_SYS_PCI_RESET	2

#define THECUS_PIC24FJ128_LED_SATA_FAIL_OFF	0
#define THECUS_PIC24FJ128_LED_SATA_FAIL_ON	1
#define THECUS_PIC24FJ128_LED_SATA_FAIL_BLINK	2

#define THECUS_PIC24FJ128_LED_SYS_FAIL_OFF	0
#define THECUS_PIC24FJ128_LED_SYS_FAIL_ON		1
#define THECUS_PIC24FJ128_LED_SYS_FAIL_BLINK	2

#define THECUS_PIC24FJ128_LED_SYS_BUSY_OFF	0
#define THECUS_PIC24FJ128_LED_SYS_BUSY_ON		1
#define THECUS_PIC24FJ128_LED_SYS_BUSY_BLINK	2

#define THECUS_PIC24FJ128_BUZZER_OFF		0
#define THECUS_PIC24FJ128_BUZZER_ON		1

#define THECUS_PIC24FJ128_PWR_STATUS_OFF	0
#define THECUS_PIC24FJ128_PWR_STATUS_BOOTING	1
#define THECUS_PIC24FJ128_PWR_STATUS_BOOTED	2
#define THECUS_PIC24FJ128_PWR_STATUS_UBOOT	3

#define THECUS_PIC24FJ128_LCM_DISPLAY_OFF	0
#define THECUS_PIC24FJ128_LCM_DISPLAY_ON		1

#define THECUS_PIC24FJ128_LCM_USB_COPYING	0x66
#define THECUS_PIC24FJ128_LCM_USB_COPY_OK	0x67
#define THECUS_PIC24FJ128_LCM_USB_COPY_FAIL	0x68

#define THECUS_PIC24FJ128_LCM_ESATA_COPYING	0x66
#define THECUS_PIC24FJ128_LCM_ESATA_COPY_OK	0x67
#define THECUS_PIC24FJ128_LCM_ESATA_COPY_FAIL	0x68

#define THECUS_PIC24FJ128_INT_POWER	0
#define THECUS_PIC24FJ128_INT_USB	1
#define THECUS_PIC24FJ128_INT_USER1	2
#define THECUS_PIC24FJ128_INT_USER2	3
#define THECUS_PIC24FJ128_INT_USER3	4
#define THECUS_PIC24FJ128_INT_IR		5
#define THECUS_PIC24FJ128_INT_IR_MCE	6
#define THECUS_PIC24FJ128_INT_IR_RC5	7
#define THECUS_PIC24FJ128_INT_DEFAULT	255

#define I2C_RETRY 1

#define PIC24FJ128_I2C_ID 0x36

#ifdef PIC24_DEBUG
# define _PIC24_DBG(x, fmt, args...) do{ if (x>=DEBUG) printk("%s: " fmt "\n", __FUNCTION__, ##args); } while(0);
#else
# define _PIC24_DBG(x, fmt, args...) do { } while(0);
#endif

struct rec {
    u8 reg_num;
    u8 val[20];
    int size;
};


u8 addQ(u8 reg_num, u8 * val, int size);
int isFullQ(void);
int isEmptyQ(void);
struct rec *removeQ(void);


#endif
