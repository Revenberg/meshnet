/**
 * Nodes Router
 * API routes for node management and connections
 */

const express = require('express');
const router = express.Router();
const nodeConnections = require('./node-connections');

/**
 * Log a team connection to a node
 * POST /api/nodes/connection
 */
router.post('/connection', nodeConnections.logConnection);

/**
 * Get connections for a specific node
 * GET /api/nodes/:nodeId/connections
 */
/**
 * Get connection statistics
 * GET /api/nodes/stats
 */
router.get('/stats/connections', nodeConnections.getConnectionStats);

/**
 * Get connections for a specific node
 * GET /api/nodes/:nodeId/connections
 */
router.get('/:nodeId/connections', nodeConnections.getNodeConnections);

/**
 * Display node connections dashboard
 * GET /dashboard/nodes
 */
router.get('/dashboard', nodeConnections.displayNodeConnectionsDashboard);

module.exports = router;
