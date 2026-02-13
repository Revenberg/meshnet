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
    Serial.printf("║  🚀 MeshNet V%s - STARTING UP      ║\n", FIRMWARE_VERSION);
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println("");
    
    // VERSION VERIFICATION
    Serial.println("[VERSION] Firmware Details:");
    Serial.printf("[VERSION]   Name: %s\n", FIRMWARE_NAME);
    Serial.printf("[VERSION]   Version: %s\n", FIRMWARE_VERSION);
    Serial.printf("[VERSION]   Build: %s %s\n", FIRMWARE_BUILD_DATE, FIRMWARE_BUILD_TIME);
    Serial.printf("[VERSION] ✓ MeshNet V%s confirmed\n\n", FIRMWARE_VERSION);
    
    RPI4::setup();
    User::setRuntimeCacheOnly(false);
    User::loadUsersNVS();
    LoraNode::setUsersSynced(User::getUserCount() > 0);
    NodeWebServer::setUsersSynced(User::getUserCount() > 0);
    NodeWebServer::setPagesSynced(false);

    Serial.println("[SETUP] Waiting for LoRa init before sync...");
    
    NodeWebServer::webserverSetup();
    LoraNode::setup();
    Node_display_setup();
    button.begin();

    bool hasStoredPages = NodeWebServer::getStoredPagesCount() > 0;
    LoraNode::setPagesSynced(hasStoredPages);
    NodeWebServer::setPagesSynced(hasStoredPages);

    Serial.printf("[SETUP] Stored users: %d | Stored pages: %d\n",
                  User::getUserCount(),
                  NodeWebServer::getStoredPagesCount());
    
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

    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.length() == 0) {
            return;
        }
        Serial.println("[SERIAL] Command received: " + cmd);
        if (cmd.equalsIgnoreCase("REQ;USERS") || cmd.equalsIgnoreCase("USERS")) {
            Serial.println("[SERIAL] Triggering users sync request");
            LoraNode::requestUsers();
        } else if (cmd.equalsIgnoreCase("REQ;PAGES") || cmd.equalsIgnoreCase("PAGES")) {
            Serial.println("[SERIAL] Triggering pages sync request");
            LoraNode::requestPages();
        } else if (cmd.equalsIgnoreCase("REFRESH")) {
            Serial.println("[SERIAL] Refreshing users/pages");
            LoraNode::setPagesSynced(false);
            NodeWebServer::setPagesSynced(false);
            LoraNode::requestUsers();
        } else if (cmd.equalsIgnoreCase("HARDREFRESH")) {
            Serial.println("[SERIAL] Hard refresh: clearing users/pages and reloading");
            User::clearUsers();
            User::saveUsersNVS();
            NodeWebServer::clearPages(true);
            LoraNode::setUsersSynced(false);
            NodeWebServer::setUsersSynced(false);
            LoraNode::setPagesSynced(false);
            NodeWebServer::setPagesSynced(false);
            LoraNode::requestUsers();
        } else if (cmd.equalsIgnoreCase("STATUS")) {
            Serial.printf("[SERIAL] UsersSynced=%s PagesSynced=%s StoredPages=%d Users=%d\n",
                          LoraNode::isUsersSynced() ? "true" : "false",
                          LoraNode::isPagesSynced() ? "true" : "false",
                          NodeWebServer::getStoredPagesCount(),
                          User::getUserCount());
        } else if (cmd.equalsIgnoreCase("LISTPAGES")) {
            Serial.println("[SERIAL] Stored pages:");
            for (int i = 0; i < 10; i++) {
                String name = NodeWebServer::getTeamNameAt(i);
                int len = NodeWebServer::getTeamPageLengthAt(i);
                String updated = NodeWebServer::getTeamUpdatedAtAt(i);
                if (name.length() == 0 && len == 0) {
                    continue;
                }
                Serial.printf("  [%d] team='%s' len=%d updated=%s\n", i, name.c_str(), len, updated.c_str());
            }
        } else if (cmd.equalsIgnoreCase("LISTUSERS")) {
            Serial.println("[SERIAL] Users:");
            for (int i = 0; i < User::getUserCount(); i++) {
                Serial.printf("  [%d] user='%s' team='%s'\n", i, User::getUserName(i).c_str(), User::getUserTeam(i).c_str());
            }
        } else {
            Serial.println("[SERIAL] Forwarding to LoRa handler: " + cmd);
            LoraNode::handlePacket(cmd);
        }
    }

}