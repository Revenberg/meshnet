/**
 * MeshNet LoRa Radio Implementation
 */

#include "LoRaRadio.h"

bool LoRaRadio::initialized = false;

void LoRaRadio::init() {
  Serial.println("[LoRaRadio] Initializing...");
  
  int state = radio.begin();
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("[LoRaRadio] ✓ Initialized");
    
    // Configure LoRa parameters
    radio.setFrequency(868E6);           // 868MHz EU
    radio.setBandwidth(125000);          // 125kHz
    radio.setSpreadingFactor(9);         // SF9
    radio.setCodingRate(8);              // CR4/8
    radio.setOutputPower(17);            // 17dBm (legal EU limit)
    radio.setPreambleLength(8);
    radio.setCRC(true);                  // Enable CRC
    
    radio.startReceive();
    initialized = true;
  } else {
    Serial.print("[LoRaRadio] ✗ Init failed: ");
    Serial.println(state);
  }
}

void LoRaRadio::transmit(const MeshMessage &msg) {
  if (!initialized) return;
  
  // Build binary message
  String binary = "";
  binary += char(msg.type);
  binary += msg.from;
  binary += msg.to;
  binary += char((msg.msgId >> 24) & 0xFF);
  binary += char((msg.msgId >> 16) & 0xFF);
  binary += char((msg.msgId >> 8) & 0xFF);
  binary += char(msg.msgId & 0xFF);
  binary += char(msg.ttl);
  binary += char(msg.hops);
  binary += char(msg.data.length());
  binary += msg.data;
  
  int state = radio.transmit(binary);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.print("[LoRaRadio] ✓ TX: ");
    Serial.println(msg.data);
  } else {
    Serial.print("[LoRaRadio] ✗ TX error: ");
    Serial.println(state);
  }
}

bool LoRaRadio::receive(MeshMessage &msg) {
  if (!initialized) return false;
  
  int state = radio.readData(256);
  
  if (state == RADIOLIB_ERR_NONE) {
    String raw = radio.readDataStr();
    
    if (raw.length() > 10) {
      msg.type = raw[0];
      msg.rssi = radio.getRSSI();
      msg.snr = radio.getSNR();
      Serial.print("[LoRaRadio] ✓ RX Type:");
      Serial.println(msg.type);
      return true;
    }
  }
  
  return false;
}

void LoRaRadio::setFrequency(long freq) {
  if (initialized) {
    radio.setFrequency(freq);
  }
}

void LoRaRadio::setPower(byte power) {
  if (initialized) {
    radio.setOutputPower(constrain(power, 2, 17));
  }
}

int LoRaRadio::getRSSI() {
  if (initialized) {
    return radio.getRSSI();
  }
  return 0;
}

int LoRaRadio::getSNR() {
  if (initialized) {
    return (int)radio.getSNR();
  }
  return 0;
}
