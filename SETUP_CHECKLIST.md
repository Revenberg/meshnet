# MeshNet Project Setup Checklist

## Phase 1: Project Structure ✅ COMPLETED

### Mappenstructuur
- [x] Root directory aangemaakt: `MeshNet/`
- [x] Node subsysteem: `node/` met src, include, boards
- [x] RPI subsysteem: `rpi/` met docker, backend, webserver
- [x] Protocol docs: `protocol/`
- [x] Documentatie: `documentation/`

### Core Files Aangemaakt
- [x] `PROJECT_PLAN.md` - Volledige projectplanning
- [x] `PROTOCOL_SPEC.md` - Communicatie protocol definitie
- [x] `README.md` - Project overzicht
- [x] Node header files: `Config.h`, `Protocol.h`
- [x] Node sketch: `mesh_node.ino`
- [x] RPI Docker setup: `docker-compose.yml`
- [x] RPI Backend: Node.js server met API
- [x] RPI Webserver: Dashboard applicatie
- [x] Database schema: `mysql.sql`
- [x] Setup scripts: `setup.sh`, `setup-network.sh`

## Phase 2: Node Software (TBD)

### Core Node Modules (to be implemented)
- [ ] `src/LoRaMesh.cpp` - Mesh networking logic
- [ ] `src/NodeDisplay.cpp` - OLED display driver
- [ ] `src/NodeWebServer.cpp` - WiFi AP & HTTP server
- [ ] `src/NodeStorage.cpp` - SPIFFS file system
- [ ] `src/NodeAuth.cpp` - Login & authentication
- [ ] `src/NodeComm.cpp` - Message handling & routing

### Node Features
- [ ] MAC address reading & storage
- [ ] OLED initialization & display frames
- [ ] WiFi AP mode activation
- [ ] WebServer request handling
- [ ] LoRa message transmission
- [ ] Message reception & parsing
- [ ] Relay logic
- [ ] Keep-alive heartbeat
- [ ] User session caching
- [ ] Page rendering on display

### Testing Node
- [ ] Unit tests voor LoRa comm
- [ ] Integration tests voor mesh
- [ ] Display output verification
- [ ] WebServer accessibility
- [ ] Battery calculation accuracy

## Phase 3: RPI Backend (TBD)

### Backend Implementation
- [ ] Express.js server setup ✓ (skeleton)
- [ ] Database connection pool ✓ (skeleton)
- [ ] API endpoints implementation
  - [ ] GET /api/nodes
  - [ ] POST /api/nodes/:id/update
  - [ ] User management endpoints
  - [ ] Group management endpoints
  - [ ] Page management endpoints
  - [ ] Topology endpoints

### USB Bridge Communication
- [ ] Serial port initialization
- [ ] Message serialization/deserialization
- [ ] Bridge polling mechanism
- [ ] Error handling & reconnection

### Database Operations
- [ ] Connection pooling ✓ (skeleton)
- [ ] CRUD operations for all tables
- [ ] Transaction support
- [ ] Index optimization
- [ ] Backup strategy

## Phase 4: RPI Webserver (TBD)

### Dashboard Pages
- [ ] Main dashboard layout ✓ (skeleton)
- [ ] Node management page
- [ ] User management page
- [ ] Group management page
- [ ] Page editor interface
- [ ] Network monitoring page
- [ ] Settings & configuration

### Frontend Features
- [ ] Real-time updates (Socket.io) ✓ (skeleton)
- [ ] Network topology visualization
- [ ] Page editor with HTML preview
- [ ] User/group crud forms
- [ ] Authentication UI
- [ ] Mobile responsive design

### Asset Management
- [ ] Image upload handling
- [ ] Asset directory structure
- [ ] Cache management
- [ ] File size limits

## Phase 5: Integration (TBD)

### Node ↔ RPI Communication
- [ ] DISCOVER message flow
- [ ] STATUS message flow
- [ ] CONFIG message handling
- [ ] AUTH token synchronization
- [ ] PAGE content delivery

### System Tests
- [ ] Single node lifecycle
- [ ] Multi-node mesh discovery
- [ ] Message relay verification
- [ ] Node ↔ RPI communication
- [ ] Database synchronization
- [ ] WebServer functionality

## Phase 6: Deployment (TBD)

### RPI Setup
- [ ] Docker installation
- [ ] Docker Compose setup
- [ ] Network configuration
- [ ] Certificate generation
- [ ] Environment variables

### Node Deployment
- [ ] Flashing first node
- [ ] First network join
- [ ] RPI discovery
- [ ] User registration
- [ ] Page assignment

### Documentation
- [ ] Setup guides
- [ ] Troubleshooting docs
- [ ] API documentation
- [ ] User manual
- [ ] Architecture diagrams

## Controlechecklist per Fase

### ✅ Phase 1: Project Structure (COMPLETED)

**Verificatie:**
1. Alle mappen aanwezig?
   ```bash
   ls -la MeshNet/node/src/ MeshNet/rpi/docker/ MeshNet/protocol/
   ```

2. Alle bestanden aanwezig?
   ```bash
   ls -la MeshNet/node/*.ino
   ls -la MeshNet/rpi/docker/docker-compose.yml
   ls -la MeshNet/protocol/PROTOCOL_SPEC.md
   ```

3. Documentatie compleet?
   ```bash
   grep -l "message types\|node id\|database schema" MeshNet/protocol/*.md MeshNet/rpi/docker/*.sql
   ```

**✓ Status**: Phase 1 COMPLETE - Project foundation established

---

### Phase 2: Node Software (READY TO START)

**Acceptance Criteria:**
- [ ] Node compileert zonder errors
- [ ] MAC address is uniek per node
- [ ] Display toont minstens: node name/ID, connected node count
- [ ] WiFi AP is bereikbaar (SSID: "MeshNode-AABBCCDDEE")
- [ ] All files compile successfully

**Entry Checkpoint:**
```bash
cd node/
# Copy Config.h en Protocol.h naar Arduino IDE
# Open mesh_node.ino in Arduino IDE
# Select Heltec V3 board
# Compile (Verify) - should show 0 errors
```

---

### Phase 3: RPI Backend (READY TO START)

**Acceptance Criteria:**
- [ ] Backend starts without errors: `docker-compose up`
- [ ] API responds on port 3001
- [ ] MySQL database populated
- [ ] All endpoints accessible

**Entry Checkpoint:**
```bash
cd rpi/docker/
docker-compose up -d
sleep 10
curl http://localhost:3001/health
```

---

### Phase 4: RPI Webserver (READY TO START)

**Acceptance Criteria:**
- [ ] Dashboard loads on port 80
- [ ] Real-time updates work (Socket.io)
- [ ] All pages accessible
- [ ] API proxy working

**Entry Checkpoint:**
```bash
docker-compose logs webserver
curl http://localhost/health
```

---

## Next Steps

**Immediate Actions:**
1. ✅ Mappenstructuur opgesteld
2. ✅ Core headers en skeleton bestanden aangemaakt
3. ✅ Protocol definitie vastgesteld
4. ✅ Docker infrastructure beschreven
5. **→ Start Phase 2**: Node software modules implementeren

**For Phase 2 Development:**
- [ ] Focus eerst op `src/NodeStorage.cpp` - SPIFFS operaties
- [ ] Dan `src/NodeDisplay.cpp` - Display output verificatie
- [ ] Dan `src/NodeWebServer.cpp` - HTTP requests afhandelen
- [ ] Daarna `src/LoRaMesh.cpp` - Mesh communicatie
- [ ] Test elke module incrementeel

**Build Strategy:**
1. Compileerbaar → Basic functionality → Integration testing
2. Elke module heeft unit tests
3. Hardware testing op echte Heltec V3

---

**Project Status**: 🟢 Phase 1 Complete, Ready for Phase 2  
**Last Updated**: 28-01-2026  
**Current Focus**: Foundation establishment complete
