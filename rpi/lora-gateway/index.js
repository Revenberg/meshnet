// LoraGateway Service
// Bridge between Docker backend and LoRa network (Heltec device on RPI4 USB)

const express = require('express');
const SerialPort = require('serialport').SerialPort;
const ReadlineParser = require('@serialport/parser-readline').ReadlineParser;
const axios = require('axios');
const bodyParser = require('body-parser');
const os = require('os');
const { selectSerialPort } = require('./serialConfig');

const app = express();
const PORT = process.env.PORT || 3002;
const BACKEND_URL = process.env.BACKEND_URL || 'http://backend:3001';
const SERIAL_PORT = process.env.SERIAL_PORT || '';
const SERIAL_BAUD = Number(process.env.SERIAL_BAUD || '115200');

let serialPort = null;
let isConnected = false;
const connectedNodes = new Map();
const lastSent = new Map();
const lastAck = new Map();
const PING_INTERVAL_MS = 60 * 1000;

// ============ MIDDLEWARE ============
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

// ============ GATEWAY INITIALIZATION ============
async function initializeGateway() {
  try {
    console.log('[LoraGateway] Searching for Heltec device on USB...');
    
    // List available ports
    const ports = await SerialPort.list();
    console.log('[LoraGateway] Available ports:', ports.map(p => p.path).join(', '));

    const selection = selectSerialPort({ ports, serialPortOverride: SERIAL_PORT });
    
    if (!selection) {
      console.log('[LoraGateway] ⚠️ No Heltec device found. Continuing in simulation mode.');
      // Continue without serial connection for now
      return;
    }

    if (selection.source === 'override' && !selection.matched) {
      console.log(`[LoraGateway] SERIAL_PORT not found in list, attempting: ${selection.path}`);
    }

    console.log(`[LoraGateway] Using serial port: ${selection.path} (${selection.source})`);
    
    // Open serial connection
    serialPort = new SerialPort({
      path: selection.path,
      baudRate: SERIAL_BAUD,
      dataBits: 8,
      stopBits: 1,
      parity: 'none'
    });
    
    const parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));
    
    // Handle incoming LoRa messages from device
    parser.on('data', async (data) => {
      handleLoRaMessage(data);
    });
    
    serialPort.on('open', () => {
      console.log('[LoraGateway] ✅ Serial connection opened');
      isConnected = true;
    });
    
    serialPort.on('error', (err) => {
      console.error('[LoraGateway] Serial port error:', err.message);
      isConnected = false;
    });
    
  } catch (error) {
    console.error('[LoraGateway] Error initializing gateway:', error.message);
    console.log('[LoraGateway] Continuing in simulation mode');
  }
}

async function schedulePings() {
  try {
    const res = await axios.get(`${BACKEND_URL}/api/nodes`);
    const nodes = res.data || [];
    const now = Date.now();
    for (const node of nodes) {
      const nodeId = node.nodeId;
      const lastAckTs = lastAck.get(nodeId) || 0;
      const lastSentTs = lastSent.get(nodeId) || 0;
      if (now - lastAckTs > PING_INTERVAL_MS && now - lastSentTs > PING_INTERVAL_MS) {
        await sendSerialRaw(`LORA_TX;PING;${nodeId}`);
        lastSent.set(nodeId, now);
      }
    }
  } catch (error) {
    console.error('[PING] Scheduler error:', error.message);
  }
}

// ============ MESSAGE HANDLING ============
async function handleLoRaMessage(data) {
  try {
    const message = data.trim();
    if (!message) return;
    
    console.log(`[LoRa RX] ${message}`);

    if (message.startsWith('LORA_RX;')) {
      await handleLoRaMessage(message.substring('LORA_RX;'.length));
      return;
    }

    if (message.startsWith('REQ;USERS;')) {
      const nodeId = message.split(';')[2] || '';
      await handleUsersRequest(nodeId);
      return;
    }

    if (message.startsWith('REQ;PAGES;')) {
      const nodeId = message.split(';')[2] || '';
      await handlePagesRequest(nodeId);
      return;
    }

    if (message.includes('BEACON;')) {
      await handleBeacon(message);
      return;
    }

    if (message.includes('Offline:')) {
      handleOffline(message);
      return;
    }

    if (message.startsWith('ACK;')) {
      await handleAck(message);
      return;
    }

    if (message.startsWith('PONG;')) {
      await handlePong(message);
      return;
    }
    
    // Parse different message types
    if (message.startsWith('[PING]')) {
      handlePing(message);
    } else if (message.startsWith('[BROADCAST RX]')) {
      handleBroadcast(message);
    } else if (message.startsWith('[NODE]')) {
      handleNodeStatus(message);
    } else if (message.startsWith('[METRICS]')) {
      handleMetrics(message);
    }
  } catch (error) {
    console.error('[LoRa ERROR]', error.message);
  }
}

async function sendSerialRaw(packet) {
  if (!serialPort || !isConnected) {
    console.log('[LoraGateway] Serial not connected, skipping transmission');
    return false;
  }
  return new Promise((resolve) => {
    serialPort.write(packet + '\n', (err) => {
      if (err) {
        console.error('[LoRa TX ERROR]', err.message);
        resolve(false);
        return;
      }
      console.log(`[LoRa TX] ${packet}`);
      resolve(true);
    });
  });
}

async function handleUsersRequest(nodeId) {
  try {
    const res = await axios.get(`${BACKEND_URL}/api/sync/users`);
    const users = res.data.users || [];
    const payload = users.map(u => `${u.username}|${u.password_hash}|${u.team}`).join(';');
    await sendSerialRaw(`RESP;USERS;${payload}`);
    lastSent.set(nodeId, Date.now());
    await axios.post(`${BACKEND_URL}/api/nodes/register`, { nodeId });
    await sendSerialRaw(`LORA_TX;BCAST;${Date.now()};SYSTEM;3;Node connected: ${nodeId}`);
  } catch (error) {
    console.error('[Users Sync] Error:', error.message);
  }
}

async function handlePagesRequest(nodeId) {
  try {
    const res = await axios.get(`${BACKEND_URL}/api/sync/pages`, { params: { nodeId } });
    const pages = res.data.pages || [];
    const payload = pages.map(p => `${p.team}|${encodeURIComponent(p.html || '')}`).join(';');
    await sendSerialRaw(`RESP;PAGES;${payload}`);
    lastSent.set(nodeId, Date.now());
    await axios.post(`${BACKEND_URL}/api/nodes/register`, { nodeId });
  } catch (error) {
    console.error('[Pages Sync] Error:', error.message);
  }
}

async function handleAck(message) {
  const parts = message.split(';');
  if (parts.length < 6) return;
  const [, msgId, nodeId, object, func, timestamp] = parts;
  try {
    await axios.post(`${BACKEND_URL}/api/acks`, { msgId, nodeId, object, function: func, timestamp: Number(timestamp) });
    lastAck.set(nodeId, Date.now());
  } catch (error) {
    console.error('[ACK] Error:', error.message);
  }
}

async function handlePong(message) {
  const parts = message.split(';');
  if (parts.length < 3) return;
  const [, nodeId] = parts;
  try {
    await axios.post(`${BACKEND_URL}/api/pings`, { nodeId, status: 'ok' });
    connectedNodes.set(nodeId, {
      nodeId,
      lastSeen: new Date(),
      status: 'online'
    });
    await axios.post(`${BACKEND_URL}/api/nodes/register`, { nodeId });
    lastAck.set(nodeId, Date.now());
  } catch (error) {
    console.error('[PONG] Error:', error.message);
  }
}

setInterval(schedulePings, PING_INTERVAL_MS);

async function pingScheduler() {
  if (!serialPort || !isConnected) return;
  try {
    const res = await axios.get(`${BACKEND_URL}/api/nodes`);
    const nodes = res.data || [];
    const now = Date.now();
    for (const node of nodes) {
      const nodeId = node.nodeId;
      const lastAckTime = lastAck.get(nodeId) || 0;
      const lastSentTime = lastSent.get(nodeId) || 0;
      if (now - lastAckTime > 60000 && now - lastSentTime > 60000) {
        await sendSerialRaw(`LORA_TX;PING;${nodeId}`);
        lastSent.set(nodeId, now);
      }
    }
  } catch (error) {
    console.error('[PING] Scheduler error:', error.message);
  }
}

function handlePing(message) {
  // [PING] NodeID: abc123 | Battery: 87% | Signal: -75dBm
  const parts = message.match(/NodeID: ([^ |]+)|Battery: ([0-9]+)%|Signal: (-?[0-9]+)dBm/g) || [];
  const nodeId = parts[0]?.split(': ')[1];
  const battery = parseInt(parts[1]?.split(': ')[1]);
  const signal = parseInt(parts[2]?.split(': ')[1]);
  
  if (nodeId) {
    connectedNodes.set(nodeId, {
      nodeId,
      battery,
      signal,
      lastSeen: new Date(),
      status: 'online'
    });
    
    console.log(`[PING OK] ${nodeId} | Battery: ${battery}% | Signal: ${signal}dBm`);
    
    // Update backend
    updateNodeStatus(nodeId, { battery, signalStrength: signal, lastSeen: new Date() });
  }
}

function handleBroadcast(message) {
  // [BROADCAST RX] User: Sander | TTL: 120 | Params: ...
  console.log(`[BROADCAST] ${message}`);
}

function handleNodeStatus(message) {
  console.log(`[NODE STATUS] ${message}`);
}

function handleMetrics(message) {
  console.log(`[METRICS] ${message}`);
}

async function handleBeacon(message) {
  const match = message.match(/BEACON;([^;\s]+)/);
  const nodeId = match?.[1] || '';
  if (!nodeId) return;

  connectedNodes.set(nodeId, {
    nodeId,
    lastSeen: new Date(),
    status: 'online'
  });

  try {
    await axios.post(`${BACKEND_URL}/api/nodes/register`, { nodeId });
    console.log(`[BEACON] Registered node ${nodeId}`);
  } catch (error) {
    console.error(`[BEACON] Register failed for ${nodeId}:`, error.message);
  }
}

function handleOffline(message) {
  const match = message.match(/Offline:\s*(\S+)/);
  const nodeId = match?.[1];
  if (!nodeId) return;

  const existing = connectedNodes.get(nodeId) || { nodeId };
  connectedNodes.set(nodeId, {
    ...existing,
    status: 'offline',
    lastSeen: new Date()
  });
}

// ============ SENDING TO LORA ============
async function sendToLoRa(packet) {
  if (!serialPort || !isConnected) {
    console.log('[LoraGateway] Serial not connected, skipping transmission');
    return false;
  }
  
  try {
    serialPort.write(packet + '\n', (err) => {
      if (err) {
        console.error('[LoRa TX ERROR]', err.message);
        return false;
      }
      console.log(`[LoRa TX] ${packet}`);
    });
    return true;
  } catch (error) {
    console.error('[LoRa TX]', error.message);
    return false;
  }
}

// ============ BACKEND COMMUNICATION ============
async function updateNodeStatus(nodeId, data) {
  try {
    await axios.put(`${BACKEND_URL}/api/nodes/${nodeId}`, data);
    console.log(`[BACKEND] Updated node ${nodeId}`);
  } catch (error) {
    console.error(`[BACKEND ERROR] Failed to update ${nodeId}:`, error.message);
  }
}

async function relayBroadcast(broadcast) {
  try {
    // Format: BCAST;msgId;username;ttl;content
    const packet = `BCAST;${Date.now()};${broadcast.username};${broadcast.ttl};${broadcast.content}`;
    console.log(`[RELAY BROADCAST] Received from ${broadcast.username}: "${broadcast.content.substring(0, 50)}..."`);
    console.log(`[RELAY BROADCAST] Packet format: ${packet.substring(0, 60)}...`);
    
    const success = await sendToLoRa(packet);
    
    if (success) {
      console.log(`[RELAY OK] Broadcast from ${broadcast.username} sent via LoRa`);
      return { success: true, message: 'Broadcast relayed to LoRa network' };
    } else {
      console.log(`[RELAY SIMULATION] Serial not connected - broadcast logged (simulation mode)`);
      // In simulation mode, log and continue
      return { success: true, message: 'Broadcast logged (simulation mode)' };
    }
  } catch (error) {
    console.error('[RELAY ERROR]', error.message);
    return { success: false, message: error.message };
  }
}

// ============ ROUTES ============

// Health check
app.get('/health', (req, res) => {
  res.json({
    status: 'ok',
    service: 'LoraGateway',
    serialConnected: isConnected,
    connectedNodes: connectedNodes.size,
    hostname: os.hostname()
  });
});

// Get gateway status
app.get('/status', (req, res) => {
  res.json({
    connected: isConnected,
    nodesConnected: Array.from(connectedNodes.values()),
    timestamp: new Date()
  });
});

// Send broadcast to LoRa network
app.post('/relay/broadcast', async (req, res) => {
  try {
    const broadcast = req.body;
    
    if (!broadcast.username || !broadcast.content) {
      return res.status(400).json({ error: 'username and content required' });
    }
    
    const result = await relayBroadcast(broadcast);
    
    if (result.success) {
      res.json({ success: true, message: result.message });
    } else {
      res.status(503).json({ error: result.message });
    }
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Send raw LoRa packet
app.post('/relay/packet', async (req, res) => {
  try {
    const { packet } = req.body;
    
    if (!packet) {
      return res.status(400).json({ error: 'packet required' });
    }
    
    const success = await sendToLoRa(packet);
    
    if (success) {
      res.json({ success: true, message: 'Packet sent' });
    } else {
      res.status(503).json({ error: 'Failed to send packet' });
    }
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get connected nodes
app.get('/nodes', (req, res) => {
  res.json(Array.from(connectedNodes.values()));
});

// ============ ERROR HANDLING ============
process.on('SIGINT', () => {
  console.log('\n[LoraGateway] Shutting down...');
  if (serialPort && serialPort.isOpen) {
    serialPort.close();
  }
  process.exit(0);
});

// ============ START SERVER ============
async function start() {
  await initializeGateway();
  
  app.listen(PORT, () => {
    console.log(`[LoraGateway] 🚀 Server running on port ${PORT}`);
    console.log(`[LoraGateway] Serial connected: ${isConnected}`);
    console.log(`[LoraGateway] Connected nodes: ${connectedNodes.size}`);
  });

  setInterval(pingScheduler, 60 * 1000);
}

start().catch(console.error);
