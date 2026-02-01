const assert = require('assert');
const { selectSerialPort } = require('../serialConfig');

function port(path, vendorId) {
  return { path, vendorId };
}

// Override should always win, even if not in list
{
  const ports = [port('/dev/ttyUSB0', '10C4')];
  const res = selectSerialPort({ ports, serialPortOverride: '/dev/ttyUSB9' });
  assert.strictEqual(res.path, '/dev/ttyUSB9');
  assert.strictEqual(res.source, 'override');
  assert.strictEqual(res.matched, false);
}

// Override should report matched true when present in list
{
  const ports = [port('/dev/ttyUSB0', '10C4')];
  const res = selectSerialPort({ ports, serialPortOverride: '/dev/ttyUSB0' });
  assert.strictEqual(res.path, '/dev/ttyUSB0');
  assert.strictEqual(res.source, 'override');
  assert.strictEqual(res.matched, true);
}

// Auto-detect via vendorId
{
  const ports = [port('/dev/ttyUSB0', '10C4')];
  const res = selectSerialPort({ ports, serialPortOverride: '' });
  assert.strictEqual(res.path, '/dev/ttyUSB0');
  assert.strictEqual(res.source, 'auto');
  assert.strictEqual(res.matched, true);
}

// Auto-detect via path fallback
{
  const ports = [port('/dev/ttyACM0', undefined)];
  const res = selectSerialPort({ ports, serialPortOverride: '' });
  assert.strictEqual(res.path, '/dev/ttyACM0');
}

// No match returns null
{
  const ports = [port('/dev/ttyS0', undefined)];
  const res = selectSerialPort({ ports, serialPortOverride: '' });
  assert.strictEqual(res, null);
}

console.log('serialConfig tests passed');
