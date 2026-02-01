/**
 * MeshNet Battery Manager Implementation
 */

#include "BatteryManager.h"

float BatteryManager::voltage = 4.2f;
int BatteryManager::percentage = 100;
unsigned long BatteryManager::last_update = 0;

void BatteryManager::init() {
  Serial.println("[BatteryManager] Initializing...");
  
  analogSetAttenuation(ADC_11db);
  analogSetClockDiv(1);
  
  Serial.println("[BatteryManager] ✓ Initialized");
}

void BatteryManager::update() {
  // Read battery voltage from ADC
  // Heltec board: Battery voltage is 1/2 of actual voltage due to voltage divider
  int raw = analogRead(BATTERY_PIN);
  
  // Convert ADC reading to voltage
  // ADC: 0-4095 maps to 0-3.3V
  // With voltage divider: actual_voltage = (adc_reading / 4095) * 3.3 * 2
  voltage = (raw / 4095.0) * 3.3 * 2.0 * 1.1; // 1.1 calibration factor
  
  // Constrain to realistic values
  voltage = constrain(voltage, BATTERY_MIN_VOLTAGE, BATTERY_MAX_VOLTAGE);
  
  // Convert to percentage
  float range = BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE;
  percentage = (int)((voltage - BATTERY_MIN_VOLTAGE) / range * 100.0);
  percentage = constrain(percentage, 0, 100);
  
  last_update = millis();
}

int BatteryManager::getPercentage() {
  return percentage;
}

float BatteryManager::getVoltage() {
  return voltage;
}

bool BatteryManager::isCritical() {
  return percentage <= BATTERY_CRITICAL_LEVEL;
}

bool BatteryManager::isLow() {
  return percentage <= BATTERY_LOW_LEVEL;
}

String BatteryManager::getStatus() {
  String status = String(percentage) + "% (" + String(voltage, 2) + "V)";
  
  if (isCritical()) {
    status += " CRITICAL";
  } else if (isLow()) {
    status += " LOW";
  }
  
  return status;
}
