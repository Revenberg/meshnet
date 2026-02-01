
#pragma once
#include <Arduino.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "User.h"

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
    static void setUsersSynced(bool synced);
    static void setPagesSynced(bool synced);
    static bool isUsersSynced();
    static bool isPagesSynced();
    static void storeTeamPage(const String &team, const String &html);
    static String getTeamPage(const String &team);
    static bool hasTeamPage(const String &team);
    static AsyncWebServer httpServer;
private:
    static String makePage(String session);
    static DNSServer dnsServer;
    static bool usersSynced;
    static bool pagesSynced;
    static const int MAX_TEAM_PAGES = 10;
    static String teamNames[MAX_TEAM_PAGES];
    static String teamPages[MAX_TEAM_PAGES];
};
