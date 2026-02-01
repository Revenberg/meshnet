# MeshNet Docker Testing Guide

Dit document beschrijft hoe je de Docker containers lokaal op je Windows machine kunt testen.

## ✅ Vereisten

Je hebt al:
- ✅ Docker Desktop for Windows (29.1.3)
- ✅ Docker Compose (5.0.0)
- ✅ WSL2 (Windows Subsystem for Linux 2)

**Niets extra nodig installeren!**

---

## 🚀 Quick Start (1 minuut)

### Optie 1: Windows GUI (Makkelijker)
```bash
# Double-click op:
test-docker.bat
```

### Optie 2: PowerShell/WSL
```bash
cd C:\Users\reven\Documents\Arduino\MeshNet
cd rpi\docker
docker-compose up -d
```

---

## 📊 Wat Gebeurt Er

Zodra je het start:

1. **Docker images gedownload** (~500MB totaal)
   - Backend (Node.js)
   - Webserver (Node.js)
   - MySQL (8.0)
   - Mosquitto MQTT (optioneel)

2. **Containers gestart**
   ```
   meshnet-backend  (port 3001)
   meshnet-webserver (port 80)
   meshnet-mysql    (port 3306)
   meshnet-mqtt     (port 1883)
   ```

3. **Database geïnitialiseerd**
   - 8 tabellen aangemaakt
   - 10 indexes aangemaakt
   - Klaar voor data

---

## 🌐 Test de Services

### Test 1: Backend API
```bash
# In PowerShell/Terminal:
curl http://localhost:3001/health

# Expected response:
# {"status":"ok","timestamp":"2026-01-28T..."}
```

### Test 2: Dashboard
```bash
# In browser:
http://localhost

# Expected: MeshNet Dashboard loaded
```

### Test 3: MySQL Database
```bash
# List tables:
docker-compose exec mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet -e "SHOW TABLES;"

# Expected: 8 tabellen (users, groups, nodes, etc.)
```

### Test 4: API Endpoints
```bash
# Get all nodes:
curl http://localhost:3001/api/nodes

# Get all users:
curl http://localhost:3001/api/users

# Get all groups:
curl http://localhost:3001/api/groups
```

---

## 📈 Monitoring

### View Real-time Logs
```bash
# All containers
docker-compose logs -f

# Specific service
docker-compose logs -f backend
docker-compose logs -f webserver
docker-compose logs -f mysql
```

### Check Container Status
```bash
docker-compose ps

# Output shows:
# NAME              STATUS       PORTS
# meshnet-backend   Up 2 minutes 0.0.0.0:3001->3001/tcp
# meshnet-webserver Up 2 minutes 0.0.0.0:80->80/tcp
# meshnet-mysql     Up 2 minutes 0.0.0.0:3306->3306/tcp
```

### Inspect Container
```bash
# Inside a container
docker-compose exec backend bash
docker-compose exec mysql bash

# Exit with: exit
```

---

## 🗄️ Database Testing

### Connect to MySQL
```bash
docker-compose exec mysql mysql -u meshnet -pmeshnet_secure_pwd
```

Then in MySQL:
```sql
USE meshnet;

-- View all tables
SHOW TABLES;

-- Check users table
SELECT * FROM users;

-- Check nodes table
SELECT * FROM nodes;

-- Check structure
DESCRIBE users;
```

### Insert Test Data
```sql
-- Create a test group
INSERT INTO groups (groupId, name, description) 
VALUES ('group-admin', 'Administrators', 'Admin users');

-- Check result
SELECT * FROM groups;
```

---

## 🔧 Management Commands

### Stop Services (Keep Data)
```bash
docker-compose stop
```

### Start Services (Resume)
```bash
docker-compose start
```

### Restart Services
```bash
docker-compose restart
```

### Stop & Remove Containers
```bash
docker-compose down
```

### Complete Reset (DELETE EVERYTHING)
```bash
docker-compose down -v
```

### Rebuild Images
```bash
# Rebuilds from Dockerfile
docker-compose build --no-cache

# Then start
docker-compose up -d
```

---

## 🐛 Troubleshooting

### Port Already in Use
```
Error: bind: address already in use
```

**Solution**:
```bash
# Find what's using the port
netstat -ano | findstr :3001

# Kill the process
taskkill /PID <PID> /F

# Or change ports in docker-compose.yml
```

### Containers Won't Start
```bash
# Check logs
docker-compose logs

# Common issues:
# - Image download failed → Try: docker-compose pull
# - Port conflicts → Check netstat above
# - Disk space → Clean: docker system prune
```

### MySQL Connection Failed
```
Error: Can't connect to MySQL server
```

**Solution**:
```bash
# Wait longer (MySQL takes time)
docker-compose logs mysql

# Reset if needed
docker-compose down -v
docker-compose up -d
```

### Docker Desktop Not Running
```bash
# Restart Docker Desktop
# Or from PowerShell:
restart-service docker
```

---

## 🧪 Full Test Scenario

1. **Start containers**
   ```bash
   cd rpi/docker
   docker-compose up -d
   ```

2. **Wait for startup** (30-60 seconds)

3. **Check backend**
   ```bash
   curl http://localhost:3001/health
   ```

4. **Check dashboard**
   - Open browser: `http://localhost`

5. **Check database**
   ```bash
   docker-compose exec mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet -e "SHOW TABLES;"
   ```

6. **Insert test data**
   ```bash
   docker-compose exec mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet -e "INSERT INTO groups (groupId, name) VALUES ('test-group', 'Test Group');"
   ```

7. **Query test data**
   ```bash
   curl http://localhost:3001/api/groups
   ```

**Expected**: JSON response met de test group

---

## 📊 Performance Notes

### First Run
- **Download time**: 2-3 minutes (depends on internet)
- **Startup time**: 30-60 seconds
- **Disk space**: ~1.5 GB

### Subsequent Runs
- **Startup time**: 5-10 seconds
- **No download needed**

### Resource Usage
- **CPU**: Minimal when idle
- **Memory**: ~400-600 MB total
- **Disk**: ~1.5 GB (persistent MySQL data)

---

## 🔒 Security Notes (Development Only)

⚠️ **These credentials are FOR DEVELOPMENT ONLY**:
```
DB_USER: meshnet
DB_PASSWORD: meshnet_secure_pwd
JWT_SECRET: (not set - use dummy)
```

**For Production**:
- Change all credentials
- Use strong passwords
- Set JWT_SECRET
- Use HTTPS
- Restrict database access
- Enable authentication on MQTT

---

## 💾 Data Persistence

### What Persists
- MySQL data in `mysql/data/`
- Backend logs
- User sessions

### What Gets Reset
- Container image changes
- Environment variable changes
- Docker Compose config changes

### Backup Data
```bash
# Backup MySQL
docker-compose exec mysql mysqldump -u meshnet -pmeshnet_secure_pwd meshnet > backup.sql

# Restore from backup
docker-compose exec -T mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet < backup.sql
```

---

## 🎯 Testing Checklist

- [ ] Docker Desktop running
- [ ] Containers started: `docker-compose up -d`
- [ ] Backend responds: `curl http://localhost:3001/health`
- [ ] Dashboard loads: `http://localhost`
- [ ] MySQL tables exist: `docker-compose exec mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet -e "SHOW TABLES;"`
- [ ] API endpoints working: `curl http://localhost:3001/api/nodes`
- [ ] Database insert test works
- [ ] Logs show no errors: `docker-compose logs`

---

## 📚 Next Steps

### After Successful Test
1. Proceed to Phase 2 (Node Implementation)
2. Try adding test data via API
3. Experiment with SQL queries
4. Check logs for any issues

### Making Changes
1. Edit Docker Compose → Rebuild: `docker-compose build --no-cache`
2. Edit Node.js code → Restart: `docker-compose restart backend`
3. Edit MySQL schema → Add migration scripts

---

## 🆘 Quick Help

**Problem** → **Solution**

Port 3001 in use → Kill process or change port in docker-compose.yml

MySQL won't start → Check disk space, wait 60s, view logs

Dashboard blank → Check webserver logs with `docker-compose logs webserver`

API errors → Check backend logs: `docker-compose logs backend`

Can't connect to MySQL → Verify credentials in docker-compose.yml

Containers crash → View logs: `docker-compose logs` and check resource usage

---

## 📞 Verify Everything Works

Run this in PowerShell:
```powershell
# Quick verification script
cd rpi/docker
docker-compose ps
curl http://localhost:3001/health
curl http://localhost/health
docker-compose exec mysql mysql -u meshnet -pmeshnet_secure_pwd -e "USE meshnet; SHOW TABLES;"
```

**If all commands succeed** → ✅ Docker environment working perfectly!

---

**Last Updated**: 28-01-2026  
**Status**: Ready for Testing ✅  
**Environment**: Windows 10/11 with Docker Desktop
