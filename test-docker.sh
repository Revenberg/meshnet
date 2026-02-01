#!/bin/bash
# MeshNet Docker Testing Script
# For Windows with Docker Desktop + WSL2

set -e

echo "╔════════════════════════════════════════╗"
echo "║  MeshNet Docker Test Setup             ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check Docker
echo "Checking Docker installation..."
if ! command -v docker &> /dev/null; then
    echo -e "${RED}✗ Docker not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Docker: $(docker --version)${NC}"

# Check Docker Compose
if ! command -v docker-compose &> /dev/null; then
    echo -e "${RED}✗ Docker Compose not found${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Docker Compose: $(docker-compose --version)${NC}"

# Navigate to docker directory
cd "$(dirname "$0")/rpi/docker" || exit

echo ""
echo "Creating necessary directories..."
mkdir -p mysql/data
mkdir -p mosquitto/{config,data}
mkdir -p backend/data
echo -e "${GREEN}✓ Directories created${NC}"

echo ""
echo "Starting Docker containers..."
echo "(This will download images on first run - may take a few minutes)"
echo ""

docker-compose up -d

sleep 5

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  Container Status                      ║"
echo "╚════════════════════════════════════════╝"
docker-compose ps

echo ""
echo "Waiting for services to initialize..."
sleep 10

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  Testing Services                      ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Test Backend API
echo "Testing Backend API..."
if curl -s http://localhost:3001/health > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Backend API responding${NC}"
    curl -s http://localhost:3001/health | jq '.' 2>/dev/null || curl -s http://localhost:3001/health
else
    echo -e "${YELLOW}⚠ Backend API not ready yet (may still be starting)${NC}"
fi

echo ""

# Test Webserver
echo "Testing Webserver..."
if curl -s http://localhost/health > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Webserver responding${NC}"
else
    echo -e "${YELLOW}⚠ Webserver not ready yet (may still be starting)${NC}"
fi

echo ""

# Test MySQL
echo "Testing MySQL..."
if docker-compose exec -T mysql mysqladmin ping -u meshnet -pmeshnet_secure_pwd &> /dev/null; then
    echo -e "${GREEN}✓ MySQL responding${NC}"
else
    echo -e "${YELLOW}⚠ MySQL not ready yet${NC}"
fi

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  Access Points                         ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "Local Access:"
echo "  Dashboard:  http://localhost"
echo "  Backend API: http://localhost:3001"
echo "  MySQL:      localhost:3306"
echo "  MQTT:       localhost:1883"
echo ""
echo "Database Credentials:"
echo "  User: meshnet"
echo "  Password: meshnet_secure_pwd"
echo "  Database: meshnet"
echo ""
echo "MySQL Access:"
echo "  docker-compose exec mysql mysql -u meshnet -p meshnet"
echo ""
echo "View Logs:"
echo "  docker-compose logs -f              # All services"
echo "  docker-compose logs -f backend      # Backend only"
echo "  docker-compose logs -f webserver    # Webserver only"
echo "  docker-compose logs -f mysql        # MySQL only"
echo ""
echo "Stop Services:"
echo "  docker-compose down"
echo ""
echo "Remove Volumes (reset everything):"
echo "  docker-compose down -v"
echo ""
echo -e "${GREEN}Setup complete!${NC}"
echo ""
