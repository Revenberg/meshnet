#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <RadioLib.h>
#include <Preferences.h>
#include "User.h"
#include "version.h"

// =======================
// LoRa settings
// =======================
#define LORA_CS 8
#define LORA_RST 12
#define LORA_BUSY 13
#define LORA_DIO1 14

// Max limits
#define MAX_MSGS 50
#define MAX_ONLINE 20

// =======================
// Message struct
// =======================
struct NodeMessage
{
  String msgId;
  String user;
  int TTL;
  unsigned long timestamp;
  String object;
  String function;
  String parameters;
};

// =======================
// Online Node struct
// =======================
struct OnlineNode
{
  String name;
  float rssi;
  float snr;
  unsigned long lastSeen;
};

// =======================
// LoraNode class
// ======================
class LoraNode
{
  // Public getters for webserver access
public:
  static int getOnlineCount() { return onlineCount; }
  static const OnlineNode *getOnlineNodes() { return onlineNodes; }
  static const NodeMessage *getMessages() { return messages; }
  static void setup();
  static void loop();
  static void addMessage(NodeMessage nodeMessage);
  static void loraSend(NodeMessage nodeMessage);
  static void loraSendFW(String msgID, const String &user, int TTL, const String &packet);
  static int getMsgCount();
  static int getMsgWriteIndex();
  static NodeMessage getMessage(int index);
  static String getMessageRow(int index);
  static String getNodeName();
  // Node management
  static void addOnlineNode(const String &node, float rssi, float snr);
  static void handleMessage(NodeMessage nodeMessage);
  static void handlePacket(const String &packet);
  static void cleanOfflineNodes();
  static NodeMessage nodeMessageFromString(const String &str);
  static void requestUsers();
  static void requestPages();
  static bool isUsersSynced();
  static bool isPagesSynced();
  static void setUsersSynced(bool synced);
  static void setPagesSynced(bool synced);
  static bool isUsersSyncInProgress();
  static void transmitRaw(const String &packet);
  static void loadSyncStatus();
  static void saveSyncStatus();
  static int msgWriteIndex;
  static int onlineCount;
  static bool usersSynced;
  static bool pagesSynced;
  static unsigned long lastSyncAttempt;
  static String seenMsgIds[MAX_MSGS];
  static int seenMsgIndex;

private:
  static void sendBeacon();
  static void relayBroadcast(const String &username, const String &content, int ttl);

  // LoRa module
  static Module loraModule;
  static SX1262 radio;

  // Buffers
  static NodeMessage messages[MAX_MSGS];

  static OnlineNode onlineNodes[MAX_ONLINE];

  // Identity
  static String nodeName;
  static unsigned long lastBeacon;

  // Settings
  static int beaconInterval;
  
  // Persistent storage for sync status
  static Preferences syncPrefs;
};
