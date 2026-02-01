/**
 * MeshNet SSO (Single Sign-On) Router
 * Supports OAuth2/OpenID Connect providers: Google, GitHub, Azure AD
 * 
 * Features:
 * - Multi-provider OAuth2 authentication
 * - Automatic user provisioning
 * - JWT token generation
 * - Role-based access control
 * - Device linking to user accounts
 */

const express = require('express');
const router = express.Router();
const jwt = require('jsonwebtoken');
const { v4: uuidv4 } = require('uuid');

let dbPool;

// Initialize database pool
async function initDbPool() {
  if (!dbPool) {
    const mysql = require('mysql2/promise');
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

// ============ HELPER FUNCTIONS ============

/**
 * Generate JWT token for authenticated user
 */
function generateToken(userId, username, role = 'user') {
  const secret = process.env.JWT_SECRET || 'meshnet-secret-key-change-in-prod';
  return jwt.sign(
    {
      userId,
      username,
      role,
      iat: Math.floor(Date.now() / 1000)
    },
    secret,
    { expiresIn: '7d' }
  );
}

/**
 * Verify JWT token
 */
function verifyToken(token) {
  try {
    const secret = process.env.JWT_SECRET || 'meshnet-secret-key-change-in-prod';
    return jwt.verify(token, secret);
  } catch (error) {
    return null;
  }
}

/**
 * Create or update user from OAuth provider data
 */
async function provisionUser(provider, providerData) {
  const pool = await initDbPool();
  const providerId = `${provider}:${providerData.id}`;
  
  try {
    // Check if user already exists
    const [existingUsers] = await pool.query(
      'SELECT * FROM sso_users WHERE providerUserId = ?',
      [providerId]
    );

    if (existingUsers.length > 0) {
      // Update last login
      const user = existingUsers[0];
      await pool.query(
        'UPDATE sso_users SET lastLogin = NOW(), lastSeen = NOW() WHERE userId = ?',
        [user.userId]
      );
      return user;
    }

    // Create new user
    const userId = uuidv4();
    const username = providerData.name || providerData.email || `${provider}-user-${Date.now()}`;
    const email = providerData.email || `${provider}-${Date.now()}@meshnet.local`;

    await pool.query(
      `INSERT INTO sso_users (userId, username, email, provider, providerUserId, avatarUrl, role, createdAt, lastLogin)
       VALUES (?, ?, ?, ?, ?, ?, 'user', NOW(), NOW())`,
      [userId, username, email, provider, providerId, providerData.avatarUrl || null]
    );

    const [[newUser]] = await pool.query(
      'SELECT * FROM sso_users WHERE userId = ?',
      [userId]
    );

    return newUser;
  } catch (error) {
    console.error('[SSO] User provisioning error:', error);
    throw error;
  }
}

// ============ OAUTH2 CALLBACK HANDLERS ============

/**
 * Google OAuth2 callback
 * POST /auth/sso/callback/google
 * Body: { idToken: string }
 */
router.post('/sso/callback/google', async (req, res) => {
  try {
    const { idToken } = req.body;

    if (!idToken) {
      return res.status(400).json({ error: 'Missing idToken' });
    }

    // In production, verify token with Google
    // For now, decode and trust (should use google-auth-library in production)
    const decoded = jwt.decode(idToken);

    if (!decoded || !decoded.sub) {
      return res.status(401).json({ error: 'Invalid token' });
    }

    const user = await provisionUser('google', {
      id: decoded.sub,
      email: decoded.email,
      name: decoded.name,
      avatarUrl: decoded.picture
    });

    const token = generateToken(user.userId, user.username, user.role);

    res.json({
      status: 'authenticated',
      token,
      user: {
        userId: user.userId,
        username: user.username,
        email: user.email,
        role: user.role,
        avatarUrl: user.avatarUrl
      }
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

/**
 * GitHub OAuth2 callback
 * POST /auth/sso/callback/github
 * Body: { accessToken: string }
 */
router.post('/sso/callback/github', async (req, res) => {
  try {
    const { accessToken } = req.body;

    if (!accessToken) {
      return res.status(400).json({ error: 'Missing accessToken' });
    }

    // Fetch user data from GitHub
    const githubResponse = await fetch('https://api.github.com/user', {
      headers: { Authorization: `Bearer ${accessToken}` }
    });

    if (!githubResponse.ok) {
      return res.status(401).json({ error: 'Invalid GitHub token' });
    }

    const githubUser = await githubResponse.json();

    const user = await provisionUser('github', {
      id: githubUser.id.toString(),
      email: githubUser.email || githubUser.login + '@github.com',
      name: githubUser.name || githubUser.login,
      avatarUrl: githubUser.avatar_url
    });

    const token = generateToken(user.userId, user.username, user.role);

    res.json({
      status: 'authenticated',
      token,
      user: {
        userId: user.userId,
        username: user.username,
        email: user.email,
        role: user.role,
        avatarUrl: user.avatarUrl
      }
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

/**
 * Azure AD OAuth2 callback
 * POST /auth/sso/callback/azure
 * Body: { idToken: string, accessToken: string }
 */
router.post('/sso/callback/azure', async (req, res) => {
  try {
    const { idToken } = req.body;

    if (!idToken) {
      return res.status(400).json({ error: 'Missing idToken' });
    }

    // In production, verify with Azure
    const decoded = jwt.decode(idToken);

    if (!decoded || !decoded.oid) {
      return res.status(401).json({ error: 'Invalid token' });
    }

    const user = await provisionUser('azure', {
      id: decoded.oid,
      email: decoded.upn || decoded.email,
      name: decoded.name,
      avatarUrl: null
    });

    const token = generateToken(user.userId, user.username, user.role);

    res.json({
      status: 'authenticated',
      token,
      user: {
        userId: user.userId,
        username: user.username,
        email: user.email,
        role: user.role,
        avatarUrl: user.avatarUrl
      }
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// ============ TOKEN VERIFICATION ============

/**
 * Verify JWT token validity
 * GET /auth/verify
 * Header: Authorization: Bearer <token>
 */
router.get('/verify', (req, res) => {
  const authHeader = req.headers.authorization;

  if (!authHeader || !authHeader.startsWith('Bearer ')) {
    return res.status(401).json({ error: 'Missing or invalid Authorization header' });
  }

  const token = authHeader.substring(7);
  const decoded = verifyToken(token);

  if (!decoded) {
    return res.status(401).json({ error: 'Invalid or expired token' });
  }

  res.json({
    status: 'valid',
    user: {
      userId: decoded.userId,
      username: decoded.username,
      role: decoded.role
    }
  });
});

/**
 * Refresh JWT token
 * POST /auth/refresh
 * Body: { token: string }
 */
router.post('/refresh', (req, res) => {
  const { token } = req.body;

  if (!token) {
    return res.status(400).json({ error: 'Missing token' });
  }

  const decoded = verifyToken(token);

  if (!decoded) {
    return res.status(401).json({ error: 'Invalid or expired token' });
  }

  const newToken = generateToken(decoded.userId, decoded.username, decoded.role);

  res.json({
    status: 'refreshed',
    token: newToken
  });
});

// ============ USER PROFILE ============

/**
 * Get current user profile
 * GET /auth/profile
 * Header: Authorization: Bearer <token>
 */
router.get('/profile', async (req, res) => {
  try {
    const authHeader = req.headers.authorization;

    if (!authHeader || !authHeader.startsWith('Bearer ')) {
      return res.status(401).json({ error: 'Missing or invalid Authorization header' });
    }

    const token = authHeader.substring(7);
    const decoded = verifyToken(token);

    if (!decoded) {
      return res.status(401).json({ error: 'Invalid or expired token' });
    }

    const pool = await initDbPool();
    const [[user]] = await pool.query(
      'SELECT userId, username, email, role, avatarUrl, createdAt FROM sso_users WHERE userId = ?',
      [decoded.userId]
    );

    if (!user) {
      return res.status(404).json({ error: 'User not found' });
    }

    res.json({
      status: 'found',
      user
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// ============ LOGOUT ============

/**
 * Logout (invalidate token on client side)
 * POST /auth/logout
 */
router.post('/logout', (req, res) => {
  // In production, you might want to maintain a token blacklist
  // For now, just confirm logout
  res.json({
    status: 'logged_out',
    message: 'Please remove your token from client storage'
  });
});

module.exports = router;
