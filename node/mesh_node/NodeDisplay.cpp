/**
 * MeshNet Node Display Implementation
 */

#include "NodeDisplay.h"

DisplayFrame NodeDisplay::current_frame;
bool NodeDisplay::initialized = false;

void NodeDisplay::init() {
  Serial.println("[NodeDisplay] Initializing...");
  
  heltec_display_init();
  
  for (int i = 0; i < DISPLAY_LINES; i++) {
    current_frame.lines[i] = "";
  }
  current_frame.update_time = millis();
  current_frame.dirty = true;
  initialized = true;
  
  Serial.println("[NodeDisplay] ✓ Initialized");
}

void NodeDisplay::setLine(int line, const String &text) {
  if (line < 0 || line >= DISPLAY_LINES) return;
  
  current_frame.lines[line] = text;
  current_frame.dirty = true;
}

void NodeDisplay::clear() {
  for (int i = 0; i < DISPLAY_LINES; i++) {
    current_frame.lines[i] = "";
  }
  current_frame.dirty = true;
}

void NodeDisplay::update() {
  if (!initialized || !current_frame.dirty) return;
  
  for (int i = 0; i < DISPLAY_LINES; i++) {
    heltec_display(0, i, current_frame.lines[i].c_str());
  }
  
  current_frame.dirty = false;
  current_frame.update_time = millis();
}

void NodeDisplay::setBrightness(byte brightness) {
  // Implement brightness adjustment if supported
}

void NodeDisplay::sleep() {
  // Put display to sleep
}

void NodeDisplay::wake() {
  // Wake display
}

void NodeDisplay::showStatus(String status) {
  setLine(0, "Status:");
  setLine(1, status);
  update();
}

void NodeDisplay::showMessage(String msg) {
  setLine(0, "Message:");
  setLine(1, msg);
  update();
}

void NodeDisplay::showStats(int msgCount, int battery, int rssi) {
  String line1 = "Msgs: " + String(msgCount);
  String line2 = "Bat: " + String(battery) + "%";
  String line3 = "RSSI: " + String(rssi) + "dBm";
  
  setLine(0, line1);
  setLine(1, line2);
  setLine(2, line3);
  update();
}
