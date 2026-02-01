/**
 * MeshNet Configuration Manager Implementation
 */

#include "ConfigManager.h"

Preferences ConfigManager::prefs;
NodeConfig ConfigManager::config;

void ConfigManager::init() {
  Serial.println("[ConfigManager] Initializing...");
  
  prefs.begin("meshnet", false);
  load();
  
  Serial.println("[ConfigManager] ✓ Initialized");
}

bool ConfigManager::load() {
  config.nodeName = prefs.getString(CONFIG_NODE_NAME, DEFAULT_NODE_NAME);
  config.nodeId = prefs.getString(CONFIG_NODE_ID, "");
  config.version = "1.0.0";
  config.loraFreq = prefs.getLong(CONFIG_LORA_FREQ, DEFAULT_LORA_FREQ);
  config.loraPower = prefs.getUChar(CONFIG_LORA_POWER, DEFAULT_LORA_POWER);
  config.loraSF = prefs.getUChar(CONFIG_LORA_SF, DEFAULT_LORA_SF);
  config.configured = (config.nodeId.length() > 0);
  
  Serial.print("[ConfigManager] Loaded: ");
  Serial.println(config.nodeName);
  
  return true;
}

bool ConfigManager::save() {
  prefs.putString(CONFIG_NODE_NAME, config.nodeName);
  prefs.putString(CONFIG_NODE_ID, config.nodeId);
  prefs.putLong(CONFIG_LORA_FREQ, config.loraFreq);
  prefs.putUChar(CONFIG_LORA_POWER, config.loraPower);
  prefs.putUChar(CONFIG_LORA_SF, config.loraSF);
  
  Serial.print("[ConfigManager] Saved: ");
  Serial.println(config.nodeName);
  
  return true;
}

void ConfigManager::reset() {
  config.nodeName = DEFAULT_NODE_NAME;
  config.loraFreq = DEFAULT_LORA_FREQ;
  config.loraPower = DEFAULT_LORA_POWER;
  config.loraSF = DEFAULT_LORA_SF;
  config.configured = false;
  
  prefs.clear();
  save();
  
  Serial.println("[ConfigManager] Reset to defaults");
}

void ConfigManager::setNodeName(const String &name) {
  config.nodeName = name;
  prefs.putString(CONFIG_NODE_NAME, name);
}

void ConfigManager::setLoRaPower(byte power) {
  config.loraPower = constrain(power, 2, 17);
  prefs.putUChar(CONFIG_LORA_POWER, config.loraPower);
}

void ConfigManager::setLoRaSF(byte sf) {
  config.loraSF = constrain(sf, 6, 12);
  prefs.putUChar(CONFIG_LORA_SF, config.loraSF);
}

NodeConfig& ConfigManager::getConfig() {
  return config;
}
