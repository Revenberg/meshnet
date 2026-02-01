# Nodes Module Documentation

## Overview
The Nodes Module handles all node-related functionality, including connection tracking, team management, and dashboard displays.

## Module Structure
```
rpi/backend/src/modules/nodes/
├── node-connections.js       # Core connection logic
├── router.js                  # API routes
└── README.md                  # This file
```

## API Endpoints

### 1. Log Team Connection
**Endpoint:** `POST /api/nodes/connection`

When an ESP32 node successfully authenticates a user, it sends connection data to this endpoint.

**Request Body:**
```json
{
  "nodeId": "XX:XX:XX:XX:XX:XX",
  "username": "alice",
  "teamName": "Red Team"
}
```

**Response:**
```json
{
  "status": "logged",
  "connectionId": "uuid-xxxxx",
  "timestamp": "2024-01-29T10:30:00.000Z"
}
```

### 2. Get Node Connections
**Endpoint:** `GET /api/nodes/:nodeId/connections`

Retrieve all team connections for a specific node.

**Response:**
```json
{
  "nodeId": "XX:XX:XX:XX:XX:XX",
  "connections": [
    {
      "teamName": "Red Team",
      "username": "alice",
      "firstSeen": "2024-01-29 10:00:00",
      "lastSeen": "2024-01-29 14:30:00"
    },
    {
      "teamName": "Blue Team",
      "username": "bob",
      "firstSeen": "2024-01-29 11:00:00",
      "lastSeen": "2024-01-29 13:45:00"
    }
  ]
}
```

### 3. Get Connection Statistics
**Endpoint:** `GET /api/nodes/stats/connections`

Get overall connection statistics.

**Response:**
```json
{
  "totalConnections": 42,
  "uniqueTeams": 5,
  "connectionsPerNode": [
    {
      "nodeId": "AA:BB:CC:DD:EE:FF",
      "connectionCount": 12,
      "uniqueTeams": 3
    }
  ]
}
```

## Dashboard

### Access Dashboard
**URL:** `http://localhost:3001/dashboard/nodes`

The dashboard displays:
- **Statistics Bar:** Total nodes, connections, and unique teams
- **Node Cards:** Grid layout showing each node
- **Connection Tables:** For each node, lists all teams/users who connected
- **Timestamps:** Last connection time for each team

## Database Schema

### node_connections Table
```sql
CREATE TABLE node_connections (
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
```

## Integration with ESP32 Nodes

### On Node Login (NodeWebServer.cpp)

When a user logs in on an ESP32 node:

1. Node validates credentials locally
2. Node retrieves user's team: `String team = User::getUserTeamByName(user);`
3. Node sends HTTP POST to Docker API:
```cpp
void sendLoginToApi(const String &user, const String &team)
{
    String nodeId = WiFi.macAddress();
    String payload = "{\"username\":\"" + user + "\",\"teamName\":\"" + team + "\"}";
    WiFiClient client;
    
    if (client.connect("127.0.0.1", 3001)) {
        String path = "/api/nodes/" + nodeId + "/connection";
        client.print("POST " + path + " HTTP/1.1\r\n");
        client.print("Host: 127.0.0.1:3001\r\n");
        client.print("Content-Type: application/json\r\n");
        client.print("Content-Length: " + String(payload.length()) + "\r\n");
        client.print("Connection: close\r\n\r\n");
        client.print(payload);
        client.stop();
    }
}
```

## Server Integration

### In server.js
Register the nodes router:
```javascript
const nodesRouter = require('./modules/nodes/router');
app.use('/api/nodes', nodesRouter);
app.get('/dashboard/nodes', (req, res) => {
  const nodeConnections = require('./modules/nodes/node-connections');
  nodeConnections.displayNodeConnectionsDashboard(req, res);
});
```

## Data Flow

```
1. User logs in on ESP32 node
   ↓
2. Node sends: POST /api/nodes/connection
   { nodeId, username, teamName }
   ↓
3. API stores in node_connections table
   ↓
4. Dashboard queries and displays real-time data
```

## Features

✅ **Automatic Node Creation** - Creates node record if not exists
✅ **Non-blocking API Calls** - ESP32 doesn't wait for response
✅ **Team Tracking** - Records which team each user belongs to
✅ **Timestamp Tracking** - Tracks first and last connection time
✅ **Statistics** - Provides aggregate connection data
✅ **Real-time Dashboard** - HTML page with live connection data
✅ **Responsive Design** - Mobile-friendly interface

## Performance Considerations

- **Database Indexes:** Optimized for node, team, and date queries
- **Connection Pooling:** 5 concurrent connections with queue
- **Non-blocking:** ESP32 API calls don't block login process
- **Aggregation:** Dashboard uses GROUP BY for efficient queries

## Testing

### Test API Endpoint
```bash
curl -X POST http://localhost:3001/api/nodes/connection \
  -H "Content-Type: application/json" \
  -d '{"nodeId":"AA:BB:CC:DD:EE:FF","username":"testuser","teamName":"TestTeam"}'
```

### View Dashboard
```
Open browser: http://localhost:3001/dashboard/nodes
```

### Check Database
```bash
docker exec meshnet_mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet \
  -e "SELECT * FROM node_connections LIMIT 10;"
```

## Troubleshooting

**API returns 404:**
- Ensure router is registered in server.js
- Check port 3001 is accessible

**No data in dashboard:**
- Verify database table exists: `SHOW TABLES LIKE 'node_connections';`
- Check ESP32 serial output for API call logs
- Verify database connectivity: `docker logs meshnet_mysql`

**ESP32 API call fails:**
- Check WiFi connectivity
- Verify port 3001 is open
- Check Docker backend is running: `docker-compose logs backend`

---

**Last Updated:** January 29, 2026
**Version:** V0.7.1
**Location:** `/rpi/backend/src/modules/nodes/`
