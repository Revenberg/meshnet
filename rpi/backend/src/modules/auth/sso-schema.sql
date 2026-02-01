/**
 * SSO Users Table Schema
 * Stores users created through OAuth2/OpenID Connect providers
 */

CREATE TABLE IF NOT EXISTS sso_users (
  userId VARCHAR(36) PRIMARY KEY NOT NULL,
  
  -- User information
  username VARCHAR(255) NOT NULL UNIQUE,
  email VARCHAR(255) NOT NULL UNIQUE,
  
  -- OAuth provider info
  provider ENUM('google', 'github', 'azure', 'local') NOT NULL,
  providerUserId VARCHAR(500) UNIQUE,  -- Unique ID from provider (e.g., "google:12345")
  
  -- User metadata
  avatarUrl VARCHAR(500),
  role ENUM('admin', 'moderator', 'user') DEFAULT 'user',
  
  -- Tracking
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  lastLogin TIMESTAMP,
  lastSeen TIMESTAMP,
  
  -- Soft delete (optional)
  deletedAt TIMESTAMP NULL,
  
  INDEX idx_provider (provider, providerUserId),
  INDEX idx_email (email),
  INDEX idx_createdAt (createdAt)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/**
 * User Device Associations
 * Links users to their devices (ESP32 nodes)
 */

CREATE TABLE IF NOT EXISTS user_device_associations (
  associationId VARCHAR(36) PRIMARY KEY NOT NULL,
  
  -- Foreign keys
  userId VARCHAR(36) NOT NULL,
  nodeId VARCHAR(255) NOT NULL,  -- ESP32 node identifier
  
  -- Association metadata
  deviceName VARCHAR(255),
  role ENUM('owner', 'shared', 'guest') DEFAULT 'owner',
  
  -- Tracking
  associatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  lastAccessedAt TIMESTAMP,
  
  FOREIGN KEY (userId) REFERENCES sso_users(userId) ON DELETE CASCADE,
  FOREIGN KEY (nodeId) REFERENCES nodes(nodeId) ON DELETE CASCADE,
  
  UNIQUE KEY unique_user_device (userId, nodeId),
  INDEX idx_nodeId (nodeId),
  INDEX idx_userId (userId)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/**
 * SSO Sessions (Optional - for session management)
 * Tracks active SSO sessions
 */

CREATE TABLE IF NOT EXISTS sso_sessions (
  sessionId VARCHAR(36) PRIMARY KEY NOT NULL,
  userId VARCHAR(36) NOT NULL,
  
  -- Session info
  provider ENUM('google', 'github', 'azure') NOT NULL,
  refreshToken VARCHAR(500),
  
  -- Tracking
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  expiresAt TIMESTAMP,
  lastActivityAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  
  FOREIGN KEY (userId) REFERENCES sso_users(userId) ON DELETE CASCADE,
  INDEX idx_userId (userId),
  INDEX idx_expiresAt (expiresAt)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/**
 * Audit Log (Optional - for security tracking)
 * Tracks all authentication events
 */

CREATE TABLE IF NOT EXISTS auth_audit_log (
  auditId VARCHAR(36) PRIMARY KEY NOT NULL,
  userId VARCHAR(36),
  
  -- Event info
  eventType ENUM('login', 'logout', 'token_refresh', 'device_link', 'device_unlink', 'permission_change') NOT NULL,
  provider ENUM('google', 'github', 'azure', 'local'),
  status ENUM('success', 'failure') DEFAULT 'success',
  
  -- Request info
  ipAddress VARCHAR(45),
  userAgent VARCHAR(500),
  
  -- Metadata
  metadata JSON,
  
  -- Tracking
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  
  FOREIGN KEY (userId) REFERENCES sso_users(userId) ON DELETE SET NULL,
  INDEX idx_userId (userId),
  INDEX idx_eventType (eventType),
  INDEX idx_createdAt (createdAt)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
