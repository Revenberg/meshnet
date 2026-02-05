-- MeshNet Database Schema
-- MySQL initialization script (corrected order)

-- Groups table (FIRST - no dependencies)
CREATE TABLE IF NOT EXISTS `groups` (
  id INT AUTO_INCREMENT PRIMARY KEY,
  groupId VARCHAR(64) UNIQUE NOT NULL,
  name VARCHAR(128) NOT NULL,
  description TEXT,
  permissions JSON,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- Users table (AFTER groups)
CREATE TABLE IF NOT EXISTS users (
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
  FOREIGN KEY (groupId) REFERENCES `groups`(id),
  INDEX idx_groupId (groupId)
);

-- Nodes table
CREATE TABLE IF NOT EXISTS nodes (
  id INT AUTO_INCREMENT PRIMARY KEY,
  nodeId VARCHAR(64) UNIQUE NOT NULL,
  macAddress VARCHAR(17) UNIQUE NOT NULL,
  functionalName VARCHAR(32),
  version VARCHAR(16),
  lastSeen TIMESTAMP,
  signalStrength INT,
  battery INT,
  connectedNodes INT DEFAULT 0,
  loadedUsersCount INT DEFAULT 0,
  loadedPagesCount INT DEFAULT 0,
  statsUpdatedAt TIMESTAMP NULL,
  isActive BOOLEAN DEFAULT TRUE,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_nodeId (nodeId),
  INDEX idx_isActive (isActive)
);

-- Pages table
CREATE TABLE IF NOT EXISTS pages (
  id INT AUTO_INCREMENT PRIMARY KEY,
  pageId VARCHAR(64) UNIQUE NOT NULL,
  nodeId VARCHAR(64) NOT NULL,
  groupId INT,
  title VARCHAR(255) NOT NULL,
  content LONGTEXT,
  imageUrl VARCHAR(512),
  refreshInterval INT DEFAULT 30,
  isActive BOOLEAN DEFAULT TRUE,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (nodeId) REFERENCES nodes(nodeId),
  FOREIGN KEY (groupId) REFERENCES `groups`(id),
  INDEX idx_nodeId (nodeId),
  INDEX idx_groupId (groupId),
  INDEX idx_isActive (isActive)
);

-- Images table
CREATE TABLE IF NOT EXISTS images (
  id INT AUTO_INCREMENT PRIMARY KEY,
  imageId VARCHAR(64) UNIQUE NOT NULL,
  nodeId VARCHAR(64),
  groupId INT,
  filename VARCHAR(255) NOT NULL,
  filepath VARCHAR(512) NOT NULL,
  filesize INT,
  mimeType VARCHAR(64),
  uploadedBy VARCHAR(64),
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (nodeId) REFERENCES nodes(nodeId),
  FOREIGN KEY (groupId) REFERENCES `groups`(id),
  INDEX idx_nodeId (nodeId),
  INDEX idx_groupId (groupId)
);

-- User sessions (cached from nodes)
CREATE TABLE IF NOT EXISTS user_sessions (
  id INT AUTO_INCREMENT PRIMARY KEY,
  sessionId VARCHAR(64) UNIQUE NOT NULL,
  userId VARCHAR(64) NOT NULL,
  nodeId VARCHAR(64),
  token LONGTEXT,
  expiresAt TIMESTAMP,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (userId) REFERENCES users(userId),
  FOREIGN KEY (nodeId) REFERENCES nodes(nodeId),
  INDEX idx_userId (userId),
  INDEX idx_nodeId (nodeId)
);

-- Message log (for debugging and analytics)
CREATE TABLE IF NOT EXISTS message_log (
  id INT AUTO_INCREMENT PRIMARY KEY,
  messageId VARCHAR(64) UNIQUE NOT NULL,
  messageType VARCHAR(32),
  fromNodeId VARCHAR(64),
  toNodeId VARCHAR(64),
  status VARCHAR(32),
  hops INT,
  timestamp BIGINT,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (fromNodeId) REFERENCES nodes(nodeId),
  FOREIGN KEY (toNodeId) REFERENCES nodes(nodeId),
  INDEX idx_fromNodeId (fromNodeId),
  INDEX idx_toNodeId (toNodeId)
);

-- Node connections (topology tracking)
CREATE TABLE IF NOT EXISTS node_network_topology (
  id INT AUTO_INCREMENT PRIMARY KEY,
  nodeIdFrom VARCHAR(64),
  nodeIdTo VARCHAR(64),
  signalStrength INT,
  hops INT,
  lastSeen TIMESTAMP,
  FOREIGN KEY (nodeIdFrom) REFERENCES nodes(nodeId),
  FOREIGN KEY (nodeIdTo) REFERENCES nodes(nodeId),
  INDEX idx_nodeIdFrom (nodeIdFrom),
  INDEX idx_nodeIdTo (nodeIdTo),
  UNIQUE KEY unique_connection (nodeIdFrom, nodeIdTo)
);

-- User/Team Logins per Node
CREATE TABLE IF NOT EXISTS node_user_connections (
  id INT AUTO_INCREMENT PRIMARY KEY,
  connectionId VARCHAR(64) UNIQUE NOT NULL,
  nodeId VARCHAR(64) NOT NULL,
  username VARCHAR(128),
  teamName VARCHAR(128),
  connectedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  lastSeen TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (nodeId) REFERENCES nodes(nodeId),
  INDEX idx_nodeId (nodeId),
  INDEX idx_teamName (teamName),
  INDEX idx_connectedAt (connectedAt)
);

-- ACKs table
CREATE TABLE IF NOT EXISTS acks (
  id INT AUTO_INCREMENT PRIMARY KEY,
  msgId VARCHAR(64),
  nodeId VARCHAR(64),
  object VARCHAR(64),
  `function` VARCHAR(64),
  ackTimestamp DATETIME,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_msgId (msgId),
  INDEX idx_nodeId (nodeId)
);

-- Pings table
CREATE TABLE IF NOT EXISTS pings (
  id INT AUTO_INCREMENT PRIMARY KEY,
  nodeId VARCHAR(64),
  status VARCHAR(32),
  latencyMs INT,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_nodeId (nodeId)
);

-- Broadcasts table (mesh-wide broadcast messages)
CREATE TABLE IF NOT EXISTS broadcasts (
  id INT AUTO_INCREMENT PRIMARY KEY,
  broadcastId VARCHAR(64) UNIQUE NOT NULL,
  userId VARCHAR(64),
  username VARCHAR(128) NOT NULL,
  title VARCHAR(255),
  content LONGTEXT NOT NULL,
  targetNodes JSON,
  ttl INT DEFAULT 60,
  status VARCHAR(32) DEFAULT 'active',
  deliveredTo JSON,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  expiresAt TIMESTAMP,
  FOREIGN KEY (userId) REFERENCES users(userId),
  INDEX idx_username (username),
  INDEX idx_status (status),
  INDEX idx_createdAt (createdAt)
);

-- ============ DEFAULT DATA ============

-- Insert default groups
INSERT IGNORE INTO `groups` (groupId, name, description, permissions) VALUES 
  ('default', 'Default Group', 'Default user group', JSON_ARRAY());

INSERT IGNORE INTO `groups` (groupId, name, description, permissions) VALUES 
  ('admin', 'Administrator', 'Administrator group with full access', JSON_ARRAY('view_dashboard', 'view_nodes', 'edit_nodes', 'edit_pages', 'manage_users', 'manage_groups', 'view_logs', 'system_admin', 'view_users', 'send_broadcast'));

-- Insert admin user (password: admin123)
-- Hash generated: $2a$10$sHsFFvNhyOo3ZvaQZ0eDI.L1PltRaSPI6fGEiEH8gkj8Fc49fz4nK
INSERT IGNORE INTO users (userId, username, email, passwordHash, passwordSha256, groupId, isActive) VALUES 
  ('admin-user-001', 'admin', 'admin@meshnet.local', '$2a$10$sHsFFvNhyOo3ZvaQZ0eDI.L1PltRaSPI6fGEiEH8gkj8Fc49fz4nK', '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9', (SELECT id FROM `groups` WHERE groupId='admin'), TRUE);
