# MeshNet 0.9.4 Test Checklist

## Preconditions
- RPI Docker stack running from MeshNet/rpi/docker (ghostnet_web container)
- Heltec node flashed with MeshNet 0.9.4
- Node connected to RPI serial (gateway) or to PC for serial logs
- If gateway serial is not auto-detected, set SERIAL_PORT in rpi/docker/.env and restart lora-gateway
- Serial check (COM5/COM7) returned no data in 3s window (needs device output)
- Version check: missing versions for TEST:NODE:MAC:01, AA:BB:CC:DD:EE:FF, BB:CC:DD:EE:FF:AA, A4:C1:38:12:34:56

## A) Automated/Local Tests
- [x] lora-gateway unit tests: run `npm test` in rpi/lora-gateway (serialConfig tests)
- [x] backend tests: run `npm test` in rpi/backend (passes when no tests exist)

## B) Docker API Endpoint Tests (Backend)
### Health + Core
- [x] GET /health returns status ok + timestamp
- [x] GET /api/nodes returns list (empty allowed if no DB)
- [x] GET /api/nodes/:nodeId returns 404 for unknown id

### Users
- [x] GET /api/users returns list (empty allowed)
- [x] POST /api/users with username/password/groupId creates user
- [x] GET /api/users/:userId returns created user
- [x] PUT /api/users/:userId updates username/password/groupId/isActive
- [x] DELETE /api/users/:userId removes user

### Auth
- [x] POST /api/auth/login with valid credentials returns success + user
- [x] POST /api/auth/login with invalid credentials returns 401
- [x] DELETE /api/auth/logout/:userId returns success

### Sync for nodes
- [x] GET /api/sync/users returns { status, user_count, users[] }
- [x] GET /api/sync/pages returns { status, page_count, pages[] }

### ACK/Ping/Message
- [x] POST /api/acks stores ACK (msgId + nodeId required)
- [x] POST /api/pings stores ping (nodeId required)
- [x] POST /api/messages stores message log (nodeId required)

### Nodes write paths
- [x] POST /api/nodes creates a node (nodeId optional)
- [x] POST /api/nodes/register upserts node by nodeId
- [x] POST /api/nodes/:nodeId/update updates lastSeen and metrics
- [x] PUT /api/nodes/:nodeId updates node fields
- [x] DELETE /api/nodes/:nodeId removes node

## 1) API & Database
- [ ] Open http://<RPI-IP>/users and add a user (username, team, password)
- [ ] Open http://<RPI-IP>/pages and add HTML for that team
- [ ] Verify http://<RPI-IP>/api/sync/users returns JSON
- [ ] Verify http://<RPI-IP>/api/sync/pages returns JSON

## 2) Node Startup Sync
- [ ] Power on node
- [ ] Serial log shows `REQ;USERS;...` and `REQ;PAGES;...`
- [ ] RPI lora-gateway logs show receiving requests and sending `RESP;USERS;...` and `RESP;PAGES;...`
- [ ] Node serial log shows `Users/Pagina` sync OK
- [ ] Login page shows ✅ sync status

## 3) Login + Team Page
- [ ] Connect to WiFi SSID `MeshNet V0.9.4`
- [ ] Open http://192.168.3.1 and login with user created in DB
- [ ] Team page section shows HTML from /pages for that team

## 4) Broadcast
- [ ] Send broadcast from backend (or via serial `LORA_TX;BCAST;...`)
- [ ] Node display shows broadcast frame (sender + content)

## 5) Ping + ACK
- [ ] lora-gateway sends periodic ping when idle
- [ ] Node responds with `PONG;...`
- [ ] /messages page shows ACK entries
- [ ] Old ACKs (>2h) are cleaned up

## 6) Gateway Relay
- [ ] RPI container receives `LORA_RX;...` lines
- [ ] Node forwards `LORA_TX;...` lines from serial to LoRa

## 7) Regression
- [ ] Existing login still works
- [ ] Web UI loads without errors
- [ ] LoRa beacons update nodes table

## C) Heltec Device Function Tests (Firmware)
### Radio setup and identity
- [ ] Boot log shows SX1262 init OK
- [ ] Node name uses MAC + firmware version (LoRA_<MAC>_0.9.4)

### Sync behavior
- [ ] On boot, serial shows REQ;USERS and REQ;PAGES
- [ ] RESP;USERS triggers runtime user cache and sync OK
- [ ] RESP;PAGES stores team pages and sync OK

### LoRa RX/TX
- [ ] Received MSG/RESP packets logged with [LoRa RX]
- [ ] Raw TX via LORA_TX;... is transmitted and logged
- [ ] Broadcasts are stored and displayed

### ACK/PONG
- [ ] Targeted messages generate ACK;
- [ ] PING from gateway triggers PONG response

### Webserver
- [ ] SoftAP SSID is MeshNet V0.9.4
- [ ] Login page shows sync status banner
- [ ] Team page renders HTML from sync
