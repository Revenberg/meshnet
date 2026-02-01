// MeshNet Dashboard Client - Real-time Socket.io Handler
const socket = typeof io !== 'undefined' ? io() : null;
const API_URL = window.location.origin;

// ============ CONNECTION HANDLERS ============

if (socket) {
  socket.on('connect', () => {
    console.log('✅ Connected to dashboard server');
    document.body.classList.add('connected');
    
    // Subscribe based on current page
    const path = window.location.pathname;
    if (path === '/') {
      socket.emit('subscribe-dashboard');
      socket.emit('subscribe-monitoring');
    } else if (path === '/nodes') {
      socket.emit('subscribe-nodes');
    } else if (path === '/monitoring') {
      socket.emit('subscribe-monitoring');
    }
  });

  socket.on('disconnect', () => {
    console.log('❌ Disconnected from dashboard server');
    document.body.classList.remove('connected');
  });

  socket.on('connect_error', (error) => {
    console.error('❌ Connection error:', error);
  });

  // ============ DASHBOARD EVENTS ============

  socket.on('dashboard-data', (data) => {
    console.log('📊 Dashboard data:', data);
    updateDashboardStats(data);
  });

  socket.on('dashboard-event', (data) => {
    console.log('📈 Dashboard event:', data);
    updateActivityLog(`${data.type} - ${data.nodeId || 'System'}`);
  });
} else {
  console.warn('⚠️ Socket.io not available - real-time updates disabled');
}

function updateDashboardStats(data) {
  // Update active nodes
  const activeNodesElement = document.querySelector('[data-stat="active-nodes"]');
  if (activeNodesElement) {
    activeNodesElement.textContent = data.activeNodes || 0;
    highlightChange(activeNodesElement);
  }
  
  // Update total nodes
  const totalNodesElement = document.querySelector('[data-stat="total-nodes"]');
  if (totalNodesElement) {
    totalNodesElement.textContent = data.totalNodes || 0;
    highlightChange(totalNodesElement);
  }
  
  // Update groups
  const groupsElement = document.querySelector('[data-stat="groups"]');
  if (groupsElement) {
    groupsElement.textContent = data.totalGroups || 0;
    highlightChange(groupsElement);
  }
}

function highlightChange(element) {
  element.classList.add('pulse-update');
  setTimeout(() => element.classList.remove('pulse-update'), 300);
}

// ============ NODES EVENTS ============

if (socket) {
  socket.on('nodes-list', (nodes) => {
    console.log('📡 Nodes list received:', nodes.length, 'nodes');
    updateNodesTable(nodes);
  });

  socket.on('node-update', (data) => {
    console.log('📡 Node update:', data);
    updateNodeInTable(data);
    updateActivityLog(`Node ${data.nodeName || data.nodeId}: Updated`);
  });

  socket.on('nodes-update', (data) => {
    console.log('📡 Bulk update:', data.nodes.length, 'nodes');
    data.nodes.forEach(node => updateNodeInTable(node));
  });
}

function updateNodesTable(nodes) {
  const tbody = document.querySelector('table tbody');
  if (!tbody) return;
  
  const existingNodeIds = new Set(Array.from(tbody.querySelectorAll('tr')).map(r => r.dataset.nodeId));
  
  nodes.forEach(node => {
    let row = document.querySelector(`tr[data-node-id="${node.nodeId}"]`);
    if (!row) {
      row = document.createElement('tr');
      row.setAttribute('data-node-id', node.nodeId);
      tbody.appendChild(row);
    }
    updateNodeRow(row, node);
    existingNodeIds.delete(node.nodeId);
  });
}

function updateNodeInTable(node) {
  let row = document.querySelector(`tr[data-node-id="${node.nodeId}"]`);
  if (!row) {
    row = document.createElement('tr');
    row.setAttribute('data-node-id', node.nodeId);
    const tbody = document.querySelector('table tbody');
    if (tbody) tbody.appendChild(row);
  }
  updateNodeRow(row, node);
}

function updateNodeRow(row, node) {
  row.innerHTML = `
    <td><strong>${node.nodeName || 'Unknown'}</strong></td>
    <td><code>${node.nodeId || '-'}</code></td>
    <td><span class="battery-indicator" data-battery="${node.battery || 0}">${node.battery || 0}%</span></td>
    <td>${node.signal || node.signalStrength || '-'} dBm</td>
    <td><span class="status-badge status-${node.isActive ? 'active' : 'inactive'}">
      ${node.isActive ? '🟢 Active' : '🔴 Offline'}</span></td>
    <td><small>${node.lastSeen ? new Date(node.lastSeen).toLocaleTimeString() : '-'}</small></td>
    <td>
      <button class="btn btn-small btn-primary" onclick="editNode('${node.nodeId}')">Edit</button>
      <button class="btn btn-small btn-danger" onclick="deleteNode('${node.nodeId}')">Delete</button>
    </td>
  `;
  
  row.classList.add('highlight-update');
  setTimeout(() => row.classList.remove('highlight-update'), 500);
}

// ============ MONITORING EVENTS ============

if (socket) {
  socket.on('monitoring-data', (data) => {
    console.log('📊 Monitoring data:', data);
    updateMonitoringUI(data);
  });

  socket.on('node-status', (data) => {
    console.log('📡 Node status changed:', data.nodeId);
    updateActivityLog(`Node ${data.nodeName || data.nodeId}: ${data.isActive ? 'Online 🟢' : 'Offline 🔴'}`);
  });
}

function updateMonitoringUI(data) {
  // Update signal metric
  const signalMetric = document.querySelector('[data-metric="signal"]');
  if (signalMetric) {
    signalMetric.innerHTML = `
      <span class="value signal-${data.avgSignal > 80 ? 'excellent' : data.avgSignal > 60 ? 'good' : 'fair'}">
        ${data.avgSignal}%
      </span>
    `;
  }
  
  // Update battery metric
  const batteryMetric = document.querySelector('[data-metric="battery"]');
  if (batteryMetric) {
    batteryMetric.innerHTML = `
      <span class="value battery-${data.avgBattery > 50 ? 'good' : 'low'}">
        ${data.avgBattery}%
      </span>
    `;
  }
  
  // Update nodes table
  if (data.nodes) {
    updateNodesTable(data.nodes);
  }
}

// ============ ACTIVITY LOG ============

function updateActivityLog(message) {
  const logContainer = document.querySelector('#messageLog, .activity-log, [data-activity-log]');
  if (!logContainer) return;
  
  const entry = document.createElement('div');
  entry.className = 'log-entry';
  
  const now = new Date();
  const timeStr = now.toLocaleTimeString();
  
  entry.innerHTML = `<span class="log-time">${timeStr}</span> <span>${message}</span>`;
  
  logContainer.insertBefore(entry, logContainer.firstChild);
  
  // Keep only last 50 entries
  while (logContainer.children.length > 50) {
    logContainer.removeChild(logContainer.lastChild);
  }
}

// ============ MODAL FUNCTIONS ============

function openAddNodeModal() {
  const modal = document.getElementById('nodeModal');
  if (modal) {
    modal.classList.add('show');
    document.getElementById('nodeForm')?.reset();
  }
}

function closeModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) {
    modal.classList.remove('show');
  }
}

function openAddUserModal() {
  const modal = document.getElementById('userModal');
  if (modal) {
    modal.classList.add('show');
    document.getElementById('userForm')?.reset();
  }
}

function closeUserModal() {
  closeModal('userModal');
}

function openAddGroupModal() {
  const modal = document.getElementById('groupModal');
  if (modal) {
    modal.classList.add('show');
    document.getElementById('groupForm')?.reset();
  }
}

function closeGroupModal() {
  closeModal('groupModal');
}

function openAddPageModal() {
  const modal = document.getElementById('pageModal');
  if (modal) {
    modal.classList.add('show');
    document.getElementById('pageForm')?.reset();
  }
}

function closePageModal() {
  closeModal('pageModal');
}

// Close modals on ESC key
document.addEventListener('keydown', (e) => {
  if (e.key === 'Escape') {
    document.querySelectorAll('.modal.show').forEach(modal => {
      modal.classList.remove('show');
    });
  }
});

// Close modal when clicking outside
document.addEventListener('click', (e) => {
  if (e.target.classList.contains('modal')) {
    e.target.classList.remove('show');
  }
});

// ============ NODE MANAGEMENT ============

function editNode(nodeId) {
  // Fetch node data and populate modal
  fetch(`/api/nodes/${nodeId}`)
    .then(r => r.json())
    .then(node => {
      document.getElementById('nodeId').value = node.nodeId;
      document.getElementById('nodeName').value = node.nodeName;
      // Populate other fields...
      document.getElementById('nodeModal').classList.add('show');
    })
    .catch(err => showNotification('Error loading node: ' + err, 'error'));
}

function deleteNode(nodeId) {
  if (confirm(`Delete node ${nodeId}?`)) {
    fetch(`/api/nodes/${nodeId}`, { method: 'DELETE' })
      .then(r => r.json())
      .then(data => {
        if (data.success) {
          showNotification('Node deleted', 'success');
          document.querySelector(`tr[data-node-id="${nodeId}"]`)?.remove();
          socket.emit('node-action', { nodeId, action: 'deleted' });
        }
      })
      .catch(err => showNotification('Error deleting node: ' + err, 'error'));
  }
}

function saveNode() {
  const form = document.getElementById('nodeForm');
  if (!form) return;
  
  const nodeId = document.getElementById('nodeId').value;
  const nodeName = document.getElementById('nodeName').value;
  
  if (!nodeName) {
    showNotification('Node name is required', 'warning');
    return;
  }
  
  const data = {
    nodeName,
    // Add other fields as needed
  };
  
  const method = nodeId ? 'PUT' : 'POST';
  const url = nodeId ? `/api/nodes/${nodeId}` : '/api/nodes';
  
  fetch(url, {
    method,
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  })
    .then(r => r.json())
    .then(result => {
      if (result.success || result.nodeId) {
        showNotification(nodeId ? 'Node updated' : 'Node created', 'success');
        closeModal('nodeModal');
        socket.emit('node-action', { nodeId: result.nodeId, action: nodeId ? 'updated' : 'created' });
        setTimeout(() => location.reload(), 500);
      } else {
        showNotification(result.error || 'Operation failed', 'error');
      }
    })
    .catch(err => showNotification('Error saving node: ' + err, 'error'));
}

// ============ USER MANAGEMENT ============

function saveUser() {
  const form = document.getElementById('userForm');
  if (!form) return;
  
  const username = document.getElementById('username').value;
  const email = document.getElementById('email').value;
  const password = document.getElementById('password').value;
  const groupId = document.getElementById('groupId').value;
  
  if (!username || !email || !password || !groupId) {
    showNotification('All fields are required', 'warning');
    return;
  }
  
  const data = { username, email, password, groupId };
  
  fetch('/api/users', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  })
    .then(r => r.json())
    .then(result => {
      if (result.success || result.userId) {
        showNotification('User created successfully', 'success');
        closeUserModal();
        socket.emit('node-action', { action: 'user-created' });
        setTimeout(() => location.reload(), 500);
      } else {
        showNotification(result.error || 'Failed to create user', 'error');
      }
    })
    .catch(err => showNotification('Error: ' + err, 'error'));
}

function editUser(userId) {
  fetch(`/api/users/${userId}`)
    .then(r => r.json())
    .then(user => {
      document.getElementById('username').value = user.username;
      document.getElementById('email').value = user.email;
      document.getElementById('groupId').value = user.groupId;
      document.getElementById('password').placeholder = '(leave empty to keep current)';
      openAddUserModal();
    })
    .catch(err => showNotification('Error loading user: ' + err, 'error'));
}

function deleteUser(userId) {
  if (confirm('Delete this user?')) {
    fetch(`/api/users/${userId}`, { method: 'DELETE' })
      .then(r => r.json())
      .then(data => {
        if (data.success) {
          showNotification('User deleted', 'success');
          document.querySelector(`tr[data-user-id="${userId}"]`)?.remove();
        }
      })
      .catch(err => showNotification('Error: ' + err, 'error'));
  }
}

// ============ GROUP MANAGEMENT ============

function saveGroup() {
  const form = document.getElementById('groupForm');
  if (!form) return;
  
  const groupName = document.getElementById('groupName').value;
  if (!groupName) {
    showNotification('Group name is required', 'warning');
    return;
  }
  
  // Collect permissions
  const permissions = {
    nodes_read: document.querySelector('input[name="perm_nodes_read"]')?.checked || false,
    nodes_write: document.querySelector('input[name="perm_nodes_write"]')?.checked || false,
    users_read: document.querySelector('input[name="perm_users_read"]')?.checked || false,
    users_write: document.querySelector('input[name="perm_users_write"]')?.checked || false,
    groups_write: document.querySelector('input[name="perm_groups_write"]')?.checked || false,
    pages_write: document.querySelector('input[name="perm_pages_write"]')?.checked || false,
    monitor: document.querySelector('input[name="perm_monitor"]')?.checked || false,
    admin: document.querySelector('input[name="perm_admin"]')?.checked || false
  };
  
  const data = { name: groupName, permissions: JSON.stringify(permissions) };
  
  fetch('/api/groups', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  })
    .then(r => r.json())
    .then(result => {
      if (result.success || result.id) {
        showNotification('Group created', 'success');
        closeGroupModal();
        socket.emit('node-action', { action: 'group-created' });
        setTimeout(() => location.reload(), 500);
      } else {
        showNotification(result.error || 'Failed to create group', 'error');
      }
    })
    .catch(err => showNotification('Error: ' + err, 'error'));
}

function editGroup(groupId) {
  fetch(`/api/groups/${groupId}`)
    .then(r => r.json())
    .then(group => {
      document.getElementById('groupName').value = group.name;
      const perms = JSON.parse(group.permissions || '{}');
      document.querySelector('input[name="perm_nodes_read"]').checked = perms.nodes_read;
      document.querySelector('input[name="perm_nodes_write"]').checked = perms.nodes_write;
      // Set other permissions...
      openAddGroupModal();
    })
    .catch(err => showNotification('Error loading group: ' + err, 'error'));
}

function deleteGroup(groupId) {
  if (confirm('Delete this group?')) {
    fetch(`/api/groups/${groupId}`, { method: 'DELETE' })
      .then(r => r.json())
      .then(data => {
        if (data.success) {
          showNotification('Group deleted', 'success');
          document.querySelector(`tr[data-group-id="${groupId}"]`)?.remove();
        }
      })
      .catch(err => showNotification('Error: ' + err, 'error'));
  }
}

// ============ PAGE MANAGEMENT ============

function savePage() {
  const form = document.getElementById('pageForm');
  if (!form) return;
  
  const pageTitle = document.getElementById('pageTitle').value;
  const pageContent = document.getElementById('pageContent').value;
  const contentType = document.getElementById('contentType').value;
  const targetNode = document.getElementById('targetNode').value;
  const isActive = document.querySelector('input[name="isActive"]')?.checked || false;
  
  if (!pageTitle || !pageContent) {
    showNotification('Title and content are required', 'warning');
    return;
  }
  
  const data = {
    title: pageTitle,
    content: pageContent,
    contentType,
    nodeId: targetNode || null,
    isActive
  };
  
  fetch('/api/pages', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  })
    .then(r => r.json())
    .then(result => {
      if (result.success || result.pageId) {
        showNotification('Page created', 'success');
        closePageModal();
        socket.emit('node-action', { action: 'page-created' });
        setTimeout(() => location.reload(), 500);
      } else {
        showNotification(result.error || 'Failed to create page', 'error');
      }
    })
    .catch(err => showNotification('Error: ' + err, 'error'));
}

function editPage(pageId) {
  fetch(`/api/pages/${pageId}`)
    .then(r => r.json())
    .then(page => {
      document.getElementById('pageTitle').value = page.title;
      document.getElementById('pageContent').value = page.content;
      document.getElementById('contentType').value = page.contentType;
      document.getElementById('targetNode').value = page.nodeId || '';
      document.querySelector('input[name="isActive"]').checked = page.isActive;
      openAddPageModal();
    })
    .catch(err => showNotification('Error loading page: ' + err, 'error'));
}

function deletePage(pageId) {
  if (confirm('Delete this page?')) {
    fetch(`/api/pages/${pageId}`, { method: 'DELETE' })
      .then(r => r.json())
      .then(data => {
        if (data.success) {
          showNotification('Page deleted', 'success');
          document.querySelector(`tr[data-page-id="${pageId}"]`)?.remove();
        }
      })
      .catch(err => showNotification('Error: ' + err, 'error'));
  }
}

// ============ MONITORING ============

function setTimeRange(range) {
  const buttons = document.querySelectorAll('.time-range-selector .btn');
  buttons.forEach(btn => btn.classList.remove('active'));
  event.target.classList.add('active');
  console.log('Time range changed to:', range);
}

function inspectNode(nodeId) {
  window.location.href = `/nodes?nodeId=${nodeId}`;
}

// ============ INITIALIZATION ============

document.addEventListener('DOMContentLoaded', () => {
  console.log('📊 MeshNet Dashboard Ready');
});

// ============ NOTIFICATIONS ============

function showNotification(message, type = 'info') {
  // Create notification element
  const notification = document.createElement('div');
  notification.className = `alert alert-${type}`;
  notification.style.cssText = `
    position: fixed;
    top: 20px;
    right: 20px;
    min-width: 300px;
    z-index: 1000;
    animation: slideIn 0.3s ease-in-out;
  `;
  notification.textContent = message;
  
  document.body.appendChild(notification);
  
  // Auto remove after 4 seconds
  setTimeout(() => {
    notification.style.animation = 'slideOut 0.3s ease-in-out forwards';
    setTimeout(() => notification.remove(), 300);
  }, 4000);
}
