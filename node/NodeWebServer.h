
#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "User.h"

// Buffers
#define MAX_MSGS 20
#define MAX_ONLINE_NODES 10

class NodeWebServer
{
public:
    static void webserverSetup();
    static void webserverLoop();
    static void setupRoot();
    static void setupLogin();
    static void setupRegister();
    static void setupLogout();
    static void setupDebug();
    static void setupMessages();
    static void setupCaptivePortal();
    static AsyncWebServer httpServer;
private:
    static String makePage(String session);
    static DNSServer dnsServer;
};
