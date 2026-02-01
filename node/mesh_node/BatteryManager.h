/**
 * MeshNet Battery Manager
 * Battery voltage reading and state monitoring
 */

#ifndef MESHNET_BATTERY_H
#define MESHNET_BATTERY_H

#include <Arduino.h>

#define BATTERY_PIN 1
#define BATTERY_MIN_VOLTAGE 3.0f
#define BATTERY_MAX_VOLTAGE 4.2f
#define BATTERY_CRITICAL_LEVEL 10
#define BATTERY_LOW_LEVEL 25
#define BATTERY_WARNING_LEVEL 50

class BatteryManager {
public:
  static void init();
  static void update();
  static int getPercentage();
  static float getVoltage();
  static bool isCritical();
  static bool isLow();
  static String getStatus();
  
private:
  static float voltage;
  static int percentage;
  static unsigned long last_update;
};

#endif // MESHNET_BATTERY_H
