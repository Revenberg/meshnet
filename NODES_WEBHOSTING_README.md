# MeshNet Nodes Web Hosting System

## Overview
Het MeshNet systeem ondersteunt nu het hosten van webpagina's op de nodes. Elke node kan zijn eigen HTML-pagina's serveren via de API, waarbij elke pagina kan bevatten:
- Complete HTML content met inline styling
- Embedded SVG/Data URL images (geen externe dependencies)
- Configureerbare refresh intervals
- Real-time updates

## Node Configuration

### 3 Configured Nodes

| Node Name | Node ID | MAC Address | Status |
|-----------|---------|-------------|--------|
| MeshNode-1 | `72d67530-dac6-4666-885c-160cb36579ee` | AA:BB:CC:DD:EE:01 | Online |
| MeshNode-2 | `26b80c3a-a7e2-4634-957a-51f7b777de72` | AA:BB:CC:DD:EE:02 | Online |
| MeshNode-3 | `d1ec1f02-0e0b-4763-94d5-984e93c11bde` | AA:BB:CC:DD:EE:03 | Online |

### Groups & Users

#### Groups
- **Admin Group** (ID: `93ab0e92-2f79-45c3-a7fd-3002e0dff43d`)
  - Permissions: `["ALL"]`
  - Description: Administrator group with full permissions

- **Test Group** (ID: `744acd9f-c08e-4ff2-b37a-0cfe39668370`)
  - Permissions: `["READ", "WRITE"]`
  - Description: Test group for demo nodes

#### Admin User
- **Username**: `admin`
- **Password**: `admin123`
- **User ID**: `6f90e099-53fe-47f1-ba0c-032d07520e9f`
- **Group**: Admin
- **Status**: Active

## Web Pages

### Page 1: MeshNode-1 - "Over MeshNode-1"
- **Node**: MeshNode-1
- **Title**: Over MeshNode-1
- **Features**:
  - ✅ Embedded SVG image (node illustration)
  - Info cards with status, battery, signal, uptime
  - Feature list with LoRa capabilities
  - Purple gradient design
- **Refresh Interval**: 30 seconds
- **Status**: Published & Active

### Page 2: MeshNode-2 - "Dashboard"
- **Node**: MeshNode-2
- **Title**: Dashboard
- **Features**:
  - ✅ Embedded SVG chart (activity overview)
  - Power management metrics (battery, voltage, current)
  - Network metrics (RSSI, SNR, packets)
  - System metrics (uptime, CPU temp, RAM)
  - Pink/red gradient design
- **Refresh Interval**: 30 seconds
- **Status**: Published & Active

### Page 3: MeshNode-3 - "Status"
- **Node**: MeshNode-3
- **Title**: Status
- **Features**:
  - Real-time component status table
  - Component health monitoring
  - Event log with timestamps
  - Cyan gradient design
  - No images (content-only)
- **Refresh Interval**: 30 seconds
- **Status**: Published & Active

## API Endpoints

### Node Web Hosting API
Base URL: `http://backend:3001/api/host`

#### 1. List Pages for a Node
```
GET /api/host/node/{nodeId}/pages
```

**Response**:
```json
{
  "nodeId": "72d67530-dac6-4666-885c-160cb36579ee",
  "pages": [
    {
      "pageId": "a187e1ae-c73b-4a50-b45e-aa2bef0df899",
      "title": "Over MeshNode-1",
      "hasContent": true,
      "hasImage": false,
      "refreshInterval": 30,
      "lastUpdated": "2026-01-28T19:42:23.000Z"
    }
  ]
}
```

#### 2. Get Page Content (HTML)
```
GET /api/host/node/{nodeId}/pages/{pageId}
```

**Response**: HTML content that can be directly displayed

**Example**:
```
GET /api/host/node/72d67530-dac6-4666-885c-160cb36579ee/pages/a187e1ae-c73b-4a50-b45e-aa2bef0df899
```

#### 3. Get Page Content (JSON)
```
GET /api/host/node/{nodeId}/pages/{pageId}/json
```

**Response**:
```json
{
  "pageId": "a187e1ae-c73b-4a50-b45e-aa2bef0df899",
  "nodeId": "72d67530-dac6-4666-885c-160cb36579ee",
  "title": "Over MeshNode-1",
  "content": "<!DOCTYPE html>...",
  "imageUrl": null,
  "refreshInterval": 30
}
```

#### 4. Get Node Information
```
GET /api/host/node/{nodeId}/info
```

**Response**:
```json
{
  "nodeId": "72d67530-dac6-4666-885c-160cb36579ee",
  "functionalName": "MeshNode-1",
  "macAddress": "AA:BB:CC:DD:EE:01",
  "version": "1.0.0",
  "signalStrength": -95,
  "battery": 95,
  "connectedNodes": 2,
  "isActive": true,
  "createdAt": "2026-01-28T19:42:21.000Z",
  "updatedAt": "2026-01-28T19:42:21.000Z"
}
```

#### 5. Node Heartbeat / Status Update
```
POST /api/host/node/{nodeId}/heartbeat
```

**Request**:
```json
{
  "signalStrength": -92,
  "battery": 87,
  "connectedNodes": 3
}
```

**Response**:
```json
{
  "status": "updated",
  "timestamp": "2026-01-28T20:30:15.123Z"
}
```

## Usage Examples

### ESP32 Node - Fetch and Display Page
```cpp
// Pseudo-code for ESP32 node
void fetchAndDisplayPage() {
  String nodeId = "72d67530-dac6-4666-885c-160cb36579ee";
  String pageId = "a187e1ae-c73b-4a50-b45e-aa2bef0df899";
  
  // Get page list
  String url = "http://backend:3001/api/host/node/" + nodeId + "/pages";
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    // Parse JSON and get page ID
  }
  
  // Fetch page content
  String pageUrl = "http://backend:3001/api/host/node/" + nodeId + "/pages/" + pageId;
  http.begin(pageUrl);
  httpCode = http.GET();
  
  if (httpCode == 200) {
    String htmlContent = http.getString();
    // Display on OLED or web interface
    displayHTML(htmlContent);
  }
  
  http.end();
}
```

### Node Heartbeat - Report Status
```cpp
void reportNodeStatus() {
  String nodeId = "72d67530-dac6-4666-885c-160cb36579ee";
  String url = "http://backend:3001/api/host/node/" + nodeId + "/heartbeat";
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  String jsonPayload = "{\"signalStrength\": -92, \"battery\": 87, \"connectedNodes\": 3}";
  int httpCode = http.POST(jsonPayload);
  
  String response = http.getString();
  http.end();
}
```

## Database Schema

### Nodes Table
```sql
CREATE TABLE nodes (
  id INT PRIMARY KEY AUTO_INCREMENT,
  nodeId VARCHAR(64) UNIQUE NOT NULL,
  macAddress VARCHAR(17) UNIQUE NOT NULL,
  functionalName VARCHAR(32),
  version VARCHAR(16),
  lastSeen TIMESTAMP,
  signalStrength INT,
  battery INT,
  connectedNodes INT DEFAULT 0,
  isActive TINYINT(1) DEFAULT 1,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

### Pages Table
```sql
CREATE TABLE pages (
  id INT PRIMARY KEY AUTO_INCREMENT,
  pageId VARCHAR(64) UNIQUE NOT NULL,
  nodeId VARCHAR(64) NOT NULL,
  groupId INT,
  title VARCHAR(255) NOT NULL,
  content LONGTEXT,
  imageUrl VARCHAR(512),
  refreshInterval INT DEFAULT 30,
  isActive TINYINT(1) DEFAULT 1,
  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  FOREIGN KEY (nodeId) REFERENCES nodes(nodeId),
  FOREIGN KEY (groupId) REFERENCES `groups`(id)
);
```

## Setup Instructions

### Automatic Setup
Run the demo data setup script:
```bash
# Inside backend container
node setup-demo-data.js
```

This will:
1. ✅ Create Admin and Test groups
2. ✅ Create admin user (username: admin, password: admin123)
3. ✅ Create 3 MeshNodes with proper configuration
4. ✅ Create 3 web pages with sample content

### Manual API Testing

#### Test with cURL
```bash
# Get pages for MeshNode-1
curl http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/pages

# Get page content (HTML)
curl http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/pages/a187e1ae-c73b-4a50-b45e-aa2bef0df899

# Get page as JSON
curl http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/pages/a187e1ae-c73b-4a50-b45e-aa2bef0df899/json

# Get node info
curl http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/info

# Send heartbeat
curl -X POST http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/heartbeat \
  -H "Content-Type: application/json" \
  -d '{"signalStrength": -92, "battery": 87, "connectedNodes": 3}'
```

## Features

### ✅ Implemented
- Multiple nodes per system
- Multiple pages per node
- Group-based access control (framework ready)
- Embedded images (SVG/Data URLs)
- HTML content storage in database
- Node heartbeat/status updates
- Page refresh intervals
- Real-time node monitoring
- CORS enabled for client access
- Complete CRUD via API

### 🔄 Future Enhancements
- WebSocket support for real-time updates
- Page scheduling (publish/unpublish times)
- Template system for common page layouts
- Image upload and optimization
- Analytics dashboard
- Version history for pages
- A/B testing support
- Multi-language page versions

## Docker Compose Integration

The system runs in Docker with these services:
- **meshnet-mysql**: Database (port 3307)
- **meshnet-mqtt**: MQTT broker (port 1883)
- **meshnet-backend**: API server (port 3001)
- **meshnet-webserver**: Web frontend (port 80)
- **meshnet-phpmyadmin**: Database manager (port 8080)

All services communicate over the `meshnet` Docker network.

## Troubleshooting

### Pages not showing up
1. Check if pages are marked as `isActive = true`
2. Verify nodeId exists in nodes table
3. Check for foreign key constraint errors
4. Verify database connection

### No content returned
1. Check if `content` field is populated (not NULL)
2. Verify page is associated with correct nodeId
3. Check database for data integrity

### Node heartbeat fails
1. Verify nodeId is valid UUID format
2. Check backend API is responding to health check
3. Ensure Docker network connectivity

## Support

For issues or questions:
1. Check Docker logs: `docker logs meshnet-backend`
2. Review database: `docker exec meshnet-mysql mysql -u meshnet -p meshnet -e "SELECT * FROM pages;"`
3. Test endpoints manually with cURL
4. Check Docker network: `docker network ls`
