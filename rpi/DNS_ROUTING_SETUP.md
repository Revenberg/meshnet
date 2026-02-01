# MeshNet DNS Routing Setup

## Overview
DNS routing allows accessing MeshNet services via domain names instead of IP addresses. Uses Caddy as a reverse proxy with automatic service discovery.

## What's Now Available

### Main Dashboard
- **Local Network**: `http://meshnet.local`
- **IP Access**: `http://192.168.x.x/` (original)
- **Mobile Apps**: Auto-redirects to webserver

### Backend API
- **DNS**: `http://api.meshnet.local/api/...`
- **Direct**: `http://localhost:3001/api/...`

### LoRa Gateway
- **DNS**: `http://gateway.meshnet.local`
- **Direct**: `http://localhost:3002`

### MQTT Broker
- **Address**: `mqtt.meshnet.local:1883`
- **Direct**: `localhost:1883`

## Architecture

```
┌─────────────────────────────────────┐
│   DNS Requests (*.meshnet.local)    │
└─────────────┬───────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│  Caddy Reverse Proxy (Port 80/443)  │
│  - Route /api/* → backend:3001      │
│  - Route /socket.io → webserver     │
│  - Default → webserver:80           │
└─────────────┬───────────────────────┘
              │
      ┌───────┴────────┬──────────────┬────────────────┐
      ▼                ▼              ▼                ▼
┌──────────┐  ┌──────────────┐  ┌──────────┐  ┌──────────────┐
│ Backend  │  │  Webserver   │  │ LoRa GW  │  │   MySQL      │
│ API 3001 │  │   (8080)     │  │  (3002)  │  │   (3307)     │
└──────────┘  └──────────────┘  └──────────┘  └──────────────┘
```

## Setup Instructions

### 1. Update /etc/hosts for Local DNS (Raspberry Pi)

Add to `/etc/hosts`:
```bash
127.0.0.1 meshnet.local
127.0.0.1 api.meshnet.local
127.0.0.1 gateway.meshnet.local
127.0.0.1 mqtt.meshnet.local
```

Or run:
```bash
echo "127.0.0.1 meshnet.local" | sudo tee -a /etc/hosts
echo "127.0.0.1 api.meshnet.local" | sudo tee -a /etc/hosts
echo "127.0.0.1 gateway.meshnet.local" | sudo tee -a /etc/hosts
echo "127.0.0.1 mqtt.meshnet.local" | sudo tee -a /etc/hosts
```

### 2. Optional: Setup mDNS for Network-Wide Access

Install avahi (mDNS support):
```bash
sudo apt-get install avahi-daemon avahi-utils
```

This enables `meshnet.local` access from any device on the network.

### 3. Start DNS Routing

Rebuild and restart Docker services:
```bash
cd /path/to/MeshNet/rpi
docker-compose down
docker-compose up -d caddy webserver backend
```

Verify Caddy is running:
```bash
docker-compose logs caddy | tail -20
```

### 4. Test DNS Routing

From the Raspberry Pi or any device on the network:

```bash
# Test main dashboard
curl http://meshnet.local/health

# Test API
curl http://api.meshnet.local/api/nodes

# Test gateway
curl http://gateway.meshnet.local/status

# Test with browser
open http://meshnet.local
```

## Client Setup

### Web Browsers (Desktop/Mobile)

1. Ensure device is on same network as RPI
2. Navigate to: `http://meshnet.local`
3. Dashboard loads with responsive design

### Node.js/API Clients

```javascript
// Instead of hardcoding IP
const API_URL = 'http://api.meshnet.local';

// Or use environment variable
const API_URL = process.env.API_URL || 'http://api.meshnet.local';
```

### Arduino/ESP32 Clients

For IoT devices, you can use mDNS library:
```cpp
#include <ESPmDNS.h>

// In setup()
if (!MDNS.begin("meshnet")) {
    Serial.println("Error setting up MDNS responder!");
}

// In your code
const char* hostname = "api.meshnet.local";  // Resolves automatically
```

## Port Mapping

| Service | Docker Port | Host Port | DNS Name |
|---------|------------|-----------|----------|
| Webserver | 80 | 8080 | meshnet.local |
| Backend API | 3001 | (via Caddy) | api.meshnet.local |
| LoRa Gateway | 3002 | (via Caddy) | gateway.meshnet.local |
| MQTT Broker | 1883 | 1883 | mqtt.meshnet.local |
| MySQL | 3306 | 3307 | (internal only) |
| Caddy | 80/443 | 80/443 | (reverse proxy) |

## Troubleshooting

### "meshnet.local Not Found"

1. Check /etc/hosts contains entry
2. Verify Caddy is running: `docker-compose ps caddy`
3. Restart Caddy: `docker-compose restart caddy`
4. Try IP address instead: `http://192.168.x.x:8080/`

### Caddy Not Starting

Check logs:
```bash
docker-compose logs caddy --tail=50
```

Common issues:
- Port 80 already in use: `lsof -i :80`
- Caddyfile syntax error: validate with `caddy validate Caddyfile`

### API Endpoint Returns 404

Verify routing in Caddy logs:
```bash
docker-compose logs caddy | grep api.meshnet.local
```

### WebSocket Connection Failed

Ensure `/socket.io/*` route is in Caddyfile:
```
route /socket.io/* {
  reverse_proxy webserver:80
}
```

## Advanced Configuration

### Custom Domain (Instead of .local)

Edit `docker-compose.yml` and `Caddyfile`:

```
# In Caddyfile
yourdomain.com,
api.yourdomain.com {
  reverse_proxy backend:3001
}
```

Then point DNS record to RPI IP:
```
yourdomain.com → 192.168.x.x
```

### SSL/TLS Certificates

Caddy can auto-generate Let's Encrypt certificates:

```
# In Caddyfile
yourdomain.com {
  reverse_proxy backend:3001
}
```

Caddy handles HTTPS automatically!

### Rate Limiting

Add to Caddyfile:
```
meshnet.local {
  rate_limit {
    zone name {
      key {remote_host}
      events 100
      window 1m
    }
  }
  reverse_proxy webserver:80
}
```

## Related Documentation

- [Caddy Server Docs](https://caddyserver.com/docs/)
- [Docker Compose Networking](https://docs.docker.com/compose/networking/)
- [mDNS/Avahi Setup](https://wiki.archlinux.org/title/Avahi)

---

**Status**: ✅ DNS Routing Implemented  
**Version**: V0.8.1  
**Date**: 2026-01-29
