# MeshNet Nodes Web Hosting - Quick Reference

## 🚀 Quick Start

### Access the APIs

**3 Active Nodes:**
```
MeshNode-1: 72d67530-dac6-4666-885c-160cb36579ee
MeshNode-2: 26b80c3a-a7e2-4634-957a-51f7b777de72
MeshNode-3: d1ec1f02-0e0b-4763-94d5-984e93c11bde
```

### Test in Browser

1. **List pages for MeshNode-1:**
   ```
   http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/pages
   ```

2. **View Page 1 (Over MeshNode-1):**
   ```
   http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/pages/a187e1ae-c73b-4a50-b45e-aa2bef0df899
   ```

3. **Get Node Info:**
   ```
   http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/info
   ```

## 📁 Created Files

### Backend
- `rpi/backend/src/node-router.js` - Node hosting API routes
- `rpi/backend/setup-demo-data.js` - Setup script for demo data
- Modified: `rpi/backend/src/server.js` - Added node router import

### Documentation
- `NODES_WEBHOSTING_README.md` - Full documentation
- `test-node-api.ps1` - PowerShell test script
- `test-node-api.sh` - Bash test script

## 📊 Data Created

### Groups (2)
| Name | ID | Permissions |
|------|-----|------------|
| Admin | `93ab0e92-2f79-45c3-a7fd-3002e0dff43d` | ["ALL"] |
| Test | `744acd9f-c08e-4ff2-b37a-0cfe39668370` | ["READ", "WRITE"] |

### User (1)
| Username | Password | User ID | Group |
|----------|----------|---------|--------|
| admin | admin123 | `6f90e099-53fe-47f1-ba0c-032d07520e9f` | Admin |

### Nodes (3)
| Name | ID | MAC | Pages |
|------|-----|------|-------|
| MeshNode-1 | `72d67530-dac6-4666-885c-160cb36579ee` | AA:BB:CC:DD:EE:01 | 1 |
| MeshNode-2 | `26b80c3a-a7e2-4634-957a-51f7b777de72` | AA:BB:CC:DD:EE:02 | 1 |
| MeshNode-3 | `d1ec1f02-0e0b-4763-94d5-984e93c11bde` | AA:BB:CC:DD:EE:03 | 1 |

### Web Pages (3)
| Node | Title | Features | Page ID |
|------|-------|----------|---------|
| MeshNode-1 | Over MeshNode-1 | ✅ Embedded SVG image | `a187e1ae-c73b-4a50-b45e-aa2bef0df899` |
| MeshNode-2 | Dashboard | ✅ Embedded SVG chart | (auto-generated) |
| MeshNode-3 | Status | Event log, no image | (auto-generated) |

## 🔌 API Endpoints

### Get Pages List
```
GET /api/host/node/{nodeId}/pages
```

### Get Page (HTML)
```
GET /api/host/node/{nodeId}/pages/{pageId}
```

### Get Page (JSON)
```
GET /api/host/node/{nodeId}/pages/{pageId}/json
```

### Get Node Info
```
GET /api/host/node/{nodeId}/info
```

### Send Heartbeat
```
POST /api/host/node/{nodeId}/heartbeat
Content-Type: application/json

{
  "signalStrength": -92,
  "battery": 87,
  "connectedNodes": 3
}
```

## 🧪 Testing

### PowerShell (Windows)
```powershell
.\test-node-api.ps1
```

### Bash (Linux/Mac)
```bash
bash test-node-api.sh
```

### Manual cURL
```bash
# Get pages
curl http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/pages

# Get page content
curl http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/pages/a187e1ae-c73b-4a50-b45e-aa2bef0df899

# Send heartbeat
curl -X POST http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/heartbeat \
  -H "Content-Type: application/json" \
  -d '{"signalStrength": -92, "battery": 87, "connectedNodes": 3}'
```

## 🔄 Docker Operations

### Rebuild & Restart
```bash
cd rpi/docker
docker-compose up -d --build
```

### Check Backend Logs
```bash
docker logs meshnet-backend
```

### Re-run Setup Script
```bash
docker exec meshnet-backend node setup-demo-data.js
```

### Database Access
```bash
docker exec meshnet-mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet
```

Query pages:
```sql
SELECT pageId, nodeId, title, isActive FROM pages;
```

Query nodes:
```sql
SELECT nodeId, functionalName, macAddress FROM nodes WHERE functionalName LIKE 'MeshNode%';
```

## 🎯 Common Tasks

### Check if System is Working
1. Pages API returns data: ✅
2. HTML can be retrieved: ✅
3. Node info accessible: ✅
4. Heartbeat updates status: ✅

### Add New Page to Node
```javascript
// Pseudo-code
const newPage = {
  pageId: uuidv4(),
  nodeId: "72d67530-dac6-4666-885c-160cb36579ee",
  title: "New Page",
  content: "<html>...</html>",
  imageUrl: null,
  refreshInterval: 30,
  isActive: true
};

// INSERT into database
INSERT INTO pages (...) VALUES (...);
```

### Update Node Battery Status
```bash
curl -X POST http://localhost:3001/api/host/node/72d67530-dac6-4666-885c-160cb36579ee/heartbeat \
  -H "Content-Type: application/json" \
  -d '{"battery": 45, "signalStrength": -98}'
```

## 📚 Feature Summary

| Feature | Status |
|---------|--------|
| 3 Configured Nodes | ✅ Complete |
| Admin + Test Groups | ✅ Complete |
| Admin User Created | ✅ Complete |
| 3 Web Pages (2 with images) | ✅ Complete |
| Embedded SVG Images | ✅ Implemented |
| Node Hosting Router | ✅ Implemented |
| Page Content Retrieval | ✅ Working |
| Node Info Endpoint | ✅ Working |
| Heartbeat/Status Updates | ✅ Working |
| Docker Integration | ✅ Tested |
| API Documentation | ✅ Complete |
| Test Scripts | ✅ Available |

## 🚨 Troubleshooting

**Pages not showing?**
- Check: `SELECT COUNT(*) FROM pages;`
- Should show: 3

**Node not found?**
- Check: `SELECT COUNT(*) FROM nodes WHERE functionalName LIKE 'MeshNode%';`
- Should show: 3

**API not responding?**
- Check: `curl http://localhost:3001/health`
- Should return: `{"status":"ok",...}`

**Docker containers down?**
- Check: `docker ps`
- Restart: `docker-compose up -d --build`

## 📖 Full Documentation

See `NODES_WEBHOSTING_README.md` for complete documentation including:
- Architecture overview
- Complete API reference
- Database schema
- Setup instructions
- Usage examples
- Future enhancements
