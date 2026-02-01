// MeshNet Protocol Header File
// Definities voor communicatie messages en data structures

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <Arduino.h>
#include <ArduinoJSON.h>

// ============ MESSAGE TYPES ============
enum MessageType {
  MSG_DISCOVER = 0,      // Node aanmelding in mesh
  MSG_STATUS = 1,        // Status rapportage
  MSG_RELAY = 2,         // Doorsturen berichten
  MSG_CONFIG = 3,        // Configuratie update
  MSG_AUTH = 4,          // Authenticatie sync
  MSG_PAGE = 5,          // Pagina content
  MSG_KEEPALIVE = 6,     // Connection heartbeat
  MSG_UNKNOWN = 255
};

// ============ DATA STRUCTURES ============

// Node ID storage structure
struct NodeID {
  uint8_t mac[6];                // MAC address (6 bytes)
  char functionalName[32];       // Friendly name
  
  // Helper: Get full node ID as string (AA:BB:CC:DD:EE:FF:name)
  String toString() {
    char buffer[64];
    snprintf(buffer, sizeof(buffer),
      "%02X:%02X:%02X:%02X:%02X:%02X:%s",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
      functionalName);
    return String(buffer);
  }
  
  // Helper: Get MAC as string (AA:BB:CC:DD:EE:FF)
  String getMacString() {
    char buffer[18];
    snprintf(buffer, sizeof(buffer),
      "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buffer);
  }
};

// Base message structure
struct Message {
  MessageType type;
  String from;               // Node ID of sender
  String to;                 // Node ID or BROADCAST
  String msgId;              // Unique message ID
  uint32_t timestamp;        // Unix timestamp
  uint8_t ttl;              // Time to live (hops remaining)
  uint8_t hops;             // Hops traveled
  JsonDocument data;        // Message payload
  
  // Helper: Convert to JSON
  JsonDocument toJson() {
    JsonDocument doc;
    doc["type"] = type;
    doc["from"] = from;
    doc["to"] = to;
    doc["msgId"] = msgId;
    doc["timestamp"] = timestamp;
    doc["ttl"] = ttl;
    doc["hops"] = hops;
    doc["data"] = data;
    return doc;
  }
  
  // Helper: Parse from JSON
  static Message fromJson(JsonDocument &doc) {
    Message msg;
    msg.type = (MessageType)doc["type"].as<int>();
    msg.from = doc["from"].as<String>();
    msg.to = doc["to"].as<String>();
    msg.msgId = doc["msgId"].as<String>();
    msg.timestamp = doc["timestamp"].as<uint32_t>();
    msg.ttl = doc["ttl"].as<uint8_t>();
    msg.hops = doc["hops"].as<uint8_t>();
    msg.data = doc["data"];
    return msg;
  }
};

// DISCOVER message data
struct DiscoverData {
  String nodeId;
  String functionalName;
  String version;
  int8_t signalStrength;     // RSSI
  uint8_t battery;           // Percentage
  uint32_t uptime;           // Seconds
};

// STATUS message data
struct StatusData {
  String nodeId;
  int8_t rssi;
  uint8_t battery;
  uint8_t connectedNodes;
  uint32_t lastPageUpdate;
  uint8_t activeUsers;
};

// CONFIG message data
struct ConfigData {
  String nodeId;
  String functionalName;
  String wifiSSID;
  String wifiPassword;
  bool meshEnabled;
  uint16_t discoveryInterval;
};

// AUTH message data
struct AuthData {
  enum Action { AUTH_LOGIN, AUTH_UPDATE, AUTH_LOGOUT };
  
  String action;             // LOGIN, UPDATE, LOGOUT
  String userId;
  String groupId;
  String groupName;
  String token;              // JWT token
  uint32_t expiresAt;
  // permissions array (handled in JSON)
};

// PAGE message data
struct PageData {
  String nodeId;
  String pageId;
  String groupId;
  String title;
  String content;            // HTML
  String imageUrl;
  uint16_t refreshInterval;  // seconds
};

// Node connection info
struct NodeConnection {
  String nodeId;
  int8_t rssi;              // Signal strength
  uint8_t hops;             // Distance in hops
  uint32_t lastSeen;        // Timestamp
  uint8_t messageCount;     // Messages received
};

// User session info (cached on node)
struct UserSession {
  String userId;
  String groupId;
  String groupName;
  String token;
  uint32_t expiresAt;
  uint32_t loginTime;
};

// ============ CONSTANTS ============
const char* MSG_TYPE_STRINGS[] = {
  "DISCOVER", "STATUS", "RELAY", "CONFIG", "AUTH", "PAGE", "KEEPALIVE", "UNKNOWN"
};

#endif // PROTOCOL_H
