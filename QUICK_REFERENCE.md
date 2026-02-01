# MeshNet Quick Reference Guide

## 🚀 Getting Started

### For Node Development (ESP32/Arduino)

**File Locations:**
```
MeshNet/node/
├── mesh_node.ino           # Main sketch - start here
├── include/Config.h        # All hardware settings
├── include/Protocol.h      # Message definitions
├── src/                    # Implementation (to add)
└── boards/                 # Board configurations
```

**Quick Configure:**
1. Edit `include/Config.h`:
   - `#define LORA_BAND 915E6` (US) or `868E6` (EU)
   - `#define WIFI_AP_PASSWORD "your_password"`

2. Flash to Heltec V3:
   - Arduino IDE → Tools → Board: "Heltec WiFi LoRa 32 V3"
   - Select correct COM port
   - Upload `mesh_node.ino`

### For RPI Setup

**One Command Setup:**
```bash
cd MeshNet
sudo chmod +x setup.sh
sudo ./setup.sh
```

**Manual Setup:**
```bash
cd MeshNet/rpi/docker
docker-compose up -d
```

**Access Points:**
- Dashboard: `http://<rpi-ip>`
- API: `http://<rpi-ip>:3001`
- Database: `<rpi-ip>:3306`

---

## 📡 Protocol Quick Reference

### Message Types (7 Total)

| Type | From | To | Frequency | Purpose |
|------|------|----|-----------|---------| 
| DISCOVER | Node | RPI | Startup + 5min | Node registration |
| STATUS | Node | RPI | 10sec | Health report |
| RELAY | Node | Node | On-demand | Message forwarding |
| CONFIG | RPI | Node | On-change | Settings update |
| AUTH | RPI | Node | On-login | User auth sync |
| PAGE | RPI | Node | On-update | Content delivery |
| KEEPALIVE | Bi-dir | Bi-dir | 5sec | Heartbeat |

### Node ID Format
```
MAC_ADDRESS:FUNCTIONAL_NAME

Example: AA:BB:CC:DD:EE:FF:kitchen_sensor
         └─ Unique per node   └─ Custom friendly name
```

### Basic Message Structure
```json
{
  "type": "DISCOVER|STATUS|RELAY|CONFIG|AUTH|PAGE|KEEPALIVE",
  "from": "AA:BB:CC:DD:EE:FF:name",
  "to": "BROADCAST|NODE_ID",
  "msgId": "unique_id",
  "timestamp": 1234567890,
  "data": { /* type-specific */ }
}
```

---

## 🔧 Configuration Quick Reference

### Node Settings (Config.h)

**LoRa Parameters:**
```cpp
#define LORA_BAND 915E6              // US: 915MHz, EU: 868MHz
#define LORA_SPREADING_FACTOR 10     // 7-12 (higher = range, slower)
#define LORA_TX_POWER 17             // 2-17 dBm
```

**Network Settings:**
```cpp
#define MESH_BROADCAST_INTERVAL 300000  // 5 minutes
#define MESH_KEEPALIVE_INTERVAL 5000    // 5 seconds
#define MESH_MAX_TTL 10                 // Max relay hops
```

**Display:**
```cpp
#define DISPLAY_UPDATE_INTERVAL 1000   // Update every 1 sec
#define DISPLAY_BRIGHT_TIME 30000      // Keep bright for 30 sec
```

### RPI Settings (docker/.env)

```bash
# Database
DB_HOST=mysql
DB_USER=meshnet
DB_PASSWORD=meshnet_secure_pwd

# JWT Auth
JWT_SECRET=your_secret_here

# Environment
NODE_ENV=production
```

---

## 📊 Database Quick Reference

### Main Tables

**users**
- userId (UUID)
- username, email
- passwordHash
- groupId (foreign key)

**groups**
- groupId (UUID)
- name, description
- permissions (JSON)

**nodes**
- nodeId (MAC:name)
- macAddress, functionalName
- lastSeen, battery%, rssi
- connectedNodes count

**pages**
- pageId (UUID)
- nodeId, groupId
- title, content (HTML)
- imageUrl, refreshInterval

**Sessions** (user_sessions)
- Track active logins on nodes
- Token expiry handling

**Topology** (node_connections)
- Track which nodes can reach which
- Signal strength & hop count

---

## 🔌 API Quick Reference

### Nodes API
```
GET    /api/nodes              # List all nodes
GET    /api/nodes/:nodeId      # Get specific node
POST   /api/nodes/:nodeId/update  # Update node info
```

### Users API
```
GET    /api/users              # List all users
POST   /api/users              # Create user
```

### Groups API
```
GET    /api/groups             # List groups
POST   /api/groups             # Create group
```

### Pages API
```
GET    /api/pages/:nodeId      # Pages for node
POST   /api/pages              # Create page
```

### Status
```
GET    /api/topology           # Network topology
GET    /health                 # Health check
```

---

## 🛠️ Development Commands

### Node Development
```bash
cd MeshNet/node
# Open in Arduino IDE
# Select: Heltec WiFi LoRa 32 V3
# Verify (Ctrl+R)
# Upload (Ctrl+U)
```

### Backend Development
```bash
cd MeshNet/rpi/backend
npm install
npm start  # Runs on :3001
npm run dev  # With auto-reload
```

### Webserver Development
```bash
cd MeshNet/rpi/webserver
npm install
npm start  # Runs on :80
npm run dev  # With auto-reload
```

### Docker Commands
```bash
cd MeshNet/rpi/docker

# Start all services
docker-compose up -d

# View logs
docker-compose logs -f
docker-compose logs backend
docker-compose logs webserver

# Stop services
docker-compose down

# Rebuild images
docker-compose build --no-cache

# Check status
docker-compose ps

# Access MySQL directly
docker-compose exec mysql mysql -u meshnet -p meshnet
```

---

## 🐛 Troubleshooting Quick Tips

### Node Issues

**Problem:** Node not visible in RPI dashboard
```
1. Check USB bridge is connected: lsusb
2. Check LoRa frequency matches
3. Monitor node serial output (115200 baud)
4. Check battery level on display
```

**Problem:** WiFi AP not appearing
```
1. Verify WiFi AP enabled in Config.h
2. Check SSID setting: "MeshNode-AABBCCDDEE"
3. Power cycle node
4. Check for compile errors
```

### RPI Issues

**Problem:** Containers won't start
```bash
# Check Docker
docker ps -a
docker logs meshnet-backend
docker logs meshnet-webserver

# Rebuild
docker-compose build --no-cache
docker-compose up -d
```

**Problem:** Database connection failed
```bash
# Check MySQL
docker-compose logs mysql

# Test connection
docker-compose exec mysql mysql -u meshnet -p

# Reset database
docker-compose exec mysql mysql -u root -p
# Then: DROP DATABASE meshnet; CREATE DATABASE meshnet;
```

**Problem:** Port already in use
```bash
# Find what's using port
lsof -i :80        # Webserver
lsof -i :3001      # Backend
lsof -i :3306      # MySQL

# Kill process
kill -9 <PID>
```

---

## 📋 Daily Workflow

### Monitoring Network
```bash
# SSH to RPI
ssh pi@<rpi-ip>

# Check services
docker-compose ps
docker-compose logs -f

# View database
docker-compose exec mysql mysql -u meshnet -p meshnet
SELECT * FROM nodes;
SELECT * FROM user_sessions;
```

### Adding New User
```bash
# Via API
curl -X POST http://<rpi-ip>:3001/api/users \
  -H "Content-Type: application/json" \
  -d '{"username":"john","email":"john@example.com","password":"pass","groupId":1}'

# Or via Dashboard: Users → Add User
```

### Creating Node Page
1. Open Dashboard
2. Go to Pages → Create New Page
3. Select Node & Group
4. Edit HTML content
5. Upload image (optional)
6. Save → Automatically synced to node

### Naming a Node
1. In Dashboard: Nodes → Select node
2. Enter Functional Name
3. Save → Node will reboot and update

---

## 📚 Documentation Map

| Document | Purpose |
|----------|---------|
| README.md | Overview & quick start |
| PROJECT_PLAN.md | 7-week development timeline |
| PROJECT_STRUCTURE.md | Complete file listing |
| SETUP_CHECKLIST.md | Phase completion verification |
| PROTOCOL_SPEC.md | Message formats & routing |
| Config.h | Node configuration reference |
| Protocol.h | Data structures documentation |

---

## 🎯 Success Indicators

### Phase 1: ✅ Complete
- [x] All files created
- [x] Documentation written
- [x] Architecture defined

### Phase 2: Ready to Start
- [ ] Node compiles (0 errors)
- [ ] Display shows node name
- [ ] WiFi AP active
- [ ] Connect to node from phone

### Phase 3: Next Phase
- [ ] Backend starts without errors
- [ ] MySQL populated
- [ ] API endpoints working

### Phase 4: Integration
- [ ] Dashboard loads
- [ ] Real-time updates
- [ ] User management works
- [ ] Pages synced to nodes

---

## 📞 Quick Help

**Where to find...**
- Node configuration → `node/include/Config.h`
- Message formats → `protocol/PROTOCOL_SPEC.md`
- API documentation → `rpi/backend/src/server.js`
- Database schema → `rpi/docker/mysql.sql`
- Dashboard code → `rpi/webserver/server.js`
- Network setup → `rpi/network/setup-network.sh`

**How to...**
- Flash a node → See NODE_SETUP.md (to be created)
- Setup RPI → Run `setup.sh` or see RPI_SETUP.md (to be created)
- Add new API endpoint → Extend `rpi/backend/src/server.js`
- Create new page template → Add to `rpi/webserver/public/pages/`
- Change protocol message → Edit `protocol/PROTOCOL_SPEC.md` & `node/include/Protocol.h`

---

**Last Updated**: 28-01-2026  
**Status**: Foundation Phase Complete ✅  
**Next**: Node Development Phase 🚀
