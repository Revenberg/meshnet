# Node Connections Tracking - Implementation Complete

## Overview
This implementation enables the MeshNet system to track which teams and users have connected to each node, with timestamps. LoRa mesh network relays login data through the Heltec gateway to the Docker API for centralized tracking.

## Archit001
- Receives page data and user data from gateway
- Uses node's MAC address as unique nodeId

**Files Modified:**
- `heltec_lora_game/lora_node/NodeWebServer.cpp`
- `game_on/arduino/lora_node/NodeWebServer.cpp`

**Key Function Added:**
```cpp
void sendLoginViaLora(const String &user, const String &pass)
{
    String nodeId = WiFi.macAddress();
    // Create LoRa message for gateway
    String loraMsg = "LOGIN|" + nodeId + "|" + user + "|" + pass;
    
    // Send to Heltec gateway via LoRa (non-blocking)
    LoraNode::loraSend({
        msgId: uuidv4(),
        user: "node",
        TTL: 1,
        object: "auth",
        function: "login",
        parameters: loraMsg
    });
}
```

**Login Handler Updated:**
- Captures user and password on successful login
- Calls `sendLoginViaLora(user, password)`
- Logs to serial: `[INFO] Login: {user} sent via LoRa to gateway`
- Waits for response from gateway with user/page data

### 2. **Heltec Gateway** (LoRa Relay)
Acts as bridge between mesh nodes and Docker backend:
- Receives login messages from nodes via LoRa
- Extracts username, password, nodeId
- Retrieves team info (uses password to verify)
- Forwards connection data to Docker API: `POST /api/nodes/connection`
- Sends page data and user list back to requesting node

**Message Flow:**
```
Node A (LoRa) → Heltec Gateway (USB) → Docker Backend (HTTP)
                                    ↓
Node A (LoRa) ← Heltec Gateway (USB) ← Docker Backend (HTTP)
```

### 2. **Docker Backend API** (Node.js/Express)
Handles connection logging and data distribution:

**File Modified:** `MeshNet/rpi/backend/src/modules/nodes/node-connections.js`

#### POST `/api/nodes/connection`
- **Purpose**: Receive login data from Heltec gateway
- **Body Parameters**: 
  - `nodeId` (string - MAC address)
  - `username` (string)
  - `password` (string - for team lookup)
- **Actions**:
  - Generates unique `connectionId` (UUID)
  - Looks up team from password hash
  - Auto-creates node record if it doesn't exist
  - Inserts connection record into `node_connections` table
  - Returns user list and page data for gateway to relay back to node
  - Returns: `{ status: 'logged', connectionId, users: [...], pages: [...], timestamp }`

#### GET `/api/nodes/:nodeId/data`
- **Purpose**: Get all user and page data for a node
- **Response**: 
  ```json
  {
    "nodeId": "XX:XX:XX:XX:XX:XX",
    "users": [
      {
        "username": "alice",
        "team": "Red Team",
        "hash": "sha256hash..."
      }
    ],
    "pages": [
      {
        "id": "page1",
        "title": "Welcome",
        "content": "...",
        "refreshInterval": 30
      }
    ]
  }
  ```

### 3. **Heltec Gateway** (C++ Firmware)
Acts as LoRa-to-HTTP bridge:

**Responsibilities:**
- Listen for LoRa messages from nodes
- Parse login requests: `LOGIN|nodeId|username|password`
- Forward to Docker API endpoint: `POST /api/nodes/connection`
- Request full user/page dataset if needed
- Parse response with user list and pages
- Send back to requesting node via LoRa (non-blocking)

**LoRa Message Format:**
```
From Node: LOGIN|AA:BB:CC:DD:EE:FF|alice|password123
From Gateway: AUTHOK|data_json|user_list_json|pages_json
```

### 3. **Database Schema**
New table stores all connection history:

**File Modified:** `MeshNet/rpi/docker/mysql.sql`

```sql
CREATE TABLE IF NOT EXISTS node_connections (
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

**Columns:**
- `id`: Auto-incremented primary key
- `connectionId`: Unique UUID for each login event
- `nodeId`: Foreign key to nodes table (MAC address)
- `username`: Who logged in
- `teamName`: Which team they belong to
- `connectedAt`: When they first connected
- `lastSeen`: Last activity timestamp (auto-updated)

**Indexes:**
- `idx_nodeId`: For querying connections per node
- `idx_teamName`: For querying by team
- `idx_connectedAt`: For time-based queries

### 4. **Docker Webserver Dashboard**
New HTML page displays all node connections:

**File Modified:** `MeshNet/rpi/backend/src/server.js`

#### GET `/node-connections`
- **Purpose**: Display beautiful HTML dashboard of node connections
- **Features**:
  - Shows all active nodes in a grid layout
  - For each node, lists all teams/users who connected
  - Displays last connection timestamp
  - Color-coded team badges
  - Responsive design with gradient background
  - Shows "No connections" message for idle nodes

**Access**: Visit `http://localhost:3001/node-connections` in browser

**Page Design:**
```
╔═══════════════════════════════════════╗
║  🌐 Node Team Connections Dashboard   ║
╠═══════════════════════════════════════╣
║  ┌─────────────────────────────────┐  ║
║  │ 📍 Node Name                    │  ║
║  │ Node ID: XX:XX:XX:XX:XX:XX      │  ║
║  ├─────────────┬──────┬──────────┤  ║
║  │ Team        │ User │ Last Con │  ║
║  ├─────────────┼──────┼──────────┤  ║
║  │ [Team A]    │ user │ 14:45:00 │  ║
║  │ [Team B]    │ user │ 12:30:00 │  ║
║  └─────────────┴──────┴──────────┘  ║
║  ┌─────────────────────────────────┐  ║
║  │ 📍 Another Node                 │  ║
║  │ No team connections recorded    │  ║
║  └─────────────────────────────────┘  ║
╚═══════════════════════════════════════╝
```

## Data Flow

### Successful Login Flow:
```
1. User enters credentials on mesh node
   ↓
2. Node validates against local User database (optional check)
   ↓
3. Node creates LoRa message: LOGIN|nodeId|username|password
   ↓
4. Message relayed via LoRa through gateway
   ↓
5. Heltec Gateway receives LoRa message
   ↓
6. Gateway sends HTTP POST to Docker API: /api/nodes/connection
   {
     "nodeId": "AA:BB:CC:DD:EE:FF",
     "username": "alice",
     "password": "hash"
   }
   ↓
7. Docker API:
   - Verifies password hash
   - Looks up team from user database
   - Inserts into node_connections table
   - Returns user list and page data
   ↓
8. Heltec Gateway receives API response
   ↓
9. Gateway forwards user/page data back to node via LoRa
   ↓
10. Node receives data, updates local User cache and displays dashboard
    (Session valid for 24h)
```

### Data Synchronization Flow:
```
Gateway → Node (on login)
├─ User list with passwords (for validation)
├─ Page data (for display)
└─ Team assignments (for tracking)

Node → Gateway (on login)
├─ nodeId (MAC address)
├─ username
├─ password (for verification)
└─ Connection info (logged at Docker)
```

## Deployment Steps

### Step 1: Update Database Schema
```bash
# Copy mysql.sql with new node_connections table to docker volume
docker cp ./rpi/docker/mysql.sql meshnet_mysql:/
docker exec meshnet_mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet < /mysql.sql
```

### Step 2: Deploy Updated Backend
```bash
# Restart Docker backend to load new routes
cd ./rpi/docker
docker-compose up -d --build backend
```

### Step 3: Deploy Updated Firmware
```bash
# Compile both ESP32 projects
cd heltec_lora_game/lora_node
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3

cd game_on/arduino/lora_node
arduino-cli compile --fqbn esp32:esp32:heltec_wifi_lora_32_V3

# Upload to devices
arduino-cli upload --port COM5 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 heltec_lora_game/lora_node
arduino-cli upload --port COM7 --fqbn esp32:esp32:heltec_wifi_lora_32_V3 game_on/arduino/lora_node
```

## Testing the Implementation

### Test 1: Check Serial Output
When user logs in, you should see:
```
[INFO] Login successful: alice | Team: Red Team
[API] Sent connection data to Docker backend for user: alice
```

### Test 2: Check Database
```bash
docker exec meshnet_mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet -e "SELECT * FROM node_connections LIMIT 5;"
```

Expected output:
```
| id | connectionId | nodeId | username | teamName | connectedAt | lastSeen |
|----|--------------|--------|----------|----------|-------------|----------|
| 1  | uuid-xxx     | AA:BB  | alice    | Red Team | 2024-01-15  | 2024-01-15 |
```

### Test 3: View Dashboard
Open browser: `http://192.168.1.100:3001/node-connections`
- Should list all active nodes
- For each node, show teams that connected
- Show last connection timestamp

## API Endpoints Reference

### Node Connections API
```
POST   /api/host/node/:nodeId/connection      <- Called by ESP32 on login
GET    /api/host/node/:nodeId/connections     <- Get connections for a node
```

### Dashboard
```
GET    /node-connections                       <- HTML dashboard view
```

## Troubleshooting

### Connection Data Not Appearing
1. **Check ESP32 Serial Output**
   - Should show `[API] Sent connection data...`
   - If shows `[API] Failed to connect...`: Network issue

2. **Check Docker API Logs**
   ```bash
   docker logs meshnet_backend | grep -i "connection\|api"
   ```

3. **Check Database Connection**
   ```bash
   docker exec meshnet_mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet -e "SHOW TABLES;"
   ```

### API Returns 404
- Confirm backend is running: `docker ps | grep backend`
- Check port 3001 is accessible from ESP32
- Verify `node-router.js` is loaded in `server.js`

### Dashboard Shows No Data
- Check database table exists: `SHOW TABLES LIKE 'node_connections';`
- Check nodes table has records: `SELECT COUNT(*) FROM nodes;`
- Check for SQL errors: `docker logs meshnet_mysql`

## Benefits

✅ **Mesh Network**: Uses LoRa for communication (works without WiFi)
✅ **Gateway Relay**: Heltec gateway bridges mesh to cloud
✅ **Data Sync**: Nodes receive user/page updates from server
✅ **Track User Engagement**: See which teams use which nodes
✅ **Network Health**: Identify frequently used vs idle nodes
✅ **User Management**: Know who's connected where and when
✅ **Historical Data**: Full audit trail with timestamps
✅ **Real-time Dashboard**: Live view of node activity at Docker backend
✅ **Non-blocking**: LoRa messages don't block user login
✅ **Offline Capable**: Nodes can work without WiFi, sync when gateway available  

## Future Enhancements

- [ ] Add time-range filter to dashboard
- [ ] Export connection history to CSV
- [ ] Add analytics (most active teams, peak hours)
- [ ] Webhook notifications when team connects
- [ ] Mobile-friendly dashboard view
- [ ] Real-time updates using WebSocket

## Notes

- **Non-blocking**: ESP32 doesn't wait for API response (timeout is immediate)
- **Automatic Nodes**: If nodeId doesn't exist in `nodes` table, it's created automatically
- **UUID Generation**: Uses Node.js `uuid` package (ensure installed)
- **Timestamps**: Uses MySQL CURRENT_TIMESTAMP for consistency
- **Update Frequency**: `lastSeen` updates on each login attempt (even failed ones)

---

**Implementation Date**: January 2024  
**Last Updated**: January 29, 2026

## Version History

- **V0.7.2** (Jan 29, 2026): Added broadcast message display frame - messages now visible on node OLED displays
- **V0.7.1** (Jan 29, 2026): Initial firmware deployment  
- **V0.7.0** (Jan 2024): LoRa mesh and node connection tracking

## Deployment Status

| Component | Status | Version | Notes |
|-----------|--------|---------|-------|
| **COM5 (Heltec Gateway)** | ✅ **DEPLOYED** | V0.7.2 | Broadcast messages now displayed on screen |
| **COM6 (Game Node)** | ✅ **DEPLOYED** | V0.7.2 | Broadcast messages now displayed on screen |
| **Docker Backend** | ✅ READY | Latest | All API endpoints configured and running |
| **Database Schema** | ✅ READY | Latest | node_connections table created |

**Current Status**: Both gateway (COM5) and game nodes are operational with V0.7.2 firmware. Broadcast messages are now visible on node displays as a new 4th frame in the display rotation (WiFi → Battery → Nodes → Broadcast).

## What's New in V0.7.2

✅ **Broadcast Message Display (FIXED)**
- Added new display frame showing latest broadcast messages
- Messages show: sender name, message content (truncated), TTL remaining
- Rotates through 4 display frames every 30 seconds:
  1. WiFi Network Name
  2. Battery Percentage
  3. Connected Nodes Count
  4. **Latest Broadcast Message** (NEW)

**Frame 4 Broadcast Display**:
- Shows: 📢 Broadcast
- Sender: [username]
- Message content (truncated to 30 chars for OLED)
- TTL: [seconds remaining]
- Falls back to "No broadcasts" if none received

**Implementation Details**:
- Uses `LoraNode::getMessages()` to access message buffer
- Displays most recent message (index: msgWriteIndex - 1)
- Automatically truncates content for OLED compatibility
- Updates live as new messages arrive via LoRa
- Fully integrated with existing mesh relay system

**Files Modified**:
- `heltec_lora_game/lora_node/node_display.cpp` - Added frame4 callback
- `heltec_lora_game/lora_node/node_display.h` - Updated frame array to 4 items
- `game_on/arduino/lora_node/node_display.cpp` - Replaced version frame with broadcast
- `game_on/arduino/lora_node/NodeWebServer.cpp` - Version string updated  
