/**
 * Node Connections Module
 * Handles tracking of team/user connections to nodes
 */

const express = require('express');
const mysql = require('mysql2/promise');
const { v4: uuidv4 } = require('uuid');

let dbPool;
let forceNoDb = false;

// Test hook
function setDbPoolForTests(pool, disableDb = false) {
  dbPool = pool;
  forceNoDb = disableDb;
}

/**
 * Initialize database pool for node connections
 */
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

/**
 * Log a user/team connection to a node
 * POST /api/nodes/connection
 */
async function logConnection(req, res) {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    const { nodeId, username, teamName } = req.body;

    if (!username || !teamName || !nodeId) {
      return res.status(400).json({ error: 'Missing required fields: nodeId, username, teamName' });
    }

    const connectionId = uuidv4();

    // Auto-create node if it doesn't exist
    await pool.query(
      `INSERT IGNORE INTO nodes (nodeId, macAddress, functionalName, isActive)
       VALUES (?, ?, ?, true)`,
      [nodeId, nodeId, nodeId]
    );

    // Log the connection
    await pool.query(
      `INSERT INTO node_connections (connectionId, nodeId, username, teamName)
       VALUES (?, ?, ?, ?)`,
      [connectionId, nodeId, username, teamName]
    );

    res.json({ 
      status: 'logged', 
      connectionId,
      timestamp: new Date().toISOString() 
    });
  } catch (error) {
    console.error('Error logging connection:', error);
    res.status(500).json({ error: error.message });
  }
}

/**
 * Get all team connections for a specific node
 * GET /api/nodes/:nodeId/connections
 */
async function getNodeConnections(req, res) {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    const { nodeId } = req.params;

    const [connections] = await pool.query(
      `SELECT DISTINCT teamName, username, MAX(connectedAt) as firstSeen, MAX(lastSeen) as lastSeen
       FROM node_connections
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
    console.error('Error fetching connections:', error);
    res.status(500).json({ error: error.message });
  }
}

/**
 * Get connection statistics
 * GET /api/nodes/stats/connections
 */
async function getConnectionStats(req, res) {
  try {
    const pool = await initDbPool();
    
    if (!pool) {
      return res.status(503).json({ error: 'Database not available' });
    }

    // Total unique connections
    const [totalConnections] = await pool.query(
      `SELECT COUNT(DISTINCT connectionId) as total FROM node_connections`
    );

    // Unique teams
    const [uniqueTeams] = await pool.query(
      `SELECT COUNT(DISTINCT teamName) as total FROM node_connections`
    );

    // Connections per node
    const [perNode] = await pool.query(
      `SELECT nodeId, COUNT(*) as connectionCount, COUNT(DISTINCT teamName) as uniqueTeams
       FROM node_connections
       GROUP BY nodeId
       ORDER BY connectionCount DESC`
    );

    res.json({
      totalConnections: totalConnections[0].total,
      uniqueTeams: uniqueTeams[0].total,
      connectionsPerNode: perNode
    });
  } catch (error) {
    console.error('Error fetching stats:', error);
    res.status(500).json({ error: error.message });
  }
}

/**
 * Escape HTML for safe display
 */
function escapeHtml(text) {
  if (!text) return '';
  const map = {
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;',
    "'": '&#039;'
  };
  return text.replace(/[&<>"']/g, m => map[m]);
}

/**
 * Display HTML dashboard of node connections
 * GET /dashboard/nodes
 */
async function displayNodeConnectionsDashboard(req, res) {
  try {
    if (!dbPool) {
      const pool = await initDbPool();
      if (!pool) {
        return res.send('<h2>Database not available</h2>');
      }
    }

    // Get all active nodes with their connections
    const [nodes] = await dbPool.query('SELECT * FROM nodes WHERE isActive = true ORDER BY functionalName');
    
    let html = `
    <!DOCTYPE html>
    <html>
    <head>
      <title>Node Team Connections Dashboard</title>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <style>
        * { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 0; }
        body { 
          background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
          min-height: 100vh; 
          padding: 20px;
        }
        .container { max-width: 1400px; margin: 0 auto; }
        h1 { 
          color: white; 
          text-align: center; 
          margin-bottom: 10px;
          font-size: 2.5em;
          text-shadow: 2px 2px 4px rgba(0,0,0,0.2);
        }
        .subtitle {
          color: rgba(255,255,255,0.8);
          text-align: center;
          margin-bottom: 30px;
          font-size: 1.1em;
        }
        .stats-bar {
          display: grid;
          grid-template-columns: repeat(3, 1fr);
          gap: 20px;
          margin-bottom: 30px;
        }
        .stat-card {
          background: white;
          padding: 20px;
          border-radius: 8px;
          box-shadow: 0 4px 6px rgba(0,0,0,0.1);
          text-align: center;
        }
        .stat-card h3 { color: #667eea; margin-bottom: 10px; }
        .stat-value { font-size: 2em; font-weight: bold; color: #764ba2; }
        .nodes-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(450px, 1fr)); gap: 20px; }
        .node-card { 
          background: white; 
          border-radius: 8px; 
          padding: 20px; 
          box-shadow: 0 4px 6px rgba(0,0,0,0.1);
          transition: transform 0.2s;
        }
        .node-card:hover { transform: translateY(-4px); box-shadow: 0 8px 12px rgba(0,0,0,0.15); }
        .node-card h3 { 
          color: #667eea; 
          margin-bottom: 15px; 
          border-bottom: 2px solid #667eea; 
          padding-bottom: 10px;
        }
        .node-id { color: #999; font-size: 0.9em; margin-bottom: 10px; }
        .connections-table { 
          width: 100%; 
          border-collapse: collapse; 
          margin-top: 10px; 
          font-size: 0.9em;
        }
        .connections-table th { 
          background: #f5f5f5; 
          padding: 8px; 
          text-align: left; 
          border-bottom: 2px solid #ddd; 
          font-weight: bold;
          color: #333;
        }
        .connections-table td { 
          padding: 8px; 
          border-bottom: 1px solid #eee; 
        }
        .connections-table tr:hover { background: #f9f9f9; }
        .team-badge { 
          display: inline-block; 
          background: #667eea; 
          color: white; 
          padding: 4px 8px; 
          border-radius: 4px; 
          font-size: 0.85em; 
          font-weight: bold;
        }
        .timestamp { color: #999; font-size: 0.85em; }
        .no-connections { color: #ccc; font-style: italic; padding: 15px; text-align: center; }
        .footer { 
          text-align: center; 
          color: rgba(255,255,255,0.8); 
          margin-top: 40px; 
          padding-top: 20px;
          border-top: 1px solid rgba(255,255,255,0.2);
        }
      </style>
    </head>
    <body>
      <div class="container">
        <h1>🌐 Node Connections Dashboard</h1>
        <p class="subtitle">Real-time tracking of team connections to nodes</p>
    `;

    // Get statistics
    let totalConnections = 0;
    let uniqueTeams = new Set();

    // Process each node
    for (const node of nodes) {
      const [connections] = await dbPool.query(
        `SELECT DISTINCT teamName, username, MAX(connectedAt) as firstSeen, MAX(lastSeen) as lastSeen
         FROM node_connections
         WHERE nodeId = ?
         GROUP BY teamName, username
         ORDER BY lastSeen DESC`,
        [node.nodeId]
      );

      if (connections) {
        totalConnections += connections.length;
        connections.forEach(c => uniqueTeams.add(c.teamName));
      }
    }

    // Add statistics
    html += `
      <div class="stats-bar">
        <div class="stat-card">
          <h3>Active Nodes</h3>
          <div class="stat-value">${nodes.length}</div>
        </div>
        <div class="stat-card">
          <h3>Total Connections</h3>
          <div class="stat-value">${totalConnections}</div>
        </div>
        <div class="stat-card">
          <h3>Unique Teams</h3>
          <div class="stat-value">${uniqueTeams.size}</div>
        </div>
      </div>
      <div class="nodes-grid">
    `;

    // Add node cards
    for (const node of nodes) {
      const [connections] = await dbPool.query(
        `SELECT DISTINCT teamName, username, MAX(connectedAt) as firstSeen, MAX(lastSeen) as lastSeen
         FROM node_connections
         WHERE nodeId = ?
         GROUP BY teamName, username
         ORDER BY lastSeen DESC`,
        [node.nodeId]
      );

      html += `
        <div class="node-card">
          <h3>📍 ${escapeHtml(node.functionalName || node.nodeId)}</h3>
          <p class="node-id">ID: <code>${escapeHtml(node.nodeId)}</code></p>
      `;

      if (connections && connections.length > 0) {
        html += `
          <table class="connections-table">
            <thead>
              <tr>
                <th>Team</th>
                <th>User</th>
                <th>Last Connected</th>
              </tr>
            </thead>
            <tbody>
        `;
        
        for (const conn of connections) {
          const lastSeen = new Date(conn.lastSeen).toLocaleString();
          html += `
            <tr>
              <td><span class="team-badge">${escapeHtml(conn.teamName)}</span></td>
              <td>${escapeHtml(conn.username)}</td>
              <td><span class="timestamp">${lastSeen}</span></td>
            </tr>
          `;
        }
        
        html += `
            </tbody>
          </table>
        `;
      } else {
        html += `<p class="no-connections">No team connections recorded</p>`;
      }

      html += `</div>`;
    }

    html += `
      </div>
      <div class="footer">
        <p>Last updated: ${new Date().toLocaleString()}</p>
      </div>
    </div>
    </body>
    </html>
    `;

    res.send(html);
  } catch (error) {
    console.error('Error in dashboard:', error);
    res.status(500).send(`<h2>Error loading dashboard</h2><p>${error.message}</p>`);
  }
}

/**
 * Export module functions
 */
module.exports = {
  logConnection,
  getNodeConnections,
  getConnectionStats,
  displayNodeConnectionsDashboard,
  setDbPoolForTests
};
