# MeshNet Project Plan

## Projectoverzicht
Een volledig gedistribueerd mesh netwerk systeem met:
- **Heltec WiFi LoRa 32 V3 nodes** - zelf-organiserend mesh netwerk
- **Raspberry Pi 4 controller** - centraal beheer en monitoring
- **Webinterface op nodes** - per groep andere inhoud
- **RPI management interface** - gebruiker/groep/node beheer

## Architectuur

```
┌─────────────────────────────────────────────────────────┐
│                    RPI Controller                       │
│  ┌──────────────────────────────────────────────────┐  │
│  │ Docker Stack                                     │  │
│  │ - Backend API (Node.js/Python)                  │  │
│  │ - WebServer (Node.js)                           │  │
│  │ - MySQL Database                                │  │
│  │ - Mosquitto MQTT (optioneel)                    │  │
│  └──────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────┐  │
│  │ Network: Ethernet + 2x WiFi (failover)          │  │
│  └──────────────────────────────────────────────────┘  │
└──────────────────┬────────────────────────────────────┘
                   │ USB Bridge
                   ▼
┌─────────────────────────────────────────────────────────┐
│                   Bridge Node                            │
│  (Heltec V3 - USB connected)                            │
│  - LoRa communicatie naar andere nodes                  │
│  - Relaying van RPI berichten                           │
└──────────────────┬────────────────────────────────────┘
                   │ LoRa (mesh)
      ┌────────────┼────────────┬──────────────┐
      ▼            ▼            ▼              ▼
   ┌─────┐    ┌─────┐    ┌─────┐        ┌─────┐
   │Node1│◄──►│Node2│◄──►│Node3│  ...  │NodeN│
   └─────┘    └─────┘    └─────┘        └─────┘
```

## Component Details

### Node (Heltec V3)
- **Communicatie**: LoRa mesh (via RadioLib)
- **Storage**: SPIFFS voor config
- **Display**: OLED (SSD1306) - status & naam
- **WebServer**: WiFi AP mode + Station mode
- **Auth**: Per-device login cache (persistent)

### RPI
- **Bridge**: USB LoRa naar MQTT
- **API**: REST backend voor node beheer
- **WebUI**: Management dashboard
- **Database**: Gebruikers, groepen, nodes, pagina's
- **Network**: Ethernet primary, WiFi fallback

## Fases

### Fase 1: Core Node (Week 1-2)
- [x] Mappenstructuur
- [ ] Node ID & MAC adres systeem
- [ ] Communicatie protocol definities
- [ ] Display framework
- [ ] WiFi AP + login systeem
- [ ] SPIFFS storage

### Fase 2: Node Mesh (Week 2-3)
- [ ] LoRa mesh routing
- [ ] Node discovery
- [ ] Message relaying
- [ ] Keep-alive mechanisme

### Fase 3: Node WebServer (Week 3-4)
- [ ] Webserver framework
- [ ] Login persistentie
- [ ] Groep-gebaseerde content
- [ ] Page rendering

### Fase 4: RPI Infrastructure (Week 4-5)
- [ ] Docker Compose setup
- [ ] MySQL schema
- [ ] Backend API (CRUD)
- [ ] USB bridge communicatie

### Fase 5: RPI Management (Week 5-6)
- [ ] WebUI dashboard
- [ ] Gebruiker/groep management
- [ ] Node naamgeving
- [ ] Page editor
- [ ] Statistics & monitoring

### Fase 6: Integration & Testing (Week 6-7)
- [ ] End-to-end testing
- [ ] Performance tuning
- [ ] Documentation
- [ ] Deployment scripts

## File Structure

```
MeshNet/
├── node/                          # Heltec V3 Arduino code
│   ├── mesh_node.ino             # Main sketch
│   ├── src/
│   │   ├── LoRaMesh.cpp          # Mesh networking
│   │   ├── NodeDisplay.cpp       # OLED display
│   │   ├── NodeWebServer.cpp     # AP webserver
│   │   ├── NodeStorage.cpp       # SPIFFS
│   │   ├── NodeAuth.cpp          # Login management
│   │   └── NodeComm.cpp          # Message handling
│   ├── include/
│   │   ├── LoRaMesh.h
│   │   ├── NodeDisplay.h
│   │   ├── NodeWebServer.h
│   │   ├── NodeStorage.h
│   │   ├── NodeAuth.h
│   │   ├── NodeComm.h
│   │   ├── Config.h              # Node configuration
│   │   └── Protocol.h            # Message structures
│   ├── boards/
│   │   └── boards-ht_v3.txt      # Board definitions
│   └── library.properties        # Dependencies
│
├── rpi/                           # Raspberry Pi setup
│   ├── docker/
│   │   ├── docker-compose.yml
│   │   ├── Dockerfile.backend
│   │   ├── Dockerfile.webserver
│   │   └── mysql.sql            # Database schema
│   ├── backend/
│   │   ├── src/
│   │   │   ├── server.js         # API endpoints
│   │   │   ├── nodeManager.js    # Node operations
│   │   │   ├── userManager.js    # User/group ops
│   │   │   ├── bridgeComm.js     # USB communication
│   │   │   └── database.js       # DB operations
│   │   ├── package.json
│   │   └── routes/
│   │       ├── api/
│   │       │   ├── nodes.js
│   │       │   ├── users.js
│   │       │   ├── groups.js
│   │       │   └── pages.js
│   ├── webserver/
│   │   ├── server.js
│   │   ├── package.json
│   │   ├── public/
│   │   │   ├── dashboard.html    # Main dashboard
│   │   │   ├── pages/
│   │   │   │   ├── nodes.html
│   │   │   │   ├── users.html
│   │   │   │   ├── groups.html
│   │   │   │   ├── pages-editor.html
│   │   │   │   └── monitoring.html
│   │   │   ├── assets/
│   │   │   │   ├── css/
│   │   │   │   ├── js/
│   │   │   │   └── images/
│   │   └── routes/
│   │       └── index.js
│   └── network/
│       ├── setup-ethernet.sh
│       ├── setup-wifi.sh
│       └── network-manager.sh
│
├── protocol/                      # Communication protocol
│   ├── message-format.md         # Message spec
│   ├── mesh-routing.md           # Routing algorithm
│   ├── api-spec.md               # API documentation
│   └── auth-flow.md              # Authentication flow
│
├── documentation/
│   ├── ARCHITECTURE.md
│   ├── NODE_SETUP.md             # Node flashing guide
│   ├── RPI_SETUP.md              # RPI setup
│   ├── API_REFERENCE.md
│   ├── PROTOCOL_REFERENCE.md
│   └── TROUBLESHOOTING.md
│
├── setup.sh                       # Project setup script
├── README.md
└── LICENSE
```

## Communicatie Protocol (Summary)

### Message Types
1. **DISCOVER** - Node aanmelden in mesh
2. **STATUS** - Node status rapportage
3. **RELAY** - Doorsturen naar andere nodes
4. **CONFIG** - Configuratie update
5. **AUTH** - Gebruiker authenticatie sync
6. **PAGE** - Pagina inhoud update

### Node ID
- **Format**: `MAC_HEX:FUNCTIONAL_NAME`
- **Example**: `AA:BB:CC:DD:EE:FF:kitchen_sensor`
- **Storage**: SPIFFS persistent storage

### Message Structure
```json
{
  "type": "DISCOVER|STATUS|RELAY|CONFIG|AUTH|PAGE",
  "from": "NODE_ID",
  "to": "NODE_ID|BROADCAST",
  "timestamp": 1234567890,
  "data": {},
  "hops": 0
}
```

## Controllepunten per Fase

**Fase 1 Checkpoint**: 
- [ ] Node code compileert
- [ ] MAC adres wordt correct gelezen
- [ ] SPIFFS opslag werkt
- [ ] Display toont informatie
- [ ] WiFi AP mode actief

**Fase 2 Checkpoint**:
- [ ] Nodes ontdekken elkaar
- [ ] Messages worden gerelayed
- [ ] Keep-alive werkt
- [ ] Display toont aantal connecties

**Fase 3 Checkpoint**:
- [ ] WebServer is bereikbaar
- [ ] Login wordt persistent opgeslagen
- [ ] Pagina's renderen per groep

**Fase 4 Checkpoint**:
- [ ] RPI containers runnen
- [ ] MySQL database populated
- [ ] API endpoints actief

**Fase 5 Checkpoint**:
- [ ] Dashboard bereikbaar via RPI web
- [ ] Nodes verschijnen in dashboard
- [ ] Gebruiker/groep beheer werkt
- [ ] Pagina's kunnen gemaakt en geupdate worden

## Dependencies

### Node (Arduino)
- RadioLib 6.x (LoRa)
- SSD1306 (display)
- ArduinoJSON (messaging)
- HTTPClient (API calls)

### RPI Backend
- Node.js 18+
- Express.js
- MySQL2
- node-serialport (USB)

### RPI WebServer
- Node.js 18+
- Express.js
- Socket.io (real-time updates)
- Bootstrap 5 (UI)

## Tijdschema
- **Totaal**: ~6-7 weken
- **Iteratief**: Elke fase is compleet voor volgende begint
- **Testing**: Concurrent met development

---
*Bijgewerkt: 28-01-2026*
