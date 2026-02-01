# MeshNet Communication Protocol Specification

## Protocol Overview
Het MeshNet protocol is een lichtgewicht JSON-gebaseerd messaging systeem voor communicatie tussen nodes en de RPI controller.

## Message Format

### Base Message Structure
```json
{
  "type": "MESSAGE_TYPE",
  "from": "NODE_ID",
  "to": "NODE_ID|BROADCAST",
  "msgId": "unique_message_id",
  "timestamp": 1234567890,
  "ttl": 10,
  "hops": 0,
  "data": {}
}
```

## Message Types

### 1. DISCOVER (Node aanmelding)
**Richting**: Node → Mesh → RPI  
**Frequentie**: Bij startup + periodiek (1x per 5 minuten)

```json
{
  "type": "DISCOVER",
  "from": "AA:BB:CC:DD:EE:FF",
  "to": "BROADCAST",
  "msgId": "discovery_001",
  "timestamp": 1234567890,
  "data": {
    "nodeId": "AA:BB:CC:DD:EE:FF",
    "functionalName": "kitchen_sensor",
    "version": "1.0.0",
    "signalStrength": -65,
    "battery": 85,
    "uptime": 3600
  }
}
```

### 2. STATUS (Periodieke status rapportage)
**Richting**: Node → Mesh → RPI  
**Frequentie**: 1x per 10 seconden

```json
{
  "type": "STATUS",
  "from": "AA:BB:CC:DD:EE:FF",
  "to": "BRIDGE_NODE",
  "msgId": "status_002",
  "timestamp": 1234567890,
  "data": {
    "nodeId": "AA:BB:CC:DD:EE:FF",
    "rssi": -65,
    "battery": 84,
    "connectedNodes": 3,
    "lastPageUpdate": 1234567800,
    "activeUsers": 1
  }
}
```

### 3. RELAY (Doorsturen berichten)
**Richting**: Node → Node → ... → Bridge/RPI  
**Frequentie**: On-demand

```json
{
  "type": "RELAY",
  "from": "NODE_A",
  "to": "RPI_BRIDGE",
  "msgId": "relay_003",
  "timestamp": 1234567890,
  "ttl": 8,
  "hops": 1,
  "data": {
    "originalMessage": { /* origineel bericht */ }
  }
}
```

### 4. CONFIG (Configuratie update)
**Richting**: RPI → Bridge → Mesh → Node  
**Frequentie**: On-demand (bij wijzigingen)

```json
{
  "type": "CONFIG",
  "from": "RPI_BRIDGE",
  "to": "AA:BB:CC:DD:EE:FF|BROADCAST",
  "msgId": "config_004",
  "timestamp": 1234567890,
  "data": {
    "nodeId": "AA:BB:CC:DD:EE:FF",
    "functionalName": "kitchen_sensor",
    "wifiSSID": "MeshNet",
    "wifiPassword": "secure_password",
    "meshEnabled": true,
    "discoveryInterval": 300
  }
}
```

### 5. AUTH (Authenticatie synchronisatie)
**Richting**: RPI → Bridge → Node  
**Frequentie**: Bij inlog + refresh

```json
{
  "type": "AUTH",
  "from": "RPI_BRIDGE",
  "to": "AA:BB:CC:DD:EE:FF",
  "msgId": "auth_005",
  "timestamp": 1234567890,
  "data": {
    "action": "LOGIN|UPDATE|LOGOUT",
    "userId": "user_123",
    "groupId": "group_admin",
    "groupName": "Administrators",
    "token": "jwt_token_xxx",
    "expiresAt": 1234568490,
    "permissions": ["view_dashboard", "edit_config"]
  }
}
```

### 6. PAGE (Pagina content update)
**Richting**: RPI → Bridge → Node  
**Frequentie**: Bij wijziging inhoud

```json
{
  "type": "PAGE",
  "from": "RPI_BRIDGE",
  "to": "AA:BB:CC:DD:EE:FF|BROADCAST",
  "msgId": "page_006",
  "timestamp": 1234567890,
  "data": {
    "nodeId": "AA:BB:CC:DD:EE:FF",
    "pageId": "page_kitchen_001",
    "groupId": "group_users",
    "title": "Keuken Status",
    "content": "<h1>Welkom</h1>...",
    "imageUrl": "/api/images/kitchen.jpg",
    "refreshInterval": 30
  }
}
```

### 7. KEEPALIVE (Connection heartbeat)
**Richting**: Bidirectioneel  
**Frequentie**: 1x per 5 seconden

```json
{
  "type": "KEEPALIVE",
  "from": "AA:BB:CC:DD:EE:FF",
  "to": "BRIDGE_NODE",
  "msgId": "keepalive_007",
  "timestamp": 1234567890,
  "data": {
    "nodeId": "AA:BB:CC:DD:EE:FF",
    "hopCount": 1
  }
}
```

## Node ID System

### Format
```
MAC_ADDRESS:FUNCTIONAL_NAME

Voorbeeld: AA:BB:CC:DD:EE:FF:kitchen_sensor
```

### Regels
- **MAC adres**: Verplicht, onveranderlijk (6 bytes HEX)
- **Functionele naam**: Optioneel, max 32 karakters (a-z, 0-9, underscore)
- **Separator**: Dubbele punt (`:`)

### Storage
- **Locatie**: SPIFFS `/mesh/node.json`
- **Persistentie**: Blijft behouden na restart
- **Update**: Via CONFIG message of webinterface

## Mesh Routing

### Algoritme
1. **Direct delivery**: Rechtstreeks naar target (als binnen range)
2. **Broadcast relay**: Via nabijgelegen nodes
3. **TTL-based**: Max 10 hops, daalt per relay
4. **Duplicate detection**: msgId tracking

### Relay Selection
- Prioriteit: Signaalsterkte (RSSI)
- Max 3 simultane relays per node
- Timeout: 5 seconden per hop

## Authentication Flow

```
1. User Login op Node WebServer
   └─> Node cacht token in SPIFFS
   └─> Node stuurt AUTH message naar RPI
   
2. RPI valideert credentials
   └─> RPI genereert JWT token
   └─> RPI stuurt AUTH_CONFIRM terug
   
3. Node ontvangt bevestiging
   └─> Node toont login success op display
   └─> Node serves authorized content
```

## Error Handling

### Message Validation
- Verplichtvelden: `type`, `from`, `to`, `timestamp`
- JSON size limit: 1024 bytes
- Malformed messages: Silent drop

### Timeout Handling
- Relay timeout: 5 sec
- Keep-alive timeout: 15 sec → reconnect
- Config update retry: 3x met 2 sec interval

## Performance Targets

| Metric | Target |
|--------|--------|
| Message latency | < 200ms (direct) |
| Relay latency | < 500ms (3 hops) |
| Discovery time | < 5 sec |
| Keep-alive overhead | < 1KB/min |
| Auth sync time | < 2 sec |

## Versioning

- **Current**: 1.0.0
- **Compatibility**: Backward compatible via version field
- **Updates**: Bij protocol wijzigingen versie verhogen

---
*Specificatie v1.0.0*
*Laatst gewijzigd: 28-01-2026*
