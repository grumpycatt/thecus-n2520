
var assert = require('assert')

module.exports = function (gm, dir, finish, GM) {

  var m = gm
  .cycle(4);

  var args = m.args();
  assert.equal('convert', args[0]);
  assert.equal('-cycle', args[2]);
  assert.equal(4, args[3]);

  if (!GM.integration)
    return finish();

  m
  .write(dir + '/cycle.png', function cycle (err) {
    finish(err);
  });
}
