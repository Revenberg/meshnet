#pragma once
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>

#define SDA_OLED GPIO_NUM_17 
#define SCL_OLED GPIO_NUM_18 
#define RST_OLED GPIO_NUM_21
#define VEXT GPIO_NUM_36

extern SSD1306Wire display;
extern OLEDDisplayUi ui;
extern uint8_t loraBuf[128];
extern String lastLoRaMsg;

void Node_display_setup();
void Node_display_update();
int getBatteryPercent();

// Frame callbacks
void frame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void frame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void frame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void frame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
extern FrameCallback frames[4];

// Overlay callback
void overlay(OLEDDisplay *display, OLEDDisplayUiState* state);
