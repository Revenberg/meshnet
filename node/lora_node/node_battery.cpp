#include "Node_battery.h"
#include <Arduino.h>
#include <OLEDDisplay.h>

#define VBAT_CTRL GPIO_NUM_37
#define VBAT_ADC  GPIO_NUM_1

static float last_vbat = 0.0;
static unsigned long last_update = 0;

void battery_init() {
  pinMode(VBAT_CTRL, OUTPUT);
  digitalWrite(VBAT_CTRL, LOW); // standaard uit
  analogReadResolution(12);     // 12-bit ADC (0–4095)
}

float Node_battery_voltage_cached(unsigned long interval_ms = 30000) {
    unsigned long now = millis();
    if ((now - last_update > interval_ms) || (last_vbat == 0) ) {
        last_vbat = Node_battery_voltage();
        last_update = now;
    }
    return last_vbat;
}

// Lees spanning in Volt
float Node_battery_voltage() {
  digitalWrite(VBAT_CTRL, HIGH);    // spanningsdeler aan
  delay(10);                       // even stabiliseren

  int raw = 0;
  for (int i = 0; i < 5; i++) {
      raw += analogRead(VBAT_ADC);
      delay(2);
  }
  raw /= 5;  
  digitalWrite(VBAT_CTRL, LOW);    // weer uit om stroom te sparen

  float voltage = (raw / 4095.0) * 3.3; // ADC naar spanning
  voltage *= 2.0;                       // spanningsdeler = 1/2
  return voltage;
}

// Spanning naar percentage (lineair, 0.5V = 0%, 3.7V = 100%)
int Node_battery_percent() {
  float v = Node_battery_voltage_cached();
  int percent = map(v * 100, 50, 370, 0, 100); // in centivolt
  if (percent < 0)   percent = 0;
  if (percent > 100) percent = 100;
  return percent;
}
