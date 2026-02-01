/**
 * MeshNet LoRa Radio Handler
 * Provides LoRa communication functions
 */

#ifndef MESHNET_LORA_H
#define MESHNET_LORA_H

#include <Arduino.h>
#include <heltec_unofficial.h>

// Message type constants
#define MSG_TYPE_DISCOVER   1
#define MSG_TYPE_STATUS     2
#define MSG_TYPE_RELAY      3
#define MSG_TYPE_CONFIG     4
#define MSG_TYPE_DATA       5

// Message structure
struct MeshMessage {
  byte type;
  String from;
  String to;
  unsigned long msgId;
  byte ttl;
  byte hops;
  String data;
  unsigned long timestamp;
};

class LoRaRadio {
public:
  static void init();
  static void transmit(const MeshMessage &msg);
  static bool receive(MeshMessage &msg);
  static void setFrequency(long freq);
  static void setPower(byte power);
  static int getRSSI();
  static int getSNR();
  
private:
  static bool initialized;
};

#endif // MESHNET_LORA_H
