/**
 * MeshNet Node Display Handler
 * OLED display (SSD1306) management
 */

#ifndef MESHNET_DISPLAY_H
#define MESHNET_DISPLAY_H

#include <Arduino.h>
#include <heltec_unofficial.h>

// Display dimensions
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_LINES 8

// Frame/State structure
struct DisplayFrame {
  String lines[DISPLAY_LINES];
  unsigned long update_time;
  bool dirty;
};

class NodeDisplay {
public:
  static void init();
  static void setLine(int line, const String &text);
  static void clear();
  static void update();
  static void setBrightness(byte brightness);
  static void sleep();
  static void wake();
  
  // Display modes
  static void showStatus(String status);
  static void showMessage(String msg);
  static void showStats(int msgCount, int battery, int rssi);
  
private:
  static DisplayFrame current_frame;
  static bool initialized;
};

#endif // MESHNET_DISPLAY_H
