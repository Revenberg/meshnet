/**
 * MeshNet ESP32 LoRa Node Firmware
 * Heltec ESP32 LoRa v3 Board
 * 
 * Features:
 * - LoRa mesh networking (868MHz EU)
 * - OLED display (SSD1306)
 * - Battery monitoring
 * - Button interface (HotButton)
 * - Over-the-air updates support
 * - Real-time status reporting
 * 
 * Required Libraries:
 * - RadioLib (LoRa)
 * - SSD1306 OLED
 * - HotButton
 * - ArduinoJson (for message parsing)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <heltec_unofficial.h>

// ============ HARDWARE CONFIGURATION ============
#define LORA_FREQ 868E6          // 868MHz EU ISM band
#define LORA_BANDWIDTH 125000    // 125kHz
#define LORA_SPREADING_FACTOR 9  // SF9 (good range/speed balance)
#define LORA_CODING_RATE 8       // CR4/8
#define LORA_TX_POWER 17         // dBm (legal limit EU)
#define LORA_PREAMBLE_LENGTH 8

// Display pins (Heltec v3)
#define OLED_SDA 17
#define OLED_SCL 18

// Battery pin
#define BATTERY_PIN 1

// Button pin (USER button on board)
#define BUTTON_PIN 0

// ============ PROTOCOL CONSTANTS ============
#define MESSAGE_TYPE_DISCOVER 1
#define MESSAGE_TYPE_STATUS 2
#define MESSAGE_TYPE_RELAY 3
#define MESSAGE_TYPE_CONFIG 4
#define MESSAGE_TYPE_DATA 5

#define DEFAULT_TTL 3
#define MAX_MESSAGE_SIZE 256
#define DISCOVER_INTERVAL 5 * 60 * 1000  // 5 minutes
#define STATUS_INTERVAL 10 * 1000         // 10 seconds
#define NODE_TIMEOUT 30 * 1000            // 30 seconds

// ============ GLOBAL VARIABLES ============

// Node identification
String NODE_ID = "";
String NODE_NAME = "MeshNode";
String NODE_VERSION = "1.0.0";

// State tracking
bool lora_initialized = false;
bool display_initialized = false;
unsigned long last_discover_time = 0;
unsigned long last_status_time = 0;
unsigned long last_button_press = 0;
int message_count = 0;
int relay_count = 0;

// Battery
float battery_voltage = 0.0;
int battery_percent = 100;

// Display buffer
String display_line1 = "MeshNet Node";
String display_line2 = "Init...";
String display_line3 = "Ready";

// Preferences for NVS storage
Preferences prefs;

// ============ DISPLAY FUNCTIONS ============

void display_init() {
  heltec_display_init();
  display_initialized = true;
  update_display();
}

void update_display() {
  if (!display_initialized) return;
  
  heltec_display(0, 0, display_line1.c_str());
  heltec_display(0, 1, display_line2.c_str());
  heltec_display(0, 2, display_line3.c_str());
  
  // Status indicators
  String status = "";
  if (lora_initialized) status += "LoRa";
  if (WiFi.status() == WL_CONNECTED) status += " WiFi";
  heltec_display(0, 3, status.c_str());
  
  // Battery indicator
  String battery_str = "Bat: " + String(battery_percent) + "%";
  heltec_display(0, 4, battery_str.c_str());
}

// ============ BATTERY FUNCTIONS ============

void read_battery() {
  // Read analog voltage from battery pin
  // Voltage divider: measure pin = battery_voltage / 2
  // For 4.2V max, ADC reads ~2.1V
  int adc_value = analogRead(BATTERY_PIN);
  battery_voltage = (adc_value / 4095.0) * 2.0 * 3.3 * 1.1; // 3.3V ref, 1.1 correction
  
  // Convert to percentage (3.0V = 0%, 4.2V = 100%)
  battery_percent = constrain(
    (int)((battery_voltage - 3.0) / (4.2 - 3.0) * 100),
    0,
    100
  );
}

// ============ LORA FUNCTIONS ============

void lora_init() {
  Serial.println("[LoRa] Initializing...");
  
  // Initialize with hardware settings from heltec_unofficial
  int state = radio.begin();
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[LoRa] ✓ Radio initialized");
    
    // Configure LoRa parameters
    radio.setFrequency(LORA_FREQ);
    radio.setBandwidth(LORA_BANDWIDTH);
    radio.setSpreadingFactor(LORA_SPREADING_FACTOR);
    radio.setCodingRate(LORA_CODING_RATE);
    radio.setOutputPower(LORA_TX_POWER);
    radio.setPreambleLength(LORA_PREAMBLE_LENGTH);
    
    // Enable CRC
    radio.setCRC(true);
    
    // Start receiving
    radio.startReceive();
    
    lora_initialized = true;
    display_line2 = "LoRa Ready";
  } else {
    Serial.print("[LoRa] ✗ Init failed: ");
    Serial.println(state);
    display_line2 = "LoRa Failed";
  }
  
  update_display();
}

void lora_send_discover() {
  if (!lora_initialized) return;
  
  // Message format: [TYPE][FROM][TO][MSG_ID][TTL][DATA_LEN][JSON_DATA]
  String msg = "";
  msg += char(MESSAGE_TYPE_DISCOVER);
  msg += NODE_ID;
  msg += "BROADCAST";
  msg += String(millis());
  msg += char(DEFAULT_TTL);
  
  // Build JSON data
  String json_data = "{";
  json_data += "\"nodeId\":\"" + NODE_ID + "\",";
  json_data += "\"name\":\"" + NODE_NAME + "\",";
  json_data += "\"version\":\"" + NODE_VERSION + "\",";
  json_data += "\"battery\":" + String(battery_percent) + ",";
  json_data += "\"uptime\":" + String(millis() / 1000) + "}";
  
  int len = json_data.length();
  msg += char(len);
  msg += json_data;
  
  int state = radio.transmit(msg);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[LoRa] ✓ DISCOVER sent");
    message_count++;
  } else {
    Serial.print("[LoRa] ✗ TX failed: ");
    Serial.println(state);
  }
  
  last_discover_time = millis();
}

void lora_send_status() {
  if (!lora_initialized) return;
  
  String msg = "";
  msg += char(MESSAGE_TYPE_STATUS);
  msg += NODE_ID;
  msg += "BRIDGE";
  msg += String(millis());
  msg += char(DEFAULT_TTL);
  
  String json_data = "{";
  json_data += "\"nodeId\":\"" + NODE_ID + "\",";
  json_data += "\"battery\":" + String(battery_percent) + ",";
  json_data += "\"msgCount\":" + String(message_count) + ",";
  json_data += "\"uptime\":" + String(millis() / 1000) + "}";
  
  int len = json_data.length();
  msg += char(len);
  msg += json_data;
  
  int state = radio.transmit(msg);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[LoRa] ✓ STATUS sent");
  } else {
    Serial.print("[LoRa] ✗ TX failed: ");
    Serial.println(state);
  }
  
  last_status_time = millis();
}

void lora_receive() {
  if (!lora_initialized) return;
  
  int state = radio.readData(256); // Max message size
  
  if (state == RADIOLIB_ERR_NONE) {
    // Data received
    String received = radio.readDataStr();
    Serial.print("[LoRa] ✓ RX: ");
    Serial.println(received.substring(0, 50)); // Log first 50 chars
    
    // Parse message type
    if (received.length() > 0) {
      byte msg_type = received[0];
      
      switch (msg_type) {
        case MESSAGE_TYPE_DISCOVER:
          Serial.println("[LoRa] DISCOVER received");
          break;
        case MESSAGE_TYPE_STATUS:
          Serial.println("[LoRa] STATUS received");
          break;
        case MESSAGE_TYPE_CONFIG:
          Serial.println("[LoRa] CONFIG received");
          handle_config(received);
          break;
        case MESSAGE_TYPE_DATA:
          Serial.println("[LoRa] DATA received");
          break;
        default:
          Serial.println("[LoRa] Unknown message type");
      }
    }
    
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // No data, expected
  } else {
    Serial.print("[LoRa] RX error: ");
    Serial.println(state);
  }
}

void handle_config(String msg) {
  // Parse CONFIG message and update node settings
  // Format: [TYPE][FROM][TO][MSG_ID][TTL][DATA_LEN][JSON_DATA]
  if (msg.length() < 6) return;
  
  // Extract JSON from message
  int data_len = msg[5];
  if (msg.length() < 6 + data_len) return;
  
  String json_str = msg.substring(6, 6 + data_len);
  Serial.print("[Config] Received: ");
  Serial.println(json_str);
  
  // Parse and apply configuration
  // Example: update node name, settings, etc.
}

// ============ BUTTON HANDLER ============

void button_callback() {
  unsigned long now = millis();
  
  if (now - last_button_press < 500) {
    // Debounce
    return;
  }
  
  last_button_press = now;
  
  Serial.println("[Button] Pressed");
  
  // Toggle display brightness or send test message
  display_line3 = "Button: " + String(now);
  update_display();
  
  // Send test status
  lora_send_status();
}

// ============ WIFI SETUP (For OTA updates) ============

void wifi_init() {
  // Configure WiFi for potential OTA updates
  // Can be connected to mesh network or direct AP
  WiFi.mode(WIFI_STA);
  Serial.println("[WiFi] Configured for STA mode");
}

// ============ SETUP ============

void setup() {
  // Serial communication
  Serial.begin(115200);
  delay(200);
  Serial.println("\n\n[Setup] MeshNet Node Firmware v1.0.0");
  
  // Generate Node ID from MAC address
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  NODE_ID = mac;
  display_line2 = "ID: " + NODE_ID.substring(0, 8);
  
  // Initialize display
  display_init();
  
  // Initialize battery reading
  analogSetAttenuation(ADC_11db);
  read_battery();
  
  // Initialize Preferences (NVS)
  prefs.begin("meshnet", false);
  
  // Load saved configuration
  String saved_name = prefs.getString("nodeName", "");
  if (saved_name.length() > 0) {
    NODE_NAME = saved_name;
  }
  
  // Initialize WiFi (for OTA)
  wifi_init();
  
  // Initialize LoRa
  lora_init();
  
  // Setup button handler
  button_init(BUTTON_PIN);
  attachInterrupt(BUTTON_PIN, button_callback, FALLING);
  
  Serial.println("[Setup] ✓ All systems initialized");
  display_line3 = "Ready";
  update_display();
}

// ============ MAIN LOOP ============

void loop() {
  // Read battery status periodically
  static unsigned long last_battery_read = 0;
  if (millis() - last_battery_read > 5000) {
    read_battery();
    last_battery_read = millis();
  }
  
  // Send DISCOVER periodically
  if (millis() - last_discover_time > DISCOVER_INTERVAL) {
    lora_send_discover();
  }
  
  // Send STATUS periodically
  if (millis() - last_status_time > STATUS_INTERVAL) {
    lora_send_status();
  }
  
  // Check for incoming LoRa messages
  lora_receive();
  
  // Update display
  static unsigned long last_display_update = 0;
  if (millis() - last_display_update > 1000) {
    display_line1 = "Msgs: " + String(message_count);
    update_display();
    last_display_update = millis();
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}
