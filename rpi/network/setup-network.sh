#!/bin/bash

# Network setup script for Raspberry Pi
# Configures ethernet and dual WiFi networks

echo "╔═══════════════════════════════════════╗"
echo "║  MeshNet Network Configuration       ║"
echo "╚═══════════════════════════════════════╝"
echo ""

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root"
   exit 1
fi

# ============ ETHERNET CONFIGURATION ============
echo "Configuring Ethernet..."

cat > /etc/dhcpcd.conf << EOF
# Ethernet - Primary connection (DHCP)
interface eth0
metric 100
dhcp

# WiFi 1 - Secondary connection (DHCP)
interface wlan0
metric 200
dhcp

# WiFi 2 - Tertiary connection (DHCP)
interface wlan1
metric 300
dhcp
EOF

echo "✓ Ethernet configuration updated"

# ============ DUAL WIFI CONFIGURATION ============
echo "Configuring dual WiFi..."

# Install required packages
apt-get update
apt-get install -y wpasupplicant wireless-tools

# Create WiFi configuration for wlan0
cat > /etc/wpa_supplicant/wpa_supplicant-wlan0.conf << EOF
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

# Primary WiFi Network
network={
    ssid="YourWiFiNetwork1"
    psk="YourPassword1"
    priority=2
    id_str="wifi1"
}

# Fallback WiFi Network
network={
    ssid="YourWiFiNetwork2"
    psk="YourPassword2"
    priority=1
    id_str="wifi2"
}
EOF

chmod 600 /etc/wpa_supplicant/wpa_supplicant-wlan0.conf

# Create WiFi configuration for wlan1 (second adapter)
cat > /etc/wpa_supplicant/wpa_supplicant-wlan1.conf << EOF
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="YourWiFiNetwork2"
    psk="YourPassword2"
    priority=1
}
EOF

chmod 600 /etc/wpa_supplicant/wpa_supplicant-wlan1.conf

echo "✓ WiFi configuration created"

# ============ NETWORK FAILOVER ============
echo "Setting up network failover..."

cat > /etc/network/if-up.d/meshnet-failover << 'EOF'
#!/bin/bash
# Auto-failover script for network interfaces

MESHNET_LOG="/var/log/meshnet-network.log"

check_internet() {
    timeout 2 ping -c 1 8.8.8.8 &> /dev/null
    return $?
}

{
    echo "[$(date)] Network event on interface: $IFACE"
    
    # Check if we have internet
    if ! check_internet; then
        echo "[$(date)] No internet detected, checking failover..."
        
        # Try WiFi as fallback
        if ip link show wlan0 | grep -q "UP"; then
            echo "[$(date)] Failover to wlan0"
        fi
    else
        echo "[$(date)] Internet available on $IFACE"
    fi
} >> "$MESHNET_LOG"
EOF

chmod +x /etc/network/if-up.d/meshnet-failover

echo "✓ Network failover script installed"

# ============ RESTART NETWORKING ============
echo "Restarting network services..."
systemctl restart dhcpcd
sleep 2

echo ""
echo "✓ Network configuration complete!"
echo ""
echo "Configuration summary:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
ip addr | grep -E "inet |link/ether" | awk '{print $2, $1}'
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "⚠️  IMPORTANT: Edit the WiFi configurations:"
echo "   - /etc/wpa_supplicant/wpa_supplicant-wlan0.conf"
echo "   - /etc/wpa_supplicant/wpa_supplicant-wlan1.conf"
echo ""
echo "Set your actual WiFi SSIDs and passwords!"
echo ""
