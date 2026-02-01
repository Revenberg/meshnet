// Node Web Server Router
// Allows nodes to fetch and serve their web pages from the backend

const express = require('express');
const router = express.Router();
const mysql = require('mysql2/promise');
const { v4: uuidv4 } = require('uuid');

let dbPool;
let forceNoDb = false;

// Initialize database pool
async function initDbPool() {
  if (forceNoDb) {
    return null;
  }
  if (!dbPool) {
    dbPool = mysql.createPool({
      host: process.env.DB_HOST || 'mysql',
      port: process.env.DB_PORT || 3306,
      user: process.env.DB_USER || 'meshnet',
      password: process.env.DB_PASSWORD || 'meshnet_secure_pwd',
      database: process.env.DB_NAME || 'meshnet',
      waitForConnections: true,
      connectionLimit: 5,
      queueLimit: 0
    });
  }
  return dbPool;
}

// Test hook
function setDbPoolForTests(pool, disableDb = false) {
  dbPool = pool;
  forceNoDb = disableDb;
}

// Get all pages for a specific node
router.get('/node/:nodeId/pages', async (req, res) => {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    const { nodeId } = req.params;

    // Get pages for this node
    const [pages] = await pool.query(
      `SELECT pageId, title, content, imageUrl, refreshInterval, updatedAt 
       FROM pages 
       WHERE nodeId = ? AND isActive = true 
       ORDER BY updatedAt DESC`,
      [nodeId]
    );

    res.json({
      nodeId,
      pages: pages.map(page => ({
        pageId: page.pageId,
        title: page.title,
        hasContent: !!page.content,
        hasImage: !!page.imageUrl,
        refreshInterval: page.refreshInterval || 30,
        lastUpdated: page.updatedAt
      }))
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get specific page content for node
router.get('/node/:nodeId/pages/:pageId', async (req, res) => {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    const { nodeId, pageId } = req.params;

    // Get page content
    const [pages] = await pool.query(
      `SELECT pageId, nodeId, title, content, imageUrl, refreshInterval 
       FROM pages 
       WHERE pageId = ? AND nodeId = ? AND isActive = true`,
      [pageId, nodeId]
    );

    if (pages.length === 0) {
      return res.status(404).json({ error: 'Page not found' });
    }

    const page = pages[0];

    // Return HTML content that can be served by node
    res.setHeader('Content-Type', 'text/html; charset=utf-8');
    res.setHeader('Cache-Control', `public, max-age=${page.refreshInterval || 30}`);
    res.send(page.content);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get page content as JSON (for nodes that want to format it themselves)
router.get('/node/:nodeId/pages/:pageId/json', async (req, res) => {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    const { nodeId, pageId } = req.params;

    const [pages] = await pool.query(
      `SELECT pageId, nodeId, title, content, imageUrl, refreshInterval 
       FROM pages 
       WHERE pageId = ? AND nodeId = ? AND isActive = true`,
      [pageId, nodeId]
    );

    if (pages.length === 0) {
      return res.status(404).json({ error: 'Page not found' });
    }

    res.json(pages[0]);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get node info
router.get('/node/:nodeId/info', async (req, res) => {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    const { nodeId } = req.params;

    const [nodes] = await pool.query(
      `SELECT nodeId, functionalName, macAddress, version, signalStrength, battery, 
              connectedNodes, isActive, createdAt, updatedAt 
       FROM nodes 
       WHERE nodeId = ?`,
      [nodeId]
    );

    if (nodes.length === 0) {
      return res.status(404).json({ error: 'Node not found' });
    }

    res.json(nodes[0]);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Node heartbeat / status update
router.post('/node/:nodeId/heartbeat', async (req, res) => {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    const { nodeId } = req.params;
    const { signalStrength, battery, connectedNodes } = req.body;

    await pool.query(
      `UPDATE nodes 
       SET lastSeen = NOW(), 
           signalStrength = COALESCE(?, signalStrength),
           battery = COALESCE(?, battery),
           connectedNodes = COALESCE(?, connectedNodes),
           isActive = true
       WHERE nodeId = ?`,
      [signalStrength || null, battery || null, connectedNodes || null, nodeId]
    );

    res.json({ status: 'updated', timestamp: new Date().toISOString() });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Log user/team connection to a node
router.post('/node/:nodeId/connection', async (req, res) => {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    const { nodeId } = req.params;
    const { username, teamName } = req.body;

    if (!username || !teamName || !nodeId) {
      return res.status(400).json({ error: 'Missing required fields: nodeId, username, teamName' });
    }

    const connectionId = uuidv4();
    
    // First, ensure the node exists in the database
    // Use the nodeId as macAddress if nodeId looks like a MAC address, otherwise generate a dummy one
    const macAddress = nodeId.match(/^([0-9A-F]{2}:){5}([0-9A-F]{2})$/i) ? nodeId : `00:00:00:00:00:${nodeId.substring(0, 2)}`;
    
    try {
      await pool.query(
        `INSERT INTO nodes (nodeId, macAddress, isActive, createdAt, updatedAt)
         VALUES (?, ?, true, NOW(), NOW())
         ON DUPLICATE KEY UPDATE isActive = true, updatedAt = NOW()`,
        [nodeId, macAddress]
      );
    } catch (insertError) {
      console.error('[NODE_CONNECTION] Warning: Could not create/update node:', insertError.message);
      // Continue anyway - the connection might still work if node already exists
    }

    // Then log the connection
    await pool.query(
      `INSERT INTO node_user_connections (connectionId, nodeId, username, teamName, connectedAt, lastSeen)
       VALUES (?, ?, ?, ?, NOW(), NOW())`,
      [connectionId, nodeId, username, teamName]
    );

    res.json({ 
      status: 'logged', 
      connectionId,
      timestamp: new Date().toISOString() 
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get all team connections for a node
router.get('/node/:nodeId/connections', async (req, res) => {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    const { nodeId } = req.params;

    const [connections] = await pool.query(
      `SELECT DISTINCT teamName, username, MAX(connectedAt) as firstSeen, MAX(lastSeen) as lastSeen
       FROM node_user_connections
       WHERE nodeId = ?
       GROUP BY teamName, username
       ORDER BY lastSeen DESC`,
      [nodeId]
    );

    res.json({
      nodeId,
      connections: connections || []
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

router.setDbPoolForTests = setDbPoolForTests;
module.exports = router;
