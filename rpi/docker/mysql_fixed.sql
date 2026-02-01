-- MeshNet Database Schema
-- MySQL initialization script (corrected order)

-- Groups table (FIRST - no dependencies)
CREATE TABLE IF NOT EXISTS groups (
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
  groupId INT NOT NULL,
  isActive BOOLEAN DEFAULT TRUE,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (groupId) REFERENCES groups(id),
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
  title VARCHAR(128) NOT NULL,
  content LONGTEXT,
  imageUrl VARCHAR(255),
  refreshInterval INT DEFAULT 30,
  isActive BOOLEAN DEFAULT TRUE,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (nodeId) REFERENCES nodes(nodeId),
  FOREIGN KEY (groupId) REFERENCES groups(id),
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
  FOREIGN KEY (groupId) REFERENCES groups(id),
  INDEX idx_nodeId (nodeId),
  INDEX idx_groupId (groupId)
);

-- User Sessions table
CREATE TABLE IF NOT EXISTS user_sessions (
  id INT AUTO_INCREMENT PRIMARY KEY,
  sessionId VARCHAR(128) UNIQUE NOT NULL,
  userId VARCHAR(64) NOT NULL,
  nodeId VARCHAR(64),
  token LONGTEXT NOT NULL,
  expiresAt TIMESTAMP,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (userId) REFERENCES users(userId),
  FOREIGN KEY (nodeId) REFERENCES nodes(nodeId),
  INDEX idx_userId (userId),
  INDEX idx_sessionId (sessionId)
);

-- Message Log table
CREATE TABLE IF NOT EXISTS message_log (
  id INT AUTO_INCREMENT PRIMARY KEY,
  messageId VARCHAR(64) UNIQUE NOT NULL,
  messageType VARCHAR(32),
  fromNodeId VARCHAR(64),
  toNodeId VARCHAR(64),
  status VARCHAR(16),
  hops INT,
  timestamp TIMESTAMP,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (fromNodeId) REFERENCES nodes(nodeId),
  FOREIGN KEY (toNodeId) REFERENCES nodes(nodeId),
  INDEX idx_fromNodeId (fromNodeId),
  INDEX idx_toNodeId (toNodeId),
  INDEX idx_timestamp (timestamp)
);

-- Node Connections table
CREATE TABLE IF NOT EXISTS node_connections (
  id INT AUTO_INCREMENT PRIMARY KEY,
  nodeIdFrom VARCHAR(64) NOT NULL,
  nodeIdTo VARCHAR(64) NOT NULL,
  signalStrength INT,
  hops INT,
  lastSeen TIMESTAMP,
  FOREIGN KEY (nodeIdFrom) REFERENCES nodes(nodeId),
  FOREIGN KEY (nodeIdTo) REFERENCES nodes(nodeId),
  INDEX idx_nodeIdFrom (nodeIdFrom),
  INDEX idx_nodeIdTo (nodeIdTo),
  UNIQUE KEY unique_connection (nodeIdFrom, nodeIdTo)
);

-- Insert default group
INSERT INTO groups (groupId, name, description, permissions) VALUES 
  ('default', 'Default Group', 'Default user group', JSON_ARRAY('view_dashboard', 'view_nodes', 'edit_pages'));
