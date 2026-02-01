/**
 * MeshNet Node Configuration Manager
 * Handles NVRAM storage and configuration
 */

#ifndef MESHNET_CONFIG_H
#define MESHNET_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

// Configuration keys
#define CONFIG_NODE_NAME "nodeName"
#define CONFIG_NODE_ID "nodeId"
#define CONFIG_LORA_FREQ "loraFreq"
#define CONFIG_LORA_POWER "loraPower"
#define CONFIG_LORA_SF "loraSF"

// Default values
#define DEFAULT_NODE_NAME "MeshNode"
#define DEFAULT_LORA_FREQ 868E6
#define DEFAULT_LORA_POWER 17
#define DEFAULT_LORA_SF 9

struct NodeConfig {
  String nodeName;
  String nodeId;
  String version;
  long loraFreq;
  byte loraPower;
  byte loraSF;
  bool configured;
};

class ConfigManager {
public:
  static void init();
  static bool load();
  static bool save();
  static void reset();
  static void setNodeName(const String &name);
  static void setLoRaPower(byte power);
  static void setLoRaSF(byte sf);
  static NodeConfig& getConfig();
  
private:
  static Preferences prefs;
  static NodeConfig config;
};

#endif // MESHNET_CONFIG_H
