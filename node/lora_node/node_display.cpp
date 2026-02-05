#include "node_display.h"
#include "node_battery.h"
#include <Wire.h>
#include <WiFi.h>
#include "LoraNode.h"

SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64);
OLEDDisplayUi ui(&display);

#define VEXT GPIO_NUM_36

volatile int currentFrame = 0;
unsigned long lastFrameSwitch = 0;
const unsigned long frameSwitchInterval = 30000; // 30 seconden

void frame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_10);
    String mac = WiFi.softAPmacAddress();
    String nodeId = LoraNode::getNodeName();
    display->drawString(x, y, "MAC: " + mac);
    display->drawString(x, y + 12, "ID: " + nodeId);
}

void frame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_16);
    display->drawString(x, y, "Batterij " + String(Node_battery_percent()) + "%");
}

void frame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    display->setFont(ArialMT_Plain_16);
    display->drawString(x, y, "nodes: " + String(LoraNode::getOnlineCount()) );
}

void frame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
    // Get latest broadcast message
    const NodeMessage *messages = LoraNode::getMessages();
    int msgWriteIndex = LoraNode::getMsgWriteIndex();
    
    display->setFont(ArialMT_Plain_16);
    display->drawString(x, y, "📢 Broadcast");
    display->setFont(ArialMT_Plain_10);
    
    if (msgWriteIndex > 0) {
        // Display the most recent message
        // msgWriteIndex points to NEXT write position, so latest is at index-1
        int lastMsgIndex = msgWriteIndex - 1;
        if (lastMsgIndex < 0) lastMsgIndex = MAX_MSGS - 1;
        
        const NodeMessage &msg = messages[lastMsgIndex];
        
        // Show sender
        display->drawString(x, y + 16, "From: " + msg.user);
        
        // Truncate content if needed
        String content = msg.parameters;
        if (content.length() > 30) {
            content = content.substring(0, 27) + "...";
        }
        display->drawString(x, y + 28, content);
        
        // Show TTL remaining
        display->drawString(x, y + 40, "TTL: " + String(msg.TTL) + "s");
    } else {
        // No messages yet
        display->drawString(x, y + 20, "No broadcasts");
    }
}

FrameCallback frames[4] = { frame1, frame2, frame3, frame4 };

void overlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
    int bat = Node_battery_percent();
    display->setFont(ArialMT_Plain_16);
    String ssid = WiFi.softAPSSID();
    //display->drawString(0, 0, ssid);
//    display->drawString(0, 0, String(bat) + "%");
}

void heltec_display_power(bool on) {
    if (on) {
      pinMode(RST_OLED, OUTPUT);
      digitalWrite(RST_OLED, HIGH);
      delay(1);
      digitalWrite(RST_OLED, LOW);
      delay(20);
      digitalWrite(RST_OLED, HIGH);
    } else {
      display.displayOff();
    }
}

void Node_display_setup() {
    battery_init();

    pinMode(VEXT, OUTPUT);
    digitalWrite(VEXT, LOW);  // ✅ laag = OLED krijgt stroom
    delay(10);

    heltec_display_power(true);

    display.setContrast(255);
    display.flipScreenVertically();

    display.init();
    display.clear();
    display.setContrast(255);

    display.setTextAlignment(TEXT_ALIGN_LEFT);
//    display.setFont(ArialMT_Plain_16);
//    display.drawString(0, 16, "Battery: " + String(Node_battery_percent()) + "%");

    ui.setTargetFPS(30);
    ui.setFrames(frames, 4);
    ui.setOverlays(new OverlayCallback[1]{ overlay }, 1);
    ui.init();
}

void Node_display_update() {
    int remainingTimeBudget = ui.update();

    // Iedere 30 seconden naar volgende frame
    unsigned long now = millis();
    if (now - lastFrameSwitch > frameSwitchInterval) {
        currentFrame = (currentFrame + 1) % 4; // aantal frames
        ui.switchToFrame(currentFrame);
        lastFrameSwitch = now;
    }
}
