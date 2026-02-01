#include <Arduino.h>
#include "RPI4.h"
#include <WiFi.h>
#include "LoraNode.h"
#include "User.h"
#include "NodeWebServer.h"
#include "node_display.h"
#include "NodeButton.h"

// ============ GLOBAL CONFIG ============
#define MAX_MSGS 50
#define MAX_ONLINE_NODES 20
#define NODE_TIMEOUT 60000    // 60s for offline

NodeButton button(0); // GPIO0 for button input

// ============ SETUP ============
void setup() {
    RPI4::setup();
    User::loadUsersNVS();
    NodeWebServer::webserverSetup();
    LoraNode::setup();
    Node_display_setup();
    button.begin();
}

// ============ LOOP ============
void loop() {
    RPI4::loop();
    LoraNode::loop();
    NodeWebServer::webserverLoop();
    Node_display_update();
    button.update();

    if (button.isSingleClick()) {
        Serial.println("Single click detected!");
    }
    if (button.isDoubleClick()) {
        Serial.println("Double click detected!");
    }
    if (button.isLongPress()) {
        Serial.println("Long press detected!");
    }
}