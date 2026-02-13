#!/bin/bash

# MeshNet Project Setup Script
# Initializes the complete MeshNet system on RPI

set -e

echo "╔═══════════════════════════════════════╗"
echo "║  MeshNet Project Setup                ║"
echo "╚═══════════════════════════════════════╝"
echo ""

# Check prerequisites
echo "Checking prerequisites..."

# Check Docker
if ! command -v docker &> /dev/null; then
    echo "✗ Docker not found. Installing Docker..."
    curl -fsSL https://get.docker.com -o get-docker.sh
    sh get-docker.sh
    rm get-docker.sh
fi

# Check Docker Compose v2
if ! command -v docker &> /dev/null || ! docker compose version &> /dev/null; then
    echo "✗ Docker Compose v2 not found. Please install Docker Compose v2 (no sudo used by this script)."
    exit 1
fi

echo "✓ Prerequisites OK"
echo ""

# Create necessary directories
echo "Creating directory structure..."
mkdir -p rpi/docker/mosquitto/{config,data}
mkdir -p rpi/docker/mysql/data
mkdir -p rpi/webserver/public/{pages,assets/images}
mkdir -p rpi/backend/data
echo "✓ Directories created"
echo ""

# Copy configuration files
echo "Setting up configuration..."
cp rpi/docker/docker-compose.yml ./
cp rpi/network/setup-network.sh ./network-setup.sh
chmod +x ./network-setup.sh
echo "✓ Configuration copied"
echo ""

# Build and start Docker containers
echo "Building Docker images..."
cd rpi/docker
docker compose build --no-cache
echo "✓ Docker images built"
echo ""

echo "Starting Docker containers..."
docker compose up -d
echo "✓ Containers started"
echo ""

# Wait for services to be ready
echo "Waiting for services to initialize..."
sleep 10

# Check container health
echo "Checking container status..."
docker compose ps
echo ""

# Initialize database
echo "Initializing database..."
docker compose exec -T mysql mysql -u meshnet -pmeshnet_secure_pwd meshnet < mysql.sql
echo "✓ Database initialized"
echo ""

echo "╔═══════════════════════════════════════╗"
echo "║  Setup Complete!                      ║"
echo "╚═══════════════════════════════════════╝"
echo ""
echo "Services running:"
echo "  - Backend API:      http://localhost:3001"
echo "  - Web Dashboard:    http://localhost"
echo "  - MySQL:            localhost:3306"
echo "  - MQTT (optional):  localhost:1883"
echo ""
echo "Next steps:"
echo "1. Configure network: ./network-setup.sh (requires root, run manually if needed)"
echo "2. Flash your first node with node/mesh_node.ino"
echo "3. Access dashboard at http://$(hostname -I | awk '{print $1}')"
echo ""
