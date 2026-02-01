/**
 * LoraGateway Service
 * Bridges Docker backend (Node.js) with LoRa mesh network
 * Handles USB communication to Heltec LoRa devices
 * 
 * Features:
 * - Listens for broadcast messages from backend API
 * - Sends LoRa packets to all connected mesh nodes
 * - Receives LoRa messages and forwards to backend
 * - Manages gateway status and connection health
 */

const mqtt = require('mqtt');
const axios = require('axios');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const BACKEND_URL = process.env.BACKEND_URL || 'http://backend:3001';
const MQTT_URL = process.env.MQTT_URL || 'mqtt://mqtt:1883';
const SERIAL_PORTS = process.env.SERIAL_PORTS || '/dev/ttyUSB0:/dev/ttyUSB1';
const GATEWAY_ID = process.env.GATEWAY_ID || 'gateway-001';

class LoraGateway {
  constructor() {
    this.mqttClient = null;
    this.serialPorts = [];
    this.gatewayStatus = 'initializing';
    this.connectedNodes = new Map();
    this.messageQueue = [];
  }

  async initialize() {
    console.log('[LoraGateway] Initializing...');
    
    try {
      // Connect to MQTT
      await this.connectMQTT();
      
      // Connect to serial ports (USB LoRa devices)
      await this.connectSerialPorts();
      
      // Start monitoring broadcast endpoint
      this.startBroadcastMonitor();
      
      this.gatewayStatus = 'ready';
      console.log('[LoraGateway] ✅ Initialization complete');
    } catch (error) {
      console.error('[LoraGateway] ❌ Initialization failed:', error);
      this.gatewayStatus = 'error';
      setTimeout(() => this.initialize(), 5000); // Retry in 5 seconds
    }
  }

  async connectMQTT() {
    return new Promise((resolve, reject) => {
      console.log('[MQTT] Connecting to', MQTT_URL);
      
      this.mqttClient = mqtt.connect(MQTT_URL, {
        clientId: GATEWAY_ID,
        reconnectPeriod: 5000
      });

      this.mqttClient.on('connect', () => {
        console.log('[MQTT] ✅ Connected');
        
        // Subscribe to broadcast messages
        this.mqttClient.subscribe('lora/broadcast', (err) => {
          if (err) reject(err);
          else resolve();
        });
      });

      this.mqttClient.on('message', (topic, message) => {
        this.handleMQTTMessage(topic, message.toString());
      });

      this.mqttClient.on('error', (error) => {
        console.error('[MQTT] Error:', error);
        reject(error);
      });

      setTimeout(() => reject(new Error('MQTT connection timeout')), 10000);
    });
  }

  async connectSerialPorts() {
    console.log('[Serial] Connecting to LoRa devices...');
    
    const ports = SERIAL_PORTS.split(':').filter(p => p);
    
    for (const port of ports) {
      try {
        const serialPort = new SerialPort({
          path: port,
          baudRate: 115200,
          autoOpen: true
        });

        const parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));

        serialPort.on('open', () => {
          console.log(`[Serial] ✅ Connected to ${port}`);
        });

        parser.on('data', (line) => {
          this.handleSerialData(port, line);
        });

        serialPort.on('error', (error) => {
          console.error(`[Serial] Error on ${port}:`, error);
        });

        this.serialPorts.push({ path: port, port: serialPort });
      } catch (error) {
        console.error(`[Serial] Failed to connect to ${port}:`, error);
      }
    }

    // Wait for at least one port to connect
    if (this.serialPorts.length === 0) {
      throw new Error('No serial ports available');
    }
  }

  handleSerialData(port, line) {
    // Parse incoming LoRa messages
    if (line.includes('[LORA RX]')) {
      console.log(`[LoRa RX] ${port}: ${line}`);
      this.processIncomingMessage(line, port);
    } else if (line.includes('[BROADCAST RX]')) {
      console.log(`[Broadcast RX] ${port}: ${line}`);
    }
  }

  handleMQTTMessage(topic, message) {
    console.log(`[MQTT Message] Topic: ${topic}`);
    
    if (topic === 'lora/broadcast') {
      // New broadcast from backend
      this.relayBroadcast(message);
    }
  }

  async relayBroadcast(message) {
    try {
      const data = JSON.parse(message);
      console.log(`[Relay] Broadcasting: ${data.content}`);
      
      // Format: BCAST;username;ttl;content
      const packet = `BCAST;${data.username};${data.ttl};${data.content}`;
      
      // Send to all connected nodes
      for (const { port } of this.serialPorts) {
        port.write(packet + '\n', (error) => {
          if (error) {
            console.error('[Serial TX] Error:', error);
          } else {
            console.log(`[LoRa TX] Broadcast sent to ${port.path}`);
          }
        });
      }
    } catch (error) {
      console.error('[Relay] Error:', error);
    }
  }

  async processIncomingMessage(line, port) {
    // Parse received LoRa message and send to backend
    try {
      // Extract node info from message
      const nodeMatch = line.match(/NodeID:(\w+)/);
      const signalMatch = line.match(/Signal:(-?\d+)/);
      
      if (nodeMatch) {
        const nodeId = nodeMatch[1];
        const signal = signalMatch ? parseInt(signalMatch[1]) : 0;
        
        // Update node in backend
        await axios.post(`${BACKEND_URL}/api/nodes/${nodeId}/update`, {
          lastSeen: new Date(),
          signalStrength: signal,
          gatewayId: GATEWAY_ID
        }).catch(err => console.error('[Backend] Update failed:', err.message));
      }
    } catch (error) {
      console.error('[Message Processing] Error:', error);
    }
  }

  startBroadcastMonitor() {
    // Poll backend for pending broadcasts
    setInterval(async () => {
      try {
        const response = await axios.get(`${BACKEND_URL}/api/broadcasts`);
        const broadcasts = response.data.broadcasts || response.data;
        
        for (const broadcast of broadcasts) {
          if (!broadcast.deliveredTo) {
            broadcast.deliveredTo = [];
          }
          
          if (!broadcast.deliveredTo.includes(GATEWAY_ID)) {
            console.log(`[Monitor] New broadcast: ${broadcast.content}`);
            await this.relayBroadcast(JSON.stringify({
              username: broadcast.username,
              ttl: broadcast.ttl,
              content: broadcast.content
            }));
            
            // Mark as delivered
            broadcast.deliveredTo.push(GATEWAY_ID);
          }
        }
      } catch (error) {
        console.error('[Monitor] Error checking broadcasts:', error.message);
      }
    }, 10000); // Check every 10 seconds
  }

  getStatus() {
    return {
      gatewayId: GATEWAY_ID,
      status: this.gatewayStatus,
      connectedPorts: this.serialPorts.length,
      connectedNodes: this.connectedNodes.size,
      messageQueue: this.messageQueue.length,
      timestamp: new Date()
    };
  }
}

// Initialize gateway
const gateway = new LoraGateway();
gateway.initialize();

// Health check endpoint (if needed for debugging)
if (process.env.HEALTH_PORT) {
  const http = require('http');
  http.createServer((req, res) => {
    if (req.url === '/health') {
      res.writeHead(200);
      res.end(JSON.stringify(gateway.getStatus()));
    } else {
      res.writeHead(404);
      res.end('Not Found');
    }
  }).listen(process.env.HEALTH_PORT || 3002);
  console.log(`[Health] Listening on port ${process.env.HEALTH_PORT || 3002}`);
}

console.log('[LoraGateway] Service initialized');
