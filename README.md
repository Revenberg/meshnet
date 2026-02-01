# MeshNet - Distributed LoRa Mesh Network System

Een volledig gedistribueerd mesh netwerk systeem voor Heltec WiFi LoRa 32 V3 nodes met centraal beheer via Raspberry Pi.

## 🎯 Features

### Node Features
- **Mesh Networking**: Nodes relaying berichten naar elkaar
- **Unique Node IDs**: Gebaseerd op MAC adres, optioneel met functionele naam
- **OLED Display**: Status informatie en verbinding aanduiding
- **WiFi AP Mode**: Elke node is een WiFi access point
- **Persistent Login**: Gebruiker login cache op device
- **Groep-based Content**: Webpagina's per gebruikersgroep
- **Real-time Status**: Aantal verbonden nodes

### RPI Controller Features
- **Docker Stack**: Volledig containerized
- **Multi-network**: Ethernet primary, dual WiFi fallback
- **Node Management**: Naamgeving en configuratie
- **User/Group Management**: Gebruikers- en groepsbeheer
- **Page Editor**: HTML pagina's per node/groep
- **Dashboard**: Real-time monitoring en status
- **Database**: MySQL voor persistente opslag
- **Bridge**: USB verbinding met LoRa gateway

## 📁 Project Structure

```
MeshNet/
├── node/                    # ESP32/Arduino code
│   ├── mesh_node.ino       # Main sketch
│   ├── include/            # Header files
│   │   ├── Config.h        # Configuration
│   │   └── Protocol.h      # Message structures
│   ├── src/                # Source files (TBD)
│   └── boards/             # Board definitions
│
├── rpi/                     # Raspberry Pi setup
│   ├── docker/             # Docker configuration
│   │   ├── docker-compose.yml
│   │   ├── Dockerfile.backend
│   │   ├── Dockerfile.webserver
│   │   └── mysql.sql       # Database schema
│   ├── backend/            # Node.js API server
│   ├── webserver/          # Dashboard & UI
│   └── network/            # Network configuration
│
├── protocol/               # Communication specs
│   └── PROTOCOL_SPEC.md   # Message format & routing
│
├── documentation/          # Project documentation
│   ├── ARCHITECTURE.md
│   ├── NODE_SETUP.md
│   └── RPI_SETUP.md
│
├── setup.sh               # RPI setup script
└── README.md              # This file
```

## 🚀 Quick Start

### RPI Setup (Raspberry Pi 4)

**One-liner (GhostNet, Docker Compose aanwezig)**
```bash
git clone https://github.com/Revenberg/meshnet.git; cd meshnet/rpi/docker; ./detect_serial_and_start.sh
```

1. **Prerequisites**
   ```bash
   sudo apt-get update
   sudo apt-get install -y docker.io docker-compose git
   ```

2. **Clone & Setup**
   ```bash
   git clone <repo-url> MeshNet
   cd MeshNet
   chmod +x setup.sh
   sudo ./setup.sh
   ```

3. **Access Dashboard**
   - Open browser: `http://<rpi-ip>`
   - Backend API: `http://<rpi-ip>:3001`

## 🔐 SSH key setup (GhostNet)

### Op de laptop
```powershell
cd $env:USERPROFILE\.ssh
ssh-keygen
# alles Enter
# copieer inhoud van .pub file naar klipbord
```

### Op de RPI host
```bash
cd ~/meshnet
git config pull.ff only

sudo adduser copilot
sudo usermod -aG sudo copilot
sudo usermod -aG docker copilot

mkdir -p ~/.ssh
touch ~/.ssh/authorized_keys
# voeg klipboard toe aan ~/.ssh/authorized_keys
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
```

### Werkend statement
```bash
ssh copilot@GhostNet "cd ~/meshnet;git pull;cd ~/meshnet/rpi/docker;chmod +x *.sh;./detect_serial_and_start.sh"
```

## ✅ TODO
- Hoe zet ik een gecompileerde `.ino` (of `.bin`) naar de RPI USB‑poort?

### Node Setup (Heltec V3)

1. **Install Arduino IDE** - https://www.arduino.cc/en/software

2. **Add Heltec Board Support**
   - File → Preferences → Additional Boards Manager URLs
   - Add: `https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/releases/download/0.0.7/package_heltec_esp32_index.json`
   - Tools → Board Manager → Search "Heltec ESP32" → Install

3. **Install Required Libraries**
   - Tools → Manage Libraries:
     - RadioLib
     - SSD1306
     - ArduinoJSON
     - AsyncTCP
     - ESPAsyncWebServer

4. **Flash Node Code**
   - Open `node/mesh_node.ino` in Arduino IDE
   - Select Board: "Heltec WiFi LoRa 32 V3"
   - Select Port: COM port of your Heltec
   - Click Upload

## 📡 Communication Protocol

### Message Types

| Type | Direction | Purpose |
|------|-----------|---------|
| DISCOVER | Node → RPI | Node aanmelding |
| STATUS | Node → RPI | Periodieke status |
| RELAY | Node → Node | Doorsturen berichten |
| CONFIG | RPI → Node | Configuratie update |
| AUTH | RPI → Node | Gebruiker authenticatie |
| PAGE | RPI → Node | Pagina content |
| KEEPALIVE | Bidirectioneel | Connection heartbeat |

Zie [PROTOCOL_SPEC.md](protocol/PROTOCOL_SPEC.md) voor details.

## 🔧 API Endpoints

### Nodes
- `GET /api/nodes` - Alle nodes
- `GET /api/nodes/:nodeId` - Specifieke node
- `POST /api/nodes/:nodeId/update` - Update node info

### Users
- `GET /api/users` - Alle gebruikers
- `POST /api/users` - Gebruiker aanmaken

### Groups
- `GET /api/groups` - Alle groepen
- `POST /api/groups` - Groep aanmaken

### Pages
- `GET /api/pages/:nodeId` - Pagina's voor node
- `POST /api/pages` - Pagina aanmaken

### Topology
- `GET /api/topology` - Node verbindingen

## 🔐 Security

- JWT tokens voor authenticatie
- Password hashing met bcryptjs
- HTTPS ready (configure in production)
- Login session caching op nodes
- Database credentials in environment variables

## 📊 Database Schema

### Main Tables
- `users` - Gebruiker accounts
- `groups` - Gebruikersgroepen
- `nodes` - LoRa nodes
- `pages` - Webpagina content
- `user_sessions` - Actieve sessions
- `node_connections` - Network topology
- `message_log` - Berichten historiek

## 🛠️ Configuration Files

### Node Configuration (`include/Config.h`)
- LoRa parameters (frequentie, spreading factor)
- Display settings
- WiFi credentials
- Mesh parameters
- Timing intervals

### RPI Configuration (`.env`)
```
DB_HOST=mysql
DB_USER=meshnet
DB_PASSWORD=change_me!
JWT_SECRET=change_me!
NODE_ENV=production
```

## 📈 Network Architecture

```
   ┌─────────────────────────┐
   │   RPI Controller        │
   │  (Docker Stack)         │
   └────────────┬────────────┘
                │ USB
       ┌────────▼────────┐
       │  Bridge Node    │ (Heltec V3)
       │ (USB LoRa GW)   │
       └────────┬────────┘
                │ LoRa
    ┌───────┬───┴───┬────────┐
    ▼       ▼       ▼        ▼
  Node1   Node2   Node3    NodeN
(Heltec) (Heltec)(Heltec) (Heltec)
```

## 🔄 Mesh Routing

- **Direct delivery** voor nodes in range
- **Broadcast relay** via nabijgelegen nodes
- **TTL-based** max 10 hops
- **RSSI-prioritized** relay selection
- **Automatic redundancy**

## ⚙️ Development

### Building Locally
```bash
cd node
# Compile with Arduino CLI or IDE

cd ../rpi/backend
npm install
npm start  # Starts on :3001

cd ../webserver
npm install
npm start  # Starts on :80
```

### Docker Commands
```bash
cd rpi/docker

# Start all services
docker-compose up -d

# View logs
docker-compose logs -f

# Stop services
docker-compose down

# Rebuild images
docker-compose build --no-cache
```

## 📝 Documentation

- [ARCHITECTURE.md](documentation/ARCHITECTURE.md) - System design
- [NODE_SETUP.md](documentation/NODE_SETUP.md) - Node installation guide
- [RPI_SETUP.md](documentation/RPI_SETUP.md) - RPI setup details
- [PROTOCOL_SPEC.md](protocol/PROTOCOL_SPEC.md) - Communication protocol

## 🐛 Troubleshooting

### Node niet zichtbaar op RPI
1. Check USB verbinding van bridge node
2. Verify LoRa frequency match
3. Check node battery level
4. View backend logs: `docker-compose logs backend`

### WiFi AP niet bereikbaar
1. Check node power supply
2. Verify WiFi AP settings in Config.h
3. Check OLED display for errors

### Database connection error
1. Check MySQL is running: `docker ps`
2. Verify credentials in environment
3. Check port 3306 availability

## 📞 Support

Vragen of problemen? Open een issue op GitHub.

## 📄 License

MIT License - zie LICENSE bestand

## 🙏 Acknowledgments

- RadioLib voor LoRa communicatie
- Heltec voor board support
- Express.js community
- Arduino project

---

**Status**: Alpha (v1.0.0-alpha)  
**Last Updated**: 28-01-2026  
**Maintainer**: MeshNet Project Team
