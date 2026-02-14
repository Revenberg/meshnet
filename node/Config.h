// MeshNet Node Configuration
// Centrale configuratie voor alle node instellingen

#ifndef CONFIG_H
#define CONFIG_H

// ============ HARDWARE CONFIGURATIE ============
#define LORA_BAND 915E6           // LoRa frequentie: 915 MHz (US) of 868E6 (EU)
#define LORA_BANDWIDTH 125000     // Bandbreedte in Hz
#define LORA_SPREADING_FACTOR 10  // SF7-SF12 (hogere = groter bereik, tragere data)
#define LORA_CODING_RATE 5        // CR 4/5, 4/6, 4/7, 4/8
#define LORA_TX_POWER 17          // TX Power 2-17 dBm
#define LORA_PREAMBLE_LENGTH 8    // Preamble lengte

// Heltec V3 Pin Definitions
#define LORA_MOSI 10              // DIN
#define LORA_MISO 11              // DOUT (alternative pins, verify for V3)
#define LORA_SCK 9                // SCK
#define LORA_SS 8                 // CS/NSS
#define LORA_RESET 12             // RESET
#define LORA_DIO0 14              // IRQ
#define LORA_DIO1 13              // (optional)

// OLED Display (SSD1306)
#define OLED_I2C_ADDR 0x3C        // I2C address
#define OLED_WIDTH 128            // pixels
#define OLED_HEIGHT 64            // pixels
#define OLED_RESET 21             // Reset pin (may vary)

// Battery monitoring
#define ADC_PIN 34                // ADC pin for battery voltage
#define ADC_RESOLUTION 12         // bits
#define BATTERY_VOLTAGE_MAX 4.2   // Fully charged voltage
#define BATTERY_VOLTAGE_MIN 3.0   // Minimum safe voltage

// ============ MESH NETWORK CONFIGURATIE ============
#define MESH_BROADCAST_INTERVAL 300000   // 5 minutes in ms
#define MESH_KEEPALIVE_INTERVAL 5000     // 5 seconds in ms
#define MESH_KEEPALIVE_TIMEOUT 15000     // 15 seconds before reconnect
#define MESH_MAX_TTL 10                  // Max hops per message
#define MESH_MAX_MESSAGE_SIZE 1024       // bytes
#define MESH_RELAY_TIMEOUT 5000          // ms to wait for relay response

// ============ WEBSERVER CONFIGURATIE ============
#define WIFI_AP_SSID_PREFIX "MeshNode-"  // SSID = MeshNode-AABBCCDDEE
#define WIFI_AP_PASSWORD "meshnetwork"
#define WIFI_AP_MAX_CLIENTS 4
#define WEBSERVER_PORT 80
#define WEBSERVER_TIMEOUT 30000          // 30 seconds

// ============ STORAGE CONFIGURATIE ============
#define SPIFFS_PATH "/mesh/"
#define CONFIG_FILE "/mesh/node.json"
#define AUTH_CACHE_FILE "/mesh/auth.json"
#define PAGES_CACHE_DIR "/mesh/pages/"
#define MAX_CACHED_USERS 10
#define MAX_CACHED_PAGES 5

// ============ DISPLAY CONFIGURATIE ============
#define DISPLAY_UPDATE_INTERVAL 1000     // Update display every 1 sec
#define DISPLAY_BRIGHT_TIME 30000        // Keep bright for 30 sec after activity
#define DISPLAY_DIM_BRIGHTNESS 50        // 0-255
#define DISPLAY_BRIGHT_BRIGHTNESS 255

// ============ TIMING CONFIGURATIE ============
#define STARTUP_DELAY 2000               // ms before joining mesh
#define AUTH_TOKEN_REFRESH_INTERVAL 3600000  // 1 hour
#define STATUS_REPORT_INTERVAL 10000     // Report every 10 sec
#define CONNECTION_SCAN_INTERVAL 60000   // Scan for neighbors every 1 min

// ============ DEBUG CONFIGURATIE ============
#define DEBUG_ENABLED 1
#define DEBUG_BAUD_RATE 115200
#define DEBUG_TOPICS "LORA|MESH|DISPLAY|AUTH|WEBSERVER"

// ============ VERSION ============
#define MESHNET_VERSION "2.0.3"
#define NODE_PROTOCOL_VERSION "2.0.3"

// ============ FEATURE FLAGS ============
#define FEATURE_ENABLE_MESH_ROUTING 1
#define FEATURE_ENABLE_WEBSERVER 1
#define FEATURE_ENABLE_DISPLAY 1
#define FEATURE_ENABLE_BATTERY_MONITORING 1
#define FEATURE_ENABLE_AUTH_CACHE 1

#endif // CONFIG_H
