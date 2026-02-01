function selectSerialPort({ ports, serialPortOverride }) {
  if (serialPortOverride) {
    const match = ports.find(p => p.path === serialPortOverride);
    return {
      path: serialPortOverride,
      source: 'override',
      matched: Boolean(match)
    };
  }

  const heltecPort = ports.find(p => {
    const vendorId = (p.vendorId || '').toLowerCase();
    const path = p.path || '';
    const vendorMatch = vendorId.includes('10c4') || vendorId.includes('1a86');
    const pathMatch = path.includes('ttyUSB') || path.includes('ttyACM') || path.includes('COM');
    return vendorMatch || pathMatch;
  });

  if (!heltecPort) {
    return null;
  }

  return {
    path: heltecPort.path,
    source: 'auto',
    matched: true
  };
}

module.exports = { selectSerialPort };
