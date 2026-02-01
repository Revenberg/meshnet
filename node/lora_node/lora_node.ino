#include <Arduino.h>
#include "rpi4.h"
#include <WiFi.h>
#include "LoraNode.h"
#include "User.h"
#include "NodeWebServer.h"
#include "node_display.h"
#include "NodeButton.h"

// ---------------- CONFIG ----------------

#define MAX_MSGS 50
#define MAX_ONLINE_NODES 20
#define NODE_TIMEOUT 60000    // 60s voor offline

NodeButton button(0); // Vervang 0 door het juiste pinnummer

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  🚀 MeshNet V0.9.4 - STARTING UP      ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println("");
    
    // VERSION VERIFICATION
    Serial.println("[VERSION] Firmware Details:");
    Serial.printf("[VERSION]   Name: %s\n", FIRMWARE_NAME);
    Serial.printf("[VERSION]   Version: %s\n", FIRMWARE_VERSION);
    Serial.printf("[VERSION]   Build: %s %s\n", FIRMWARE_BUILD_DATE, FIRMWARE_BUILD_TIME);
    Serial.println("[VERSION] ✓ MeshNet V0.9.4 confirmed\n");
    
    RPI4::setup();
    User::setRuntimeCacheOnly(true);
    User::clearUsers();
    NodeWebServer::setUsersSynced(false);
    NodeWebServer::setPagesSynced(false);

    Serial.println("[SETUP] Requesting users/pages from RPI container...");
    LoraNode::requestUsers();
    LoraNode::requestPages();

    Serial.printf("[SETUP] Total users available: %d\n", User::getUserCount());
    for (int i = 0; i < User::getUserCount(); i++) {
        Serial.printf("[SETUP]   [%d] %s (team=%s)\n", i, User::getUserName(i).c_str(), User::getUserTeam(i).c_str());
    }
    
    NodeWebServer::webserverSetup();
    LoraNode::setup();
    Node_display_setup();
    button.begin();
    
    Serial.println("\n[SETUP] ✓ Setup complete - ready for login");
    Serial.println("========================================\n");
}

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