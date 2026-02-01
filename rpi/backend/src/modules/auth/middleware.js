/**
 * JWT Authentication Middleware
 * Verifies bearer tokens and attaches user info to request
 */

const jwt = require('jsonwebtoken');

/**
 * Verify JWT token from Authorization header
 * Usage: router.use(verifyToken)
 */
function verifyToken(req, res, next) {
  const authHeader = req.headers.authorization;

  if (!authHeader || !authHeader.startsWith('Bearer ')) {
    return res.status(401).json({
      error: 'Missing or invalid Authorization header',
      code: 'NO_AUTH_HEADER'
    });
  }

  const token = authHeader.substring(7);
  const secret = process.env.JWT_SECRET || 'meshnet-secret-key-change-in-prod';

  try {
    const decoded = jwt.verify(token, secret);
    req.user = {
      userId: decoded.userId,
      username: decoded.username,
      role: decoded.role
    };
    next();
  } catch (error) {
    if (error.name === 'TokenExpiredError') {
      return res.status(401).json({
        error: 'Token expired',
        code: 'TOKEN_EXPIRED'
      });
    }
    res.status(401).json({
      error: 'Invalid token',
      code: 'INVALID_TOKEN'
    });
  }
}

/**
 * Verify user has specific role
 * Usage: router.get('/admin', verifyRole('admin'), ...)
 */
function verifyRole(requiredRole) {
  return (req, res, next) => {
    if (!req.user) {
      return res.status(401).json({ error: 'User not authenticated' });
    }

    const roleHierarchy = {
      admin: 3,
      moderator: 2,
      user: 1
    };

    const userLevel = roleHierarchy[req.user.role] || 0;
    const requiredLevel = roleHierarchy[requiredRole] || 0;

    if (userLevel < requiredLevel) {
      return res.status(403).json({
        error: 'Insufficient permissions',
        required: requiredRole,
        current: req.user.role
      });
    }

    next();
  };
}

/**
 * Optional authentication - doesn't fail if no token, but attaches user if present
 * Usage: router.get('/public', optionalAuth, ...)
 */
function optionalAuth(req, res, next) {
  const authHeader = req.headers.authorization;

  if (authHeader && authHeader.startsWith('Bearer ')) {
    const token = authHeader.substring(7);
    const secret = process.env.JWT_SECRET || 'meshnet-secret-key-change-in-prod';

    try {
      const decoded = jwt.verify(token, secret);
      req.user = {
        userId: decoded.userId,
        username: decoded.username,
        role: decoded.role
      };
    } catch (error) {
      // Token invalid but optional - continue
    }
  }

  next();
}

module.exports = {
  verifyToken,
  verifyRole,
  optionalAuth
};
