// MeshNet Webserver
// Dashboard and management interface

const express = require('express');
const axios = require('axios');
const path = require('path');
const bodyParser = require('body-parser');
const socketIo = require('socket.io');
const session = require('express-session');

const app = express();
const PORT = process.env.PORT || 80;
const API_URL = process.env.API_URL || 'http://localhost:3001';
const API_TIMEOUT_MS = Number(process.env.API_TIMEOUT_MS || '5000');
const API_RETRIES = Number(process.env.API_RETRIES || '3');
const API_RETRY_DELAY_MS = Number(process.env.API_RETRY_DELAY_MS || '750');

const apiClient = axios.create({
  baseURL: API_URL,
  timeout: API_TIMEOUT_MS
});

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function apiGet(path, options = {}) {
  let lastError;
  for (let attempt = 1; attempt <= API_RETRIES; attempt += 1) {
    try {
      return await apiClient.get(path, options);
    } catch (error) {
      lastError = error;
      const code = error.code || error.response?.status || 'UNKNOWN';
      console.warn(`[API] GET ${path} failed (attempt ${attempt}/${API_RETRIES}):`, code);
      if (attempt < API_RETRIES) {
        await sleep(API_RETRY_DELAY_MS * attempt);
      }
    }
  }
  throw lastError;
}

// ============ SESSION CONFIGURATION ============
app.use(session({
  secret: 'meshnet-secret-key-change-in-production',
  resave: false,
  saveUninitialized: false,
  cookie: { 
    secure: false, // Set to true if using HTTPS
    httpOnly: true,
    maxAge: 24 * 60 * 60 * 1000 // 24 hours
  }
}));

// ============ MIDDLEWARE ============
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));

app.use(express.static(path.join(__dirname, 'public')));
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

// Authentication middleware
function requireLogin(req, res, next) {
  if (req.session.user) {
    next();
  } else {
    res.redirect('/login');
  }
}

// ============ ROUTES ============

// Login page
app.get('/login', (req, res) => {
  if (req.session.user) {
    res.redirect('/');
  } else {
    res.render('login', { error: null, message: null });
  }
});

// Login POST
app.post('/login', async (req, res) => {
  const { username, password } = req.body;

  if (!username || !password) {
    return res.render('login', { 
      error: 'Username and password are required', 
      message: null 
    });
  }

  try {
    // Call backend API to authenticate
    const response = await axios.post(`${API_URL}/api/auth/login`, {
      username,
      password
    });

    if (response.data.success) {
      // Create session
      req.session.user = {
        userId: response.data.user.userId,
        username: response.data.user.username,
        email: response.data.user.email
      };

      res.redirect('/');
    } else {
      res.render('login', { 
        error: response.data.error || 'Invalid credentials', 
        message: null 
      });
    }
  } catch (error) {
    console.error('Login error:', error.response?.data || error.message);
    res.render('login', { 
      error: error.response?.data?.error || 'Invalid username or password', 
      message: null 
    });
  }
});

// Logout
app.get('/logout', async (req, res) => {
  try {
    // Delete session from backend database if user exists
    if (req.session.user && req.session.user.userId) {
      try {
        await axios.delete(`${API_URL}/api/auth/logout/${req.session.user.userId}`);
      } catch (error) {
        console.error('Error deleting session from backend:', error.message);
      }
    }

    // Destroy local session
    req.session.destroy(() => {
      res.render('logout', {
        title: 'Logged Out',
        message: 'You have been successfully logged out.'
      });
    });
  } catch (error) {
    console.error('Logout error:', error);
    req.session.destroy(() => {
      res.redirect('/login');
    });
  }
});

// Dashboard
app.get('/', requireLogin, async (req, res) => {
  try {
    const [nodesRes, groupsRes, usersRes] = await Promise.all([
      apiGet('/api/nodes'),
      apiGet('/api/groups'),
      apiGet('/api/users')
    ]);
    
    res.render('index', {
      nodes: nodesRes.data,
      groups: groupsRes.data,
      users: usersRes.data,
      user: req.session.user,
      title: 'MeshNet Dashboard',
      error: null
    });
  } catch (error) {
    console.error('Error loading dashboard:', error);
    res.render('index', {
      nodes: [],
      groups: [],
      users: [],
      user: req.session.user,
      error: 'Failed to load data from backend',
      title: 'MeshNet Dashboard'
    });
  }
});

// Nodes management page
app.get('/nodes', requireLogin, async (req, res) => {
  try {
    const response = await apiGet('/api/nodes');
    res.render('nodes', {
      nodes: response.data,
      user: req.session.user,
      title: 'Node Management',
      error: null
    });
  } catch (error) {
    console.error('Error loading nodes:', error);
    res.render('nodes', { 
      nodes: [], 
      user: req.session.user,
      error: 'Failed to load nodes',
      title: 'Node Management'
    });
  }
});

// Mobile-optimized nodes management page
app.get('/nodes-mobile', requireLogin, async (req, res) => {
  try {
    const response = await apiGet('/api/nodes');
    res.render('nodes-mobile', {
      nodes: response.data,
      user: req.session.user,
      title: 'Node Management',
      error: null
    });
  } catch (error) {
    console.error('Error loading nodes:', error);
    res.render('nodes-mobile', { 
      nodes: [], 
      user: req.session.user,
      error: 'Failed to load nodes',
      title: 'Node Management'
    });
  }
});

// Node connections dashboard
app.get('/node-connections', requireLogin, async (req, res) => {
  try {
    const response = await apiGet('/api/nodes');
    const nodes = response.data;
    
    // Get connections for each node
    const nodesWithConnections = await Promise.all(
      nodes.map(async (node) => {
        try {
          const connRes = await apiGet(`/api/host/node/${node.nodeId}/connections`);
          return {
            ...node,
            connections: connRes.data.connections || []
          };
        } catch (error) {
          console.warn(`Failed to load connections for node ${node.nodeId}:`, error.message);
          return {
            ...node,
            connections: []
          };
        }
      })
    );
    
    res.render('node-connections', {
      nodes: nodesWithConnections,
      user: req.session.user,
      title: 'Node Team Connections',
      error: null
    });
  } catch (error) {
    console.error('Error loading node connections:', error);
    res.render('node-connections', { 
      nodes: [], 
      user: req.session.user,
      error: 'Failed to load node connections',
      title: 'Node Team Connections'
    });
  }
});

// Users management page
app.get('/users', requireLogin, async (req, res) => {
  try {
    const [usersRes, groupsRes] = await Promise.all([
      apiGet('/api/users'),
      apiGet('/api/groups')
    ]);
    
    let filteredUsers = usersRes.data;
    
    // Filter by groupId
    if (req.query.groupId) {
      filteredUsers = filteredUsers.filter(u => u.groupId === parseInt(req.query.groupId));
    }
    
    // Search by name or email
    if (req.query.search) {
      const searchTerm = req.query.search.toLowerCase();
      filteredUsers = filteredUsers.filter(u => 
        u.username.toLowerCase().includes(searchTerm) || 
        u.email.toLowerCase().includes(searchTerm)
      );
    }
    
    // Create a map of group IDs to names
    const groupMap = {};
    groupsRes.data.forEach(group => {
      groupMap[group.id] = group.name;
    });
    
    res.render('users', {
      users: filteredUsers,
      groups: groupsRes.data,
      groupMap,
      user: req.session.user,
      title: 'User Management',
      error: null
    });
  } catch (error) {
    res.render('users', { 
      users: [], 
      groups: [], 
      groupMap: {}, 
      user: req.session.user,
      error: 'Failed to load users', 
      title: 'User Management' 
    });
  }
});

// Groups management page
app.get('/groups', requireLogin, async (req, res) => {
  try {
    const [groupsRes, usersRes] = await Promise.all([
      apiGet('/api/groups'),
      apiGet('/api/users')
    ]);
    
    // Count members per group
    let groupsWithCounts = groupsRes.data.map(group => ({
      ...group,
      memberCount: usersRes.data.filter(u => u.groupId === group.id).length
    }));
    
    // Search by group name
    if (req.query.search) {
      const searchTerm = req.query.search.toLowerCase();
      groupsWithCounts = groupsWithCounts.filter(g => 
        g.name.toLowerCase().includes(searchTerm)
      );
    }
    
    // Sort groups
    if (req.query.sort === 'members') {
      groupsWithCounts.sort((a, b) => b.memberCount - a.memberCount);
    } else if (req.query.sort === 'created') {
      groupsWithCounts.sort((a, b) => new Date(b.createdAt) - new Date(a.createdAt));
    } else {
      // Default: sort by name
      groupsWithCounts.sort((a, b) => a.name.localeCompare(b.name));
    }
    
    res.render('groups', {
      groups: groupsWithCounts,
      user: req.session.user,
      title: 'Group Management',
      error: null
    });
  } catch (error) {
    res.render('groups', { 
      groups: [], 
      user: req.session.user,
      error: 'Failed to load groups', 
      title: 'Group Management' 
    });
  }
});

// Pages editor
app.get('/pages', requireLogin, async (req, res) => {
  try {
    const [groupsRes, nodesRes, pagesRes] = await Promise.all([
      apiGet('/api/groups'),
      apiGet('/api/nodes'),
      apiGet('/api/pages/all') // Get all pages, not just active ones
    ]);
    
    // Create a map of node IDs to names
    const nodeMap = {};
    nodesRes.data.forEach(node => {
      nodeMap[node.nodeId] = node.functionalName || 'Node ' + node.nodeId;
    });
    
    res.render('pages', {
      groups: groupsRes.data,
      nodes: nodesRes.data,
      pages: pagesRes.data,
      nodeMap,
      user: req.session.user,
      title: 'Page Management',
      error: null
    });
  } catch (error) {
    res.render('pages', {
      groups: [],
      nodes: [],
      pages: [],
      nodeMap: {},
      user: req.session.user,
      error: 'Failed to load data',
      title: 'Page Management'
    });
  }
});

// Broadcast messages page
app.get('/broadcast', requireLogin, async (req, res) => {
  try {
    const [nodesRes, pagesRes] = await Promise.all([
      apiGet('/api/nodes'),
      apiGet('/api/pages')
    ]);
    
    res.render('broadcast', {
      nodes: nodesRes.data,
      pages: pagesRes.data,
      user: req.session.user,
      title: 'Send Broadcast Message',
      error: null
    });
  } catch (error) {
    res.render('broadcast', {
      nodes: [],
      pages: [],
      user: req.session.user,
      error: 'Failed to load data',
      title: 'Send Broadcast Message'
    });
  }
});

// Monitoring
app.get('/monitoring', async (req, res) => {
  try {
    const [nodesRes, topologyRes] = await Promise.all([
      apiGet('/api/nodes'),
      apiGet('/api/topology')
    ]);
    
    res.render('monitoring', {
      nodes: nodesRes.data,
      topology: topologyRes.data,
      title: 'Network Monitoring'
    });
  } catch (error) {
    res.render('monitoring', {
      nodes: [],
      topology: [],
      error: 'Failed to load data'
    });
  }
});

// Health check
app.get('/health', (req, res) => {
  res.json({ status: 'ok' });
});

// ============ API PROXY ENDPOINTS ============

// Create node
app.post('/api/nodes', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/nodes`, req.body);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get single node
app.get('/api/nodes/:nodeId', async (req, res) => {
  try {
    const response = await apiGet(`/api/nodes/${req.params.nodeId}`);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Update node
app.put('/api/nodes/:nodeId', async (req, res) => {
  try {
    const response = await axios.put(`${API_URL}/api/nodes/${req.params.nodeId}`, req.body);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Request node stats (users/pages) via LoRa
app.post('/api/nodes/:nodeId/stats/request', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/nodes/${req.params.nodeId}/stats/request`);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Update node stats (users/pages)
app.post('/api/nodes/:nodeId/stats', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/nodes/${req.params.nodeId}/stats`, req.body || {});
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Delete node
app.delete('/api/nodes/:nodeId', async (req, res) => {
  try {
    const response = await axios.delete(`${API_URL}/api/nodes/${req.params.nodeId}`);
    res.json(response.data || { success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Create user
app.post('/api/users', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/users`, req.body);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Update user
app.put('/api/users/:userId', async (req, res) => {
  try {
    const response = await axios.put(`${API_URL}/api/users/${req.params.userId}`, req.body);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Delete user
app.delete('/api/users/:userId', async (req, res) => {
  try {
    const response = await axios.delete(`${API_URL}/api/users/${req.params.userId}`);
    res.json(response.data || { success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Create group
app.post('/api/groups', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/groups`, req.body);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Update group
app.put('/api/groups/:groupId', async (req, res) => {
  try {
    const response = await axios.put(`${API_URL}/api/groups/${req.params.groupId}`, req.body);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Delete group
app.delete('/api/groups/:groupId', async (req, res) => {
  try {
    const response = await axios.delete(`${API_URL}/api/groups/${req.params.groupId}`);
    res.json(response.data || { success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Create page
app.post('/api/pages', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/pages`, req.body);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Update page
app.put('/api/pages/:pageId', async (req, res) => {
  try {
    const response = await axios.put(`${API_URL}/api/pages/${req.params.pageId}`, req.body);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Delete page
app.delete('/api/pages/:pageId', async (req, res) => {
  try {
    const response = await axios.delete(`${API_URL}/api/pages/${req.params.pageId}`);
    res.json(response.data || { success: true });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Push users/pages sync to nodes
app.post('/api/sync/push', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/sync/push`, req.body || {});
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Push users sync to nodes
app.post('/api/sync/push/users', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/sync/push/users`, req.body || {});
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Push pages sync to nodes
app.post('/api/sync/push/pages', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/sync/push/pages`, req.body || {});
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Ensure default pages exist
app.post('/api/pages/ensure-defaults', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/pages/ensure-defaults`, req.body || {});
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Create page (broadcast message)
app.post('/api/broadcast', async (req, res) => {
  try {
    const response = await axios.post(`${API_URL}/api/pages`, req.body);
    res.json(response.data);
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// ============ ERROR HANDLING ============
app.use((err, req, res, next) => {
  console.error('Error:', err);
  res.status(500).render('error', { error: 'Internal server error' });
});

app.use((req, res) => {
  res.status(404).render('404');
});

// ============ START SERVER ============
const server = app.listen(PORT, () => {
  console.log(`\n╔════════════════════════════════════╗`);
  console.log(`║  MeshNet Webserver                 ║`);
  console.log(`║  Running on port ${PORT}               ║`);
  console.log(`║  Backend: ${API_URL}     ║`);
  console.log(`╚════════════════════════════════════╝\n`);
});

// ============ WEBSOCKET (SOCKET.IO) ============
const io = socketIo(server, {
  cors: { origin: '*' }
});

// Real-time data managers
let activeSubscriptions = new Map();

io.on('connection', (socket) => {
  console.log('🔌 Client connected:', socket.id);
  
  // Subscribe to node updates
  socket.on('subscribe-nodes', async () => {
    socket.join('nodes');
    try {
      const response = await apiGet('/api/nodes');
      socket.emit('nodes-list', response.data);
    } catch (error) {
      console.error('Failed to fetch nodes:', error.message);
    }
  });
  
  // Subscribe to monitoring data
  socket.on('subscribe-monitoring', async () => {
    socket.join('monitoring');
    try {
      const [nodesRes, topologyRes] = await Promise.all([
        apiGet('/api/nodes'),
        apiGet('/api/topology')
      ]);
      
      const avgSignal = nodesRes.data.length > 0 
        ? Math.round(nodesRes.data.reduce((sum, n) => sum + (n.signal || 0), 0) / nodesRes.data.length)
        : 0;
      
      const avgBattery = nodesRes.data.length > 0
        ? Math.round(nodesRes.data.reduce((sum, n) => sum + (n.battery || 0), 0) / nodesRes.data.length)
        : 0;
      
      socket.emit('monitoring-data', {
        nodes: nodesRes.data,
        topology: topologyRes.data,
        avgSignal,
        avgBattery,
        timestamp: new Date()
      });
    } catch (error) {
      console.error('Failed to fetch monitoring data:', error.message);
    }
  });
  
  // Subscribe to dashboard
  socket.on('subscribe-dashboard', async () => {
    socket.join('dashboard');
    try {
      const [nodesRes, groupsRes] = await Promise.all([
        apiGet('/api/nodes'),
        apiGet('/api/groups')
      ]);
      
      socket.emit('dashboard-data', {
        totalNodes: nodesRes.data.length,
        activeNodes: nodesRes.data.filter(n => n.isActive).length,
        totalGroups: groupsRes.data.length,
        timestamp: new Date()
      });
    } catch (error) {
      console.error('Failed to fetch dashboard data:', error.message);
    }
  });
  
  // Listen for user actions and broadcast to others
  socket.on('node-action', (data) => {
    io.to('monitoring').emit('activity-log', {
      timestamp: new Date(),
      type: 'node-action',
      message: `Node ${data.nodeId}: ${data.action}`,
      nodeId: data.nodeId
    });
  });
  
  socket.on('disconnect', () => {
    console.log('🔌 Client disconnected:', socket.id);
  });
});

// Emit updates at regular intervals
setInterval(async () => {
  try {
    const response = await apiGet('/api/nodes');
    if (response.data.length > 0) {
      io.to('monitoring').emit('nodes-update', {
        nodes: response.data,
        timestamp: new Date()
      });
    }
  } catch (error) {
    console.error('Failed to broadcast node updates:', error.message);
  }
}, 5000); // Update every 5 seconds

// Broadcast node update function
function broadcastNodeUpdate(nodeData) {
  io.to('nodes').emit('node-update', {
    ...nodeData,
    timestamp: new Date()
  });
  
  io.to('monitoring').emit('node-status', {
    ...nodeData,
    timestamp: new Date()
  });
  
  io.to('dashboard').emit('dashboard-event', {
    type: 'node-updated',
    nodeId: nodeData.nodeId,
    timestamp: new Date()
  });
}

module.exports = { app, broadcastNodeUpdate };
