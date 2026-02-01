// MeshNet Backend Server
// Main API server for RPI controller

const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const dotenv = require('dotenv');
const path = require('path');
const jwt = require('jsonwebtoken');
const bcryptjs = require('bcryptjs');
const { v4: uuidv4 } = require('uuid');
const axios = require('axios');
const crypto = require('crypto');

// Import routers
const nodeRouter = require('./node-router');
const nodesRouter = require('./modules/nodes/router');

// Load environment variables
dotenv.config();

// Initialize Express app
const app = express();
const PORT = process.env.PORT || 3001;
const JWT_SECRET = process.env.JWT_SECRET || 'meshnet-dev-secret-key-change-in-production';

function sha256Hex(value) {
  return crypto.createHash('sha256').update(value || '').digest('hex');
}

function extractMacFromNodeId(nodeId) {
  if (!nodeId) return null;
  const match = nodeId.match(/([0-9A-Fa-f]{12})/);
  if (!match) return null;
  return match[1].match(/.{1,2}/g).join(':').toUpperCase();
}

function extractVersionFromNodeId(nodeId) {
  if (!nodeId) return null;
  const match = nodeId.match(/_(\d+\.\d+\.\d+)$/);
  return match ? match[1] : null;
}

// ============ MIDDLEWARE ============
app.use(cors());
app.use(bodyParser.json({ limit: '10mb' }));
app.use(bodyParser.urlencoded({ limit: '10mb', extended: true }));

// Middleware for auth verification
const verifyToken = (req, res, next) => {
  const token = req.headers.authorization?.split(' ')[1];
  if (!token) return res.status(401).json({ error: 'No token provided' });
  
  try {
    const decoded = jwt.verify(token, JWT_SECRET);
    req.user = decoded;
    next();
  } catch (error) {
    res.status(401).json({ error: 'Invalid token' });
  }
};

// ============ DATABASE CONNECTION ============
const fs = require('fs');
const mysql = require('mysql2/promise');

let dbPool;
let dbReady = false;
let dbRetryTimer = null;

// Test hook (used by automated tests)
function setDbPoolForTests(pool) {
  dbPool = pool;
}

async function ensureDatabaseSchema() {
  if (!dbPool) return;

  const [tables] = await dbPool.query(
    "SELECT table_name FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = 'users'"
  );

  if (tables.length === 0) {
    console.log('⚠ Users table missing - creating schema');

    const sqlPath = path.join(__dirname, '..', '..', 'docker', 'mysql.sql');
    let statements = [];

    if (fs.existsSync(sqlPath)) {
      const rawSql = fs.readFileSync(sqlPath, 'utf8');
      const cleaned = rawSql
        .split('\n')
        .filter(line => !line.trim().startsWith('--'))
        .join('\n');

      statements = cleaned
        .split(';')
        .map(stmt => stmt.trim())
        .filter(stmt => stmt.length > 0);
    } else {
      statements = [
        `CREATE TABLE IF NOT EXISTS \`groups\` (
          id INT AUTO_INCREMENT PRIMARY KEY,
          groupId VARCHAR(64) UNIQUE NOT NULL,
          name VARCHAR(128) NOT NULL,
          description TEXT,
          permissions JSON,
          createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
          updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
        )`,
        `CREATE TABLE IF NOT EXISTS users (
          id INT AUTO_INCREMENT PRIMARY KEY,
          userId VARCHAR(64) UNIQUE NOT NULL,
          username VARCHAR(64) UNIQUE NOT NULL,
          email VARCHAR(255) UNIQUE NOT NULL,
          passwordHash VARCHAR(255) NOT NULL,
          passwordSha256 VARCHAR(64) NOT NULL DEFAULT '',
          groupId INT NOT NULL,
          isActive BOOLEAN DEFAULT TRUE,
          createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
          updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
          FOREIGN KEY (groupId) REFERENCES \`groups\`(id),
          INDEX idx_groupId (groupId)
        )`
      ];
    }

    for (const statement of statements) {
      await dbPool.query(statement);
    }
  }

  await dbPool.query(
    `CREATE TABLE IF NOT EXISTS nodes (
      id INT AUTO_INCREMENT PRIMARY KEY,
      nodeId VARCHAR(64) UNIQUE NOT NULL,
      macAddress VARCHAR(17) UNIQUE NOT NULL,
      functionalName VARCHAR(32),
      version VARCHAR(16),
      lastSeen TIMESTAMP,
      signalStrength INT,
      battery INT,
      connectedNodes INT DEFAULT 0,
      isActive BOOLEAN DEFAULT TRUE,
      createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
      updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      INDEX idx_nodeId (nodeId),
      INDEX idx_isActive (isActive)
    )`
  );

  await dbPool.query(
    `INSERT IGNORE INTO \`groups\` (groupId, name, description, permissions)
     VALUES ('default', 'Default Group', 'Default user group', JSON_ARRAY())`
  );
  await dbPool.query(
    `INSERT IGNORE INTO \`groups\` (groupId, name, description, permissions)
     VALUES ('admin', 'Administrator', 'Administrator group with full access', JSON_ARRAY('view_dashboard','view_nodes','edit_nodes','edit_pages','manage_users','manage_groups','view_logs','system_admin','view_users','send_broadcast'))`
  );

  await dbPool.query(
    `UPDATE \`groups\` SET permissions = JSON_ARRAY() WHERE groupId = 'default'`
  );
  await dbPool.query(
    `UPDATE \`groups\` SET permissions = JSON_ARRAY('view_dashboard','view_nodes','edit_nodes','edit_pages','manage_users','manage_groups','view_logs','system_admin','view_users','send_broadcast')
     WHERE groupId = 'admin'`
  );
  const adminPassword = 'admin123';
  const adminPasswordHash = await bcryptjs.hash(adminPassword, 10);
  const adminPasswordSha = sha256Hex(adminPassword);

  await dbPool.query(
    `INSERT INTO users (userId, username, email, passwordHash, passwordSha256, groupId, isActive)
     VALUES ('admin-user-001', 'admin', 'admin@meshnet.local', ?, ?,
             (SELECT id FROM \`groups\` WHERE groupId='admin'), TRUE)
     ON DUPLICATE KEY UPDATE
       passwordHash = VALUES(passwordHash),
       passwordSha256 = VALUES(passwordSha256),
       isActive = TRUE`,
    [adminPasswordHash, adminPasswordSha]
  );
}

async function initializeDatabase() {
  try {
    console.log(`Connecting to database: ${process.env.DB_HOST}:${process.env.DB_PORT}`);

    dbPool = mysql.createPool({
      host: process.env.DB_HOST || 'mysql',
      port: process.env.DB_PORT || 3306,
      user: process.env.DB_USER || 'meshnet',
      password: process.env.DB_PASSWORD || 'meshnet_secure_pwd',
      database: process.env.DB_NAME || 'meshnet',
      waitForConnections: true,
      connectionLimit: 10,
      queueLimit: 0,
      enableKeepAlive: true,
      connectTimeout: 10000
    });

    // Test connection
    const connection = await dbPool.getConnection();
    connection.release();

    console.log('✓ Database connection pool created and tested');
    await ensureDatabaseSchema();
    dbReady = true;
    return true;
  } catch (error) {
    dbReady = false;
    dbPool = null;
    console.error('✗ Database connection failed:', error.message);
    // Don't exit - continue with placeholder
    console.log('⚠ Backend running without database (Phase 2 testing)');
    return false;
  }
}

// ============ API ROUTES ============

// Health check
app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

// Node Web Hosting Router (no auth required for node access)
app.use('/api/host', nodeRouter);

// Nodes Module Router
app.use('/api/nodes', nodesRouter);

// Nodes Dashboard
const nodeConnections = require('./modules/nodes/node-connections');
app.get('/dashboard/nodes', (req, res) => {
  nodeConnections.displayNodeConnectionsDashboard(req, res);
});

// Nodes API
app.get('/api/nodes', async (req, res) => {
  try {
    if (!dbPool) return res.json([]);
    const [nodes] = await dbPool.query('SELECT * FROM nodes WHERE isActive = true');
    res.json(nodes);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// ===== Users API =====
app.get('/api/users', async (req, res) => {
  try {
    if (!dbPool) return res.json([]);
    const [users] = await dbPool.query(
      'SELECT u.userId, u.username, u.email, u.groupId, u.isActive, u.createdAt, u.passwordSha256, g.name AS team FROM users u LEFT JOIN `groups` g ON u.groupId = g.id ORDER BY u.username'
    );
    res.json(users);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get('/api/users/:userId', async (req, res) => {
  try {
    if (!dbPool) return res.status(404).json({ error: 'Database not available' });
    const [users] = await dbPool.query('SELECT * FROM users WHERE userId = ?', [req.params.userId]);
    if (!users.length) return res.status(404).json({ error: 'User not found' });
    res.json(users[0]);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/users', async (req, res) => {
  try {
    if (!dbPool) return res.status(503).json({ error: 'Database not available' });
    const { username, password, email, groupId } = req.body;
    if (!username || !password || !groupId) return res.status(400).json({ error: 'username, password, groupId required' });

    const userId = uuidv4();
    const userEmail = email || `${username}@meshnet.local`;
    const passwordHash = await bcryptjs.hash(password, 10);
    const passwordSha256 = sha256Hex(password);

    await dbPool.query(
      'INSERT INTO users (userId, username, email, passwordHash, passwordSha256, groupId, isActive) VALUES (?, ?, ?, ?, ?, ?, true)',
      [userId, username, userEmail, passwordHash, passwordSha256, parseInt(groupId, 10)]
    );

    res.json({ success: true, userId });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.put('/api/users/:userId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    const { username, email, password, groupId, isActive } = req.body;

    let passwordHash = null;
    let passwordSha256 = null;
    if (password) {
      passwordHash = await bcryptjs.hash(password, 10);
      passwordSha256 = sha256Hex(password);
    }

    await dbPool.query(
      'UPDATE users SET username = COALESCE(?, username), email = COALESCE(?, email), passwordHash = COALESCE(?, passwordHash), passwordSha256 = COALESCE(?, passwordSha256), groupId = COALESCE(?, groupId), isActive = COALESCE(?, isActive) WHERE userId = ?',
      [username || null, email || null, passwordHash, passwordSha256, groupId ? parseInt(groupId, 10) : null, isActive !== undefined ? isActive : null, req.params.userId]
    );

    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.delete('/api/users/:userId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    await dbPool.query('DELETE FROM users WHERE userId = ?', [req.params.userId]);
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// ===== Auth API (basic username/password) =====
app.post('/api/auth/login', async (req, res) => {
  try {
    if (!dbPool) return res.status(503).json({ error: 'Database not available' });
    const { username, password } = req.body;
    if (!username || !password) return res.status(400).json({ error: 'Username and password required' });

    const [users] = await dbPool.query('SELECT * FROM users WHERE username = ? AND isActive = true', [username]);
    if (!users.length) return res.status(401).json({ error: 'Invalid credentials' });

    const user = users[0];
    const ok = await bcryptjs.compare(password, user.passwordHash);
    if (!ok) return res.status(401).json({ error: 'Invalid credentials' });

    if (!user.passwordSha256) {
      await dbPool.query('UPDATE users SET passwordSha256 = ? WHERE userId = ?', [sha256Hex(password), user.userId]);
    }

    res.json({
      success: true,
      user: { userId: user.userId, username: user.username, email: user.email }
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.delete('/api/auth/logout/:userId', async (req, res) => {
  res.json({ success: true });
});

// ===== Sync API for nodes =====
app.get('/api/sync/users', async (req, res) => {
  try {
    if (!dbPool) return res.json({ status: 'success', user_count: 0, users: [] });
    const [users] = await dbPool.query(
      'SELECT u.username, u.passwordSha256 AS password_hash, g.name AS team FROM users u LEFT JOIN `groups` g ON u.groupId = g.id WHERE u.isActive = true'
    );
    res.json({ status: 'success', user_count: users.length, users });
  } catch (error) {
    res.status(500).json({ status: 'error', message: error.message });
  }
});

app.get('/api/sync/pages', async (req, res) => {
  try {
    if (!dbPool) return res.json({ status: 'success', page_count: 0, pages: [] });
    const [pages] = await dbPool.query(
      'SELECT g.name AS team, p.content AS html FROM pages p LEFT JOIN `groups` g ON p.groupId = g.id WHERE p.isActive = true'
    );
    res.json({ status: 'success', page_count: pages.length, pages });
  } catch (error) {
    res.status(500).json({ status: 'error', message: error.message });
  }
});

// ===== ACK/Ping/Message API =====
app.post('/api/acks', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    const { msgId, nodeId, object, function: func, timestamp } = req.body;
    if (!msgId || !nodeId) return res.status(400).json({ error: 'msgId and nodeId required' });
    await dbPool.query(
      'INSERT INTO acks (msgId, nodeId, object, `function`, ackTimestamp) VALUES (?, ?, ?, ?, FROM_UNIXTIME(?/1000))',
      [msgId, nodeId, object || null, func || null, timestamp || 0]
    );
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/pings', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    const { nodeId, status, latencyMs } = req.body;
    if (!nodeId) return res.status(400).json({ error: 'nodeId required' });
    await dbPool.query(
      'INSERT INTO pings (nodeId, status, latencyMs) VALUES (?, ?, ?)',
      [nodeId, status || 'unknown', latencyMs || null]
    );
    let version = null;
    let signalStrength = null;
    try {
      const [rows] = await dbPool.query('SELECT version, signalStrength FROM nodes WHERE nodeId = ? LIMIT 1', [nodeId]);
      if (rows.length > 0) {
        version = rows[0].version || null;
        signalStrength = rows[0].signalStrength ?? null;
      }
    } catch (versionError) {
      console.warn('Ping version lookup failed:', versionError.message);
    }
    res.json({ success: true, version, signalStrength });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/messages', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    const { nodeId, username, team, object, function: func, parameters, timestamp } = req.body;
    if (!nodeId) return res.status(400).json({ error: 'nodeId required' });
    await dbPool.query(
      'INSERT INTO message_log (messageId, messageType, fromNodeId, toNodeId, status, hops, timestamp) VALUES (?, ?, ?, ?, ?, ?, ?)',
      [uuidv4(), object || null, nodeId, null, func || null, 0, timestamp || Date.now()]
    );
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get('/api/nodes/:nodeId', async (req, res) => {
  try {
    if (!dbPool) return res.status(404).json({ error: 'Database not available' });
    const [nodes] = await dbPool.query(
      'SELECT * FROM nodes WHERE nodeId = ?',
      [req.params.nodeId]
    );
    
    if (nodes.length === 0) {
      return res.status(404).json({ error: 'Node not found' });
    }
    
    res.json(nodes[0]);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/nodes/:nodeId/update', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, status: 'updated', warning: 'Database not available' });
    
    const { functionalName, version, battery, signalStrength } = req.body;
    
    const query = 'UPDATE nodes SET functionalName = ?, version = ?, battery = ?, `signalStrength` = ?, lastSeen = NOW() WHERE nodeId = ?';
    
    await dbPool.query(query, [
      functionalName,
      version,
      battery,
      signalStrength,
      req.params.nodeId
    ]);
    
    res.json({ success: true, status: 'updated' });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Create node
app.post('/api/nodes', async (req, res) => {
  try {
    if (!dbPool) return res.status(503).json({ error: 'Database not available' });
    
    const { nodeId, macAddress, functionalName, version } = req.body;
    const newNodeId = nodeId || uuidv4();
    const newMacAddress = macAddress || `00:00:00:00:00:${Math.random().toString(16).substr(2,2)}`;
    
    const query = 'INSERT INTO nodes (nodeId, macAddress, functionalName, version, isActive, battery, `signalStrength`, lastSeen) VALUES (?, ?, ?, ?, true, 100, 0, NOW())';
    
    await dbPool.query(query, [
      newNodeId,
      newMacAddress,
      functionalName || 'Unnamed Node',
      version || '1.0'
    ]);
    
    res.json({ success: true, nodeId: newNodeId, status: 'created' });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Register or update node by nodeId
app.post('/api/nodes/register', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    const { nodeId, functionalName, version } = req.body;
    if (!nodeId) return res.status(400).json({ error: 'nodeId required' });
    const macAddress = extractMacFromNodeId(nodeId) || nodeId.substring(0, 17);
    const resolvedVersion = version || extractVersionFromNodeId(nodeId) || null;
    await dbPool.query(
      'INSERT INTO nodes (nodeId, macAddress, functionalName, version, isActive, battery, `signalStrength`, lastSeen) VALUES (?, ?, ?, ?, true, 100, 0, NOW()) ON DUPLICATE KEY UPDATE functionalName = COALESCE(?, functionalName), version = COALESCE(?, version), lastSeen = NOW()',
      [nodeId, macAddress, functionalName || nodeId, resolvedVersion, functionalName || null, resolvedVersion]
    );
    res.json({ success: true, status: 'registered' });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Update node
app.put('/api/nodes/:nodeId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, status: 'updated', warning: 'Database not available' });
    
    const { functionalName, battery, signalStrength } = req.body;
    
    const query = 'UPDATE nodes SET functionalName = COALESCE(?, functionalName), battery = COALESCE(?, battery), `signalStrength` = COALESCE(?, `signalStrength`), lastSeen = NOW() WHERE nodeId = ?';
    
    await dbPool.query(query, [
      functionalName || null,
      battery !== undefined ? battery : null,
      signalStrength !== undefined ? signalStrength : null,
      req.params.nodeId
    ]);
    
    res.json({ success: true, status: 'updated' });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Delete node
app.delete('/api/nodes/:nodeId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    
    await dbPool.query('DELETE FROM nodes WHERE nodeId = ?', [req.params.nodeId]);
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Users API
app.get('/api/users', async (req, res) => {
  try {
    if (!dbPool) return res.json([]);
    const [users] = await dbPool.query(
      'SELECT id, userId, username, email, groupId, isActive, createdAt FROM users'
    );
    res.json(users);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/users', async (req, res) => {
  try {
    if (!dbPool) return res.status(503).json({ error: 'Database not available' });
    
    const { username, email, password, groupId } = req.body;
    const userId = uuidv4();
    const passwordHash = await bcryptjs.hash(password, 10);
    const passwordSha256 = sha256Hex(password);
    
    await dbPool.query(
      'INSERT INTO users (userId, username, email, passwordHash, passwordSha256, groupId) VALUES (?, ?, ?, ?, ?, ?)',
      [userId, username, email, passwordHash, passwordSha256, groupId]
    );
    
    res.json({ status: 'created', userId });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get user
app.get('/api/users/:userId', async (req, res) => {
  try {
    if (!dbPool) return res.status(404).json({ error: 'Database not available' });
    const [users] = await dbPool.query(
      'SELECT id, userId, username, email, groupId, isActive, createdAt FROM users WHERE userId = ?',
      [req.params.userId]
    );
    
    if (users.length === 0) return res.status(404).json({ error: 'User not found' });
    res.json(users[0]);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Update user
app.put('/api/users/:userId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    
    const { username, email, password, groupId, isActive } = req.body;
    
    if (password) {
      const passwordHash = await bcryptjs.hash(password, 10);
      const passwordSha256 = sha256Hex(password);
      await dbPool.query(
        'UPDATE users SET username = COALESCE(?, username), email = COALESCE(?, email), passwordHash = ?, passwordSha256 = ?, groupId = COALESCE(?, groupId), isActive = COALESCE(?, isActive) WHERE userId = ?',
        [username || null, email || null, passwordHash, passwordSha256, groupId || null, isActive !== undefined ? isActive : null, req.params.userId]
      );
    } else {
      await dbPool.query(
        'UPDATE users SET username = COALESCE(?, username), email = COALESCE(?, email), groupId = COALESCE(?, groupId), isActive = COALESCE(?, isActive) WHERE userId = ?',
        [username || null, email || null, groupId || null, isActive !== undefined ? isActive : null, req.params.userId]
      );
    }
    
    res.json({ success: true, status: 'updated' });
  } catch (error) {
    res.status(500).json({ success: false, error: error.message });
  }
});

// Delete user
app.delete('/api/users/:userId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    
    await dbPool.query('DELETE FROM users WHERE userId = ?', [req.params.userId]);
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Sync users for nodes
app.get('/api/sync/users', async (req, res) => {
  try {
    if (!dbPool) return res.json({ status: 'success', user_count: 0, users: [] });
    const [rows] = await dbPool.query(
      'SELECT u.username, g.name AS team, u.passwordSha256 '
      + 'FROM users u '
      + 'JOIN `groups` g ON u.groupId = g.id '
      + 'WHERE u.isActive = true '
      + 'ORDER BY u.username'
    );
    const users = rows.map(r => ({
      username: r.username,
      team: r.team,
      password_hash: r.passwordSha256 || ''
    }));
    res.json({ status: 'success', user_count: users.length, users });
  } catch (error) {
    res.status(500).json({ status: 'error', message: error.message });
  }
});

// Sync pages for nodes
app.get('/api/sync/pages', async (req, res) => {
  try {
    if (!dbPool) return res.json({ status: 'success', page_count: 0, pages: [] });
    const [rows] = await dbPool.query(
      'SELECT g.name AS team, p.content AS html '
      + 'FROM pages p '
      + 'JOIN `groups` g ON p.groupId = g.id '
      + 'WHERE p.isActive = true '
      + 'ORDER BY g.name'
    );
    const pages = rows.map(r => ({ team: r.team, html: r.html || '' }));
    res.json({ status: 'success', page_count: pages.length, pages });
  } catch (error) {
    res.status(500).json({ status: 'error', message: error.message });
  }
});

// Store ACKs from nodes
app.post('/api/acks', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    const { msgId, nodeId, object, function: func, timestamp } = req.body;
    if (!msgId || !nodeId) return res.status(400).json({ error: 'msgId and nodeId required' });
    await dbPool.query(
      'INSERT INTO acks (msgId, nodeId, object, `function`, ackTimestamp) VALUES (?, ?, ?, ?, FROM_UNIXTIME(?/1000))',
      [msgId, nodeId, object || '', func || '', timestamp || 0]
    );
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Store ping results from nodes
app.post('/api/pings', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    const { nodeId, status, latencyMs } = req.body;
    if (!nodeId) return res.status(400).json({ error: 'nodeId required' });
    await dbPool.query(
      'INSERT INTO pings (nodeId, status, latencyMs) VALUES (?, ?, ?)',
      [nodeId, status || 'unknown', latencyMs || null]
    );
    let version = null;
    try {
      const [rows] = await dbPool.query('SELECT version FROM nodes WHERE nodeId = ? LIMIT 1', [nodeId]);
      if (rows.length > 0) {
        version = rows[0].version || null;
      }
    } catch (versionError) {
      console.warn('Ping version lookup failed:', versionError.message);
    }
    res.json({ success: true, version });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Authentication
app.post('/api/auth/login', async (req, res) => {
  try {
    if (!dbPool) return res.status(503).json({ error: 'Database not available' });
    
    const { username, password } = req.body;
    
    const [users] = await dbPool.query(
      'SELECT userId, username, email, groupId, passwordHash FROM users WHERE username = ?',
      [username]
    );
    
    if (users.length === 0) {
      return res.status(401).json({ success: false, error: 'Invalid credentials' });
    }
    
    const user = users[0];
    const passwordMatch = await bcryptjs.compare(password, user.passwordHash);
    
    if (!passwordMatch) {
      return res.status(401).json({ success: false, error: 'Invalid credentials' });
    }

    // Create session token
    const sessionId = require('crypto').randomBytes(32).toString('hex');
    const token = jwt.sign({ userId: user.userId, username: user.username, sessionId }, JWT_SECRET, { expiresIn: '24h' });
    
    // Store session in database
    await dbPool.query(
      'INSERT INTO user_sessions (sessionId, userId, token, expiresAt) VALUES (?, ?, ?, DATE_ADD(NOW(), INTERVAL 24 HOUR))',
      [sessionId, user.userId, token]
    );

    res.json({ 
      success: true,
      token, 
      sessionId,
      user: {
        userId: user.userId,
        username: user.username,
        email: user.email
      }
    });
  } catch (error) {
    console.error('Login error:', error);
    res.status(500).json({ success: false, error: error.message });
  }
});

// Logout endpoint - delete session
app.delete('/api/auth/logout/:userId', async (req, res) => {
  try {
    if (!dbPool) return res.status(503).json({ error: 'Database not available' });
    
    const { userId } = req.params;
    
    // Delete all active sessions for this user
    await dbPool.query(
      'DELETE FROM user_sessions WHERE userId = ? AND expiresAt > NOW()',
      [userId]
    );

    res.json({ success: true, message: 'Session deleted successfully' });
  } catch (error) {
    console.error('Logout error:', error);
    res.status(500).json({ error: error.message });
  }
});

// Groups API
app.get('/api/groups', async (req, res) => {
  try {
    if (!dbPool) return res.json([]);
    const [groups] = await dbPool.query('SELECT * FROM `groups`');
    res.json(groups);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/groups', async (req, res) => {
  try {
    if (!dbPool) return res.status(503).json({ error: 'Database not available' });
    
    const { name, description, permissions } = req.body;
    const groupId = uuidv4();
    
    await dbPool.query(
      'INSERT INTO `groups` (groupId, name, description, permissions) VALUES (?, ?, ?, ?)',
      [groupId, name, description, JSON.stringify(permissions || [])]
    );
    
    res.json({ groupId, status: 'created' });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get group
app.get('/api/groups/:groupId', async (req, res) => {
  try {
    if (!dbPool) return res.status(404).json({ error: 'Database not available' });
    const [groups] = await dbPool.query(
      'SELECT * FROM `groups` WHERE id = ?',
      [req.params.groupId]
    );
    
    if (groups.length === 0) return res.status(404).json({ error: 'Group not found' });
    res.json(groups[0]);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Update group
app.put('/api/groups/:groupId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    
    const { name, description, permissions } = req.body;
    
    await dbPool.query(
      'UPDATE `groups` SET name = COALESCE(?, name), description = COALESCE(?, description), permissions = COALESCE(?, permissions) WHERE id = ?',
      [name || null, description || null, permissions ? JSON.stringify(permissions) : null, req.params.groupId]
    );
    
    res.json({ success: true, status: 'updated' });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Delete group
app.delete('/api/groups/:groupId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    
    await dbPool.query('DELETE FROM `groups` WHERE id = ?', [req.params.groupId]);
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Pages API
app.get('/api/pages', async (req, res) => {
  try {
    if (!dbPool) return res.json([]);
    const [pages] = await dbPool.query('SELECT * FROM pages WHERE isActive = true');
    res.json(pages);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get all pages (for management/editing)
app.get('/api/pages/all', async (req, res) => {
  try {
    if (!dbPool) return res.json([]);
    const [pages] = await dbPool.query('SELECT * FROM pages');
    res.json(pages);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get('/api/pages/:nodeId', async (req, res) => {
  try {
    if (!dbPool) return res.json([]);
    const [pages] = await dbPool.query(
      'SELECT * FROM pages WHERE nodeId = ? AND isActive = true',
      [req.params.nodeId]
    );
    res.json(pages);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/pages', async (req, res) => {
  try {
    if (!dbPool) return res.status(503).json({ error: 'Database not available' });
    
    let { nodeId, groupId, title, content, imageUrl, refreshInterval, isActive } = req.body;
    const pageId = uuidv4();
    
    // If no nodeId provided, get the first available node for broadcast
    if (!nodeId) {
      try {
        const [nodes] = await dbPool.query('SELECT nodeId FROM nodes LIMIT 1');
        if (nodes.length > 0) {
          nodeId = nodes[0].nodeId;
        } else {
          return res.status(400).json({ error: 'No nodes available for broadcast' });
        }
      } catch (e) {
        return res.status(400).json({ error: 'Could not find broadcast node' });
      }
    }
    
    const query = 'INSERT INTO pages (pageId, nodeId, groupId, title, content, imageUrl, refreshInterval, isActive) VALUES (?, ?, ?, ?, ?, ?, ?, ?)';
    
    await dbPool.query(query, [
      pageId,
      nodeId,
      groupId || null,
      title,
      content,
      imageUrl || null,
      refreshInterval || 30,
      isActive !== false
    ]);
    
    res.json({ pageId, status: 'created', nodeId: nodeId });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Update page
app.put('/api/pages/:pageId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ status: 'updated', warning: 'Database not available' });
    
    const { title, content, imageUrl, isActive } = req.body;
    
    await dbPool.query(
      'UPDATE pages SET title = COALESCE(?, title), content = COALESCE(?, content), imageUrl = COALESCE(?, imageUrl), isActive = COALESCE(?, isActive) WHERE pageId = ?',
      [title || null, content || null, imageUrl || null, isActive !== undefined ? isActive : null, req.params.pageId]
    );
    
    res.json({ status: 'updated' });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Delete page
app.delete('/api/pages/:pageId', async (req, res) => {
  try {
    if (!dbPool) return res.json({ success: true, warning: 'Database not available' });
    
    await dbPool.query('DELETE FROM pages WHERE pageId = ?', [req.params.pageId]);
    res.json({ success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Node connections/topology
app.get('/api/topology', async (req, res) => {
  try {
    if (!dbPool) return res.json([]);
    const [connections] = await dbPool.query(
      'SELECT nodeIdFrom, nodeIdTo, signalStrength, hops, lastSeen FROM node_connections'
    );
    res.json(connections);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// ============ BROADCAST API ============
// POST /api/broadcast - Create and send broadcast message to all nodes
app.post('/api/broadcast', async (req, res) => {
  try {
    const { username, title, content, targetNodes, ttl } = req.body;
    
    if (!username || !content) {
      return res.status(400).json({ error: 'username and content required' });
    }
    
    const broadcastId = uuidv4();
    const expiresAt = new Date(Date.now() + (ttl || 60) * 1000);
    
    if (!dbPool) {
      return res.status(503).json({ error: 'Database not available' });
    }
    
    const conn = await dbPool.getConnection();
    try {
      // Store broadcast in database
      await conn.execute(
        `INSERT INTO broadcasts (broadcastId, username, title, content, targetNodes, ttl, expiresAt, status) 
         VALUES (?, ?, ?, ?, ?, ?, ?, 'active')`,
        [broadcastId, username, title || 'Broadcast', content, targetNodes ? JSON.stringify(targetNodes) : null, ttl || 60, expiresAt]
      );
      
      // Get all active nodes
      const [nodes] = await conn.execute(`SELECT nodeId, macAddress FROM nodes WHERE isActive = TRUE`);
      
      console.log(`[BROADCAST] Created broadcast: ${broadcastId} | User: ${username} | Nodes: ${nodes.length}`);
      
      // Relay broadcast to LoraGateway
      const broadcastPayload = {
        broadcastId,
        username,
        title: title || 'Broadcast',
        content,
        ttl: ttl || 60,
        targetNodes: targetNodes || nodes.map(n => n.nodeId)
      };
      
      try {
        const loraResponse = await axios.post('http://lora-gateway:3002/relay/broadcast', broadcastPayload, {
          timeout: 5000
        });
        console.log(`[BROADCAST] Relayed to LoraGateway: ${loraResponse.data.message}`);
      } catch (loraError) {
        console.warn(`[BROADCAST] LoraGateway relay failed (non-blocking): ${loraError.message}`);
        // Don't fail the request - relay might not be available but broadcast is stored
      }
      
      res.status(201).json({
        success: true,
        broadcastId,
        message: 'Broadcast created and relayed to nodes',
        nodeCount: nodes.length,
        expiresAt
      });
    } finally {
      conn.release();
    }
  } catch (error) {
    console.error('[BROADCAST ERROR]', error);
    res.status(500).json({ error: error.message });
  }
});

// GET /api/broadcasts - Get active broadcasts
app.get('/api/broadcasts', async (req, res) => {
  try {
    if (!dbPool) {
      return res.json({ broadcasts: [], error: 'Database not available' });
    }
    
    const conn = await dbPool.getConnection();
    try {
      const [broadcasts] = await conn.execute(
        `SELECT broadcastId, username, title, content, ttl, createdAt, expiresAt, status 
         FROM broadcasts 
         WHERE status = 'active' AND expiresAt > NOW() 
         ORDER BY createdAt DESC 
         LIMIT 50`
      );
      
      res.json({ broadcasts, count: broadcasts.length });
    } finally {
      conn.release();
    }
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// ============ ERROR HANDLING ============
app.use((err, req, res, next) => {
  console.error('Error:', err);
  res.status(500).json({ error: 'Internal server error' });
});

// ============ SERVER START ============
async function startServer() {
  try {
    const initialized = await initializeDatabase();

    if (!initialized && !dbRetryTimer) {
      dbRetryTimer = setInterval(async () => {
        if (dbReady) {
          clearInterval(dbRetryTimer);
          dbRetryTimer = null;
          return;
        }
        const ok = await initializeDatabase();
        if (ok) {
          clearInterval(dbRetryTimer);
          dbRetryTimer = null;
        }
      }, 10000);
    }

    // Cleanup old ACKs every hour
    setInterval(async () => {
      if (!dbPool) return;
      try {
        await dbPool.query('DELETE FROM acks WHERE ackTimestamp < (NOW() - INTERVAL 2 HOUR)');
      } catch (error) {
        console.error('[ACK CLEANUP] Error:', error.message);
      }
    }, 60 * 60 * 1000);
    
    app.listen(PORT, '0.0.0.0', () => {
      console.log(`\n╔════════════════════════════════════╗`);
      console.log(`║  MeshNet Backend API Server        ║`);
      console.log(`║  Running on port ${PORT}               ║`);
      console.log(`║  Environment: ${process.env.NODE_ENV || 'development'}        ║`);
      console.log(`╚════════════════════════════════════╝\n`);
    });
  } catch (error) {
    console.error('Failed to start server:', error);
    // Continue anyway - backend will run in placeholder mode
    app.listen(PORT, '0.0.0.0', () => {
      console.log(`Backend listening on port ${PORT} (db error, placeholder mode)`);
    });
  }
}

// Start the server if this is the main module
if (require.main === module) {
  startServer();
}

module.exports = app;
module.exports.setDbPoolForTests = setDbPoolForTests;
