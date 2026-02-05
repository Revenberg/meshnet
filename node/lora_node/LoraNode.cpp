#include "LoraNode.h"
#include "NodeWebServer.h"
#include <map>
#include <sstream>

// =======================
// Static vars
// =======================
Module LoraNode::loraModule(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
SX1262 LoraNode::radio(&loraModule);

NodeMessage LoraNode::messages[MAX_MSGS];
int LoraNode::msgWriteIndex = 0;

OnlineNode LoraNode::onlineNodes[MAX_ONLINE];
int LoraNode::onlineCount = 0;

String LoraNode::nodeName = "";
unsigned long LoraNode::lastBeacon = 0;
int LoraNode::beaconInterval = 30000; // 30s
bool LoraNode::usersSynced = false;
bool LoraNode::pagesSynced = false;
unsigned long LoraNode::lastSyncAttempt = 0;
String LoraNode::seenMsgIds[MAX_MSGS];
int LoraNode::seenMsgIndex = 0;
static unsigned long pagesSyncRequestMs = 0;

static const int MAX_USER_SYNC_PARTS = 10;
static String usersSyncParts[MAX_USER_SYNC_PARTS];
static int usersSyncExpectedParts = 0;
static int usersSyncReceivedParts = 0;
static unsigned long usersSyncLastPartMs = 0;
static unsigned long lastUsersResendMs = 0;
static const int MAX_PAGE_SYNC_PARTS = 30;
static String pagesSyncParts[MAX_PAGE_SYNC_PARTS];
static int pagesSyncExpectedParts = 0;
static int pagesSyncReceivedParts = 0;
static unsigned long pagesSyncLastPartMs = 0;
static unsigned long lastPagesResendMs = 0;
static unsigned long lastRespPageResendMs = 0;
static const int MAX_PAGE_TEAMS = 12;
static const int MAX_PAGE_ENTRY_PARTS = 40;
static String pageEntryTeams[MAX_PAGE_TEAMS];
static int pageEntryTotals[MAX_PAGE_TEAMS];
static int pageEntryReceived[MAX_PAGE_TEAMS];
static String pageEntryUpdatedAt[MAX_PAGE_TEAMS];
static String pageEntryChunks[MAX_PAGE_TEAMS][MAX_PAGE_ENTRY_PARTS];
static unsigned long pageEntryLastPartMs[MAX_PAGE_TEAMS];

static bool hasActivePageEntries();
static void resetPageEntrySlot(int slot);

// =======================
// Setup
// =======================
void LoraNode::setup()
{
    Serial.println("[LoRa] Initializing SX1262...");
    int state = radio.begin(868.0, 125.0, 9, 7, 0x12);

    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("[LoRa] init failed, code: %d\n", state);
        while (true)
            ;
    }

    // Set node name based on MAC
    String mac = WiFi.softAPmacAddress();
    mac.replace(":", "");
    nodeName = "LoRA_" + mac + "_" + FIRMWARE_VERSION;

    Serial.println("[LoRa] Init OK, node = " + nodeName);
    addOnlineNode(nodeName, 0, 0);

    requestUsers();
}

void LoraNode::requestUsers()
{
    usersSynced = false;
    String request = "REQ;USERS;" + nodeName;
    Serial.println("[SYNC] Requesting users: " + request);
    Serial.println(request);
    LoraNode::transmitRaw(request);
}

void LoraNode::requestPages()
{
    pagesSynced = false;
    pagesSyncRequestMs = millis();
    String request = "REQ;PAGES;" + nodeName;
    Serial.println("[SYNC] Requesting pages: " + request);
    Serial.println(request);
    LoraNode::transmitRaw(request);
}

bool LoraNode::isUsersSynced() { return usersSynced; }
bool LoraNode::isPagesSynced() { return pagesSynced; }
void LoraNode::setUsersSynced(bool synced) { usersSynced = synced; }
void LoraNode::setPagesSynced(bool synced) { pagesSynced = synced; }
bool LoraNode::isUsersSyncInProgress() { return usersSyncExpectedParts > 0; }

void LoraNode::transmitRaw(const String &packet)
{
    String packetCopy = packet;
    int state = radio.transmit(packetCopy);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println("[LoRa TX RAW] " + packet);
    }
    else
    {
        Serial.printf("[LoRa TX RAW] failed, code %d\n", state);
    }
}

// =======================
// Main loop
// =======================
void LoraNode::loop()
{
    const unsigned long nowMs = millis();
    const bool usersSyncInProgress = usersSyncExpectedParts > 0;
    const unsigned long usersSyncIntervalMs = 120000; // wait 2 minutes between users sync retries
    const unsigned long pagesSyncIntervalMs = 30000;  // pages can retry more often once users are ready
    const bool needsUsers = !usersSynced;
    const bool needsPages = usersSynced && !pagesSynced;
    const unsigned long syncIntervalMs = needsUsers ? usersSyncIntervalMs : pagesSyncIntervalMs;
    if ((needsUsers || needsPages) && !usersSyncInProgress && (nowMs - lastSyncAttempt > syncIntervalMs))
    {
        if (needsUsers)
        {
            requestUsers();
        }
        else if (needsPages)
        {
            requestPages();
        }
        lastSyncAttempt = nowMs;
    }

    if (!usersSynced && usersSyncExpectedParts > 0 && (nowMs - usersSyncLastPartMs > 30000) && (nowMs - lastUsersResendMs > 30000))
    {
        Serial.println("[USER-SYNC] Missing parts detected, re-requesting users...");
        requestUsers();
        lastUsersResendMs = nowMs;
    }

    if (!pagesSynced && pagesSyncExpectedParts > 0 && (nowMs - pagesSyncLastPartMs > 45000) && (nowMs - lastPagesResendMs > 45000))
    {
        Serial.println("[PAGE-SYNC] Missing parts detected, re-requesting pages...");
        requestPages();
        lastPagesResendMs = nowMs;
    }
    if (hasActivePageEntries() && (nowMs - lastRespPageResendMs > 45000))
    {
        bool resetAny = false;
        for (int i = 0; i < MAX_PAGE_TEAMS; i++)
        {
            if (pageEntryTeams[i].length() == 0 || pageEntryTotals[i] <= 0 || pageEntryReceived[i] >= pageEntryTotals[i])
            {
                continue;
            }
            if (nowMs - pageEntryLastPartMs[i] > 90000)
            {
                Serial.printf("[PAGE-SYNC] RESP;PAGE timeout for team: %s (reset slot %d)\n", pageEntryTeams[i].c_str(), i);
                resetPageEntrySlot(i);
                resetAny = true;
            }
        }
        if (resetAny)
        {
            Serial.println("[PAGE-SYNC] RESP;PAGE missing parts, re-requesting pages...");
            requestPages();
            lastRespPageResendMs = nowMs;
        }
    }
    // Check for incoming
    String str;
    int state = radio.receive(str);

    if (state == RADIOLIB_ERR_NONE && str.length() > 0)
    {
        Serial.println("[LoRa RX] " + str);
        handlePacket(str);
        Serial.println("LORA_RX;" + str);
    }

    // Send beacon (pause while waiting for pages sync to improve reception)
    const bool waitingForPages = (!pagesSynced && pagesSyncRequestMs > 0 && (millis() - pagesSyncRequestMs) < 30000);
    const bool waitingForUsers = (!usersSynced && usersSyncExpectedParts > 0 && (millis() - usersSyncLastPartMs) < 30000);
    if (!waitingForPages && !waitingForUsers && (millis() - lastBeacon > beaconInterval))
    {
        sendBeacon();
        lastBeacon = millis();
    }

    // Cleanup offline
    cleanOfflineNodes();
}

int LoraNode::getMsgCount() { return msgWriteIndex; }
int LoraNode::getMsgWriteIndex() { return msgWriteIndex; }
NodeMessage LoraNode::getMessage(int index) { return messages[index]; }
String LoraNode::getMessageRow(int index)
{
    NodeMessage msg = getMessage(index);
    String row = "<td>" + msg.msgId + "</td>";
    row += "<td>" + msg.user + "</td>";
    row += "<td>" + msg.object + "</td>";
    row += "<td>" + msg.function + "</td>";
    row += "<td>" + msg.parameters + "</td>";
    row += "<td>" + String(msg.TTL) + "</td>";
    return row;
}

String LoraNode::getNodeName()
{
    return nodeName;
}

static bool hasSeenMsgId(const String &msgId)
{
    for (int i = 0; i < MAX_MSGS; i++)
    {
        if (LoraNode::seenMsgIds[i] == msgId)
        {
            return true;
        }
    }
    return false;
}

static void rememberMsgId(const String &msgId)
{
    LoraNode::seenMsgIds[LoraNode::seenMsgIndex] = msgId;
    LoraNode::seenMsgIndex = (LoraNode::seenMsgIndex + 1) % MAX_MSGS;
}

// =======================
// Send message
// =======================
void LoraNode::loraSend(NodeMessage nodeMessage)
{
    String packet = String(millis()) + ";" + nodeMessage.object + ";" + nodeMessage.function + ";" + nodeMessage.parameters;
    loraSendFW(nodeMessage.msgId, nodeMessage.user, 3, packet);
}

void LoraNode::loraSendFW(String msgID, const String &user, int TTL, const String &packet)
{
    String msg = "MSG;" + msgID + ";" + user + ";" + String(TTL) + ";" + packet;
    int state = radio.transmit(msg);

    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println("[LoRa TX] " + msg);
    }
    else
    {
        Serial.printf("[LoRa TX] failed, code %d\n", state);
    }
}

// =======================
// Store message locally
// =======================
void LoraNode::addMessage(NodeMessage nodeMessage)
{
    // Log broadcast reception
    Serial.printf("[BROADCAST RX] User: %s | TTL: %d | Params: %s\n", 
        nodeMessage.user.c_str(), 
        nodeMessage.TTL, 
        nodeMessage.parameters.c_str());
    
    messages[msgWriteIndex] = nodeMessage;
    msgWriteIndex = (msgWriteIndex + 1) % MAX_MSGS;
    
    Serial.printf("[BROADCAST STORED] Index: %d | Total: %d\n", 
        (msgWriteIndex - 1 + MAX_MSGS) % MAX_MSGS, 
        msgWriteIndex);
}

// =======================
// Online nodes
// =======================
void LoraNode::addOnlineNode(const String &node, float rssi, float snr)
{
    for (int i = 0; i < LoraNode::onlineCount; i++)
    {
        if (LoraNode::onlineNodes[i].name == node)
        {
            LoraNode::onlineNodes[i].rssi = rssi;
            LoraNode::onlineNodes[i].snr = snr;
            LoraNode::onlineNodes[i].lastSeen = millis();
            return;
        }
    }
    if (LoraNode::onlineCount < MAX_ONLINE)
    {
        OnlineNode onlineNode;
        onlineNode.name = node;
        onlineNode.rssi = rssi;
        onlineNode.snr = snr;
        onlineNode.lastSeen = millis();

        LoraNode::onlineNodes[LoraNode::onlineCount++] = onlineNode;
    }
}

void LoraNode::cleanOfflineNodes()
{
    unsigned long now = millis();
    for (int i = 0; i < onlineCount; i++)
    {
        if (now - onlineNodes[i].lastSeen > 60000)
        {
            Serial.println("[LoRa] Offline: " + onlineNodes[i].name);
            for (int j = i; j < onlineCount - 1; j++)
            {
                onlineNodes[j] = onlineNodes[j + 1];
            }
            onlineCount--;
            i--;
        }
    }
}

NodeMessage LoraNode::nodeMessageFromString(const String &str)
{
    NodeMessage nodeMessage;
    int p0 = str.indexOf(';');
    int p1 = str.indexOf(';', p0 + 1);
    int p2 = str.indexOf(';', p1 + 1);
    int p3 = str.indexOf(';', p2 + 1);
    int p4 = str.indexOf(';', p3 + 1);
    int p5 = str.indexOf(';', p4 + 1);
    int p6 = str.indexOf(';', p5 + 1);
    int p7 = str.indexOf(';', p6 + 1);

    nodeMessage.msgId = str.substring(p1 + 1, p2);
    nodeMessage.user = str.substring(p2 + 1, p3);
    nodeMessage.TTL = str.substring(p3 + 1, p4).toInt();
    if (p5 == -1 || p6 == -1 || p7 == -1)
    {
        nodeMessage.timestamp = millis();
        nodeMessage.object = str.substring(p4 + 1, p5 == -1 ? str.length() : p5);
        nodeMessage.function = "";
        nodeMessage.parameters = "";
    }
    else
    {
        nodeMessage.timestamp = str.substring(p4 + 1, p5).toInt();
        nodeMessage.object = str.substring(p5 + 1, p6);
        nodeMessage.function = str.substring(p6 + 1, p7);
        nodeMessage.parameters = str.substring(p7 + 1);
    }

    return nodeMessage;
}

std::map<std::string, std::string> parseFields(const std::string& input) {
    std::map<std::string, std::string> fields;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, ',')) {
        size_t pos = item.find(':');
        if (pos != std::string::npos) {
            std::string key = item.substr(0, pos);
            std::string value = item.substr(pos + 1);
            // Remove possible spaces
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            fields[key] = value;
        }
    }
    return fields;
}

static String urlDecode(const String &input)
{
    String output;
    char temp[3] = {0};
    for (int i = 0; i < input.length(); i++)
    {
        char c = input[i];
        if (c == '+')
        {
            output += ' ';
        }
        else if (c == '%' && i + 2 < input.length())
        {
            temp[0] = input[i + 1];
            temp[1] = input[i + 2];
            char decoded = (char)strtol(temp, nullptr, 16);
            output += decoded;
            i += 2;
        }
        else
        {
            output += c;
        }
    }
    return output;
}

static bool hasActivePageEntries()
{
    for (int i = 0; i < MAX_PAGE_TEAMS; i++)
    {
        if (pageEntryTeams[i].length() > 0 && pageEntryTotals[i] > 0 && pageEntryReceived[i] < pageEntryTotals[i])
        {
            return true;
        }
    }
    return false;
}

static void resetPageEntrySlot(int slot)
{
    pageEntryTeams[slot] = "";
    pageEntryTotals[slot] = 0;
    pageEntryReceived[slot] = 0;
    pageEntryUpdatedAt[slot] = "";
    pageEntryLastPartMs[slot] = 0;
    for (int i = 0; i < MAX_PAGE_ENTRY_PARTS; i++)
    {
        pageEntryChunks[slot][i] = "";
    }
}

static void sendAckForMessage(const NodeMessage &nodeMessage)
{
    String ack = "ACK;" + nodeMessage.msgId + ";" + LoraNode::getNodeName() + ";" + nodeMessage.object + ";" + nodeMessage.function + ";" + String(millis());
    Serial.println("[ACK] Sending: " + ack);
    LoraNode::transmitRaw(ack);
    Serial.println(ack);
}

static bool isTargetedForThisNode(const NodeMessage &nodeMessage)
{
    auto fields = parseFields(std::string(nodeMessage.parameters.c_str()));
    if (fields.find("node") != fields.end())
    {
        return (String(fields["node"].c_str()) == LoraNode::getNodeName());
    }
    if (fields.find("target") != fields.end())
    {
        return (String(fields["target"].c_str()) == LoraNode::getNodeName());
    }
    if (fields.find("nodeid") != fields.end())
    {
        return (String(fields["nodeid"].c_str()) == LoraNode::getNodeName());
    }
    return false;
}

void LoraNode::handleMessage(NodeMessage nodeMessage)
{
    auto fields = parseFields(std::string(nodeMessage.parameters.c_str()));
    if (nodeMessage.object == "USER")
    {
        if ((nodeMessage.function == "ADD") || (nodeMessage.function == "UPDATE")) {
            Serial.println("[LoRa] Registering user: " + nodeMessage.parameters);
            User::registerUserWithToken(
                String(fields["name"].c_str()),
                String(fields["pwdHash"].c_str()),
                String(fields["team"].c_str()),
                String(fields["token"].c_str())
            );
        }
        // DELETE function removed, as User::removeUser does not exist
    }

    if (nodeMessage.object == "SETUP")
    {
        Serial.println("[LoRa] Setting up user: " + nodeMessage.parameters);
    }
    if (nodeMessage.object == "MSG")
    {
        Serial.println("[LoRa] Sending message: " + nodeMessage.parameters);
    }

    if (isTargetedForThisNode(nodeMessage))
    {
        sendAckForMessage(nodeMessage);
    }

}
// =======================
// Handle incoming packets
// =======================
void LoraNode::handlePacket(const String &packet)
{
    String workingPacket = packet;
    if (workingPacket.startsWith("LORA_TX;"))
    {
        workingPacket = workingPacket.substring(String("LORA_TX;").length());
    }
    int usersPartPos = workingPacket.indexOf("RESP;USERS;PART;");
    if (usersPartPos > 0)
    {
        workingPacket = workingPacket.substring(usersPartPos);
    }
    int usersPos = workingPacket.indexOf("RESP;USERS;");
    if (usersPos > 0)
    {
        workingPacket = workingPacket.substring(usersPos);
    }
    int pagesPos = workingPacket.indexOf("RESP;PAGES;");
    if (pagesPos > 0)
    {
        workingPacket = workingPacket.substring(pagesPos);
    }

    int multiPagesPartPos = workingPacket.indexOf("RESP;PAGES;PART;", 1);
    if (workingPacket.startsWith("RESP;PAGES;PART;") && multiPagesPartPos > 0)
    {
        int start = 0;
        while (true)
        {
            int idx = workingPacket.indexOf("RESP;PAGES;PART;", start);
            if (idx < 0)
            {
                break;
            }
            int next = workingPacket.indexOf("RESP;PAGES;PART;", idx + 1);
            String chunk = (next > 0) ? workingPacket.substring(idx, next) : workingPacket.substring(idx);
            handlePacket(chunk);
            if (next < 0)
            {
                return;
            }
            start = next;
        }
    }

    if (workingPacket.startsWith("RESP;USERS;PART;"))
    {
        const unsigned long nowMs = millis();
        if (nowMs - usersSyncLastPartMs > 90000)
        {
            for (int i = 0; i < MAX_USER_SYNC_PARTS; i++)
            {
                usersSyncParts[i] = "";
            }
            usersSyncExpectedParts = 0;
            usersSyncReceivedParts = 0;
        }
        usersSyncLastPartMs = nowMs;

        int s1 = workingPacket.indexOf(';');
        int s2 = workingPacket.indexOf(';', s1 + 1);
        int s3 = workingPacket.indexOf(';', s2 + 1);
        int s4 = workingPacket.indexOf(';', s3 + 1);
        int s5 = workingPacket.indexOf(';', s4 + 1);
        if (s1 == -1 || s2 == -1 || s3 == -1 || s4 == -1 || s5 == -1)
        {
            Serial.println("[USER-SYNC] Invalid PART header");
            return;
        }

        int partIndex = workingPacket.substring(s3 + 1, s4).toInt();
        int partTotal = workingPacket.substring(s4 + 1, s5).toInt();
        String payload = workingPacket.substring(s5 + 1);

        if (partTotal <= 0 || partTotal > MAX_USER_SYNC_PARTS || partIndex <= 0 || partIndex > partTotal)
        {
            Serial.println("[USER-SYNC] PART out of range");
            return;
        }

        if (usersSyncExpectedParts == 0)
        {
            usersSyncExpectedParts = partTotal;
        }
        else if (usersSyncExpectedParts != partTotal)
        {
            Serial.println("[USER-SYNC] PART total changed, resetting cache");
            for (int i = 0; i < MAX_USER_SYNC_PARTS; i++)
            {
                usersSyncParts[i] = "";
            }
            usersSyncExpectedParts = partTotal;
            usersSyncReceivedParts = 0;
        }

        if (usersSyncParts[partIndex - 1].length() == 0)
        {
            usersSyncParts[partIndex - 1] = payload;
            usersSyncReceivedParts++;
        }

        lastUsersResendMs = nowMs;

        Serial.printf("[USER-SYNC] PART %d/%d received (len=%d)\n", partIndex, partTotal, payload.length());

        if (usersSyncReceivedParts >= usersSyncExpectedParts)
        {
            String combined;
            for (int i = 0; i < usersSyncExpectedParts; i++)
            {
                if (usersSyncParts[i].length() == 0)
                {
                    continue;
                }
                if (combined.length() > 0 && !combined.endsWith(";") && !usersSyncParts[i].startsWith(";"))
                {
                    combined += ';';
                }
                combined += usersSyncParts[i];
                usersSyncParts[i] = "";
            }

            usersSyncExpectedParts = 0;
            usersSyncReceivedParts = 0;
                    Serial.println("[PAGE-SYNC] Resetting PART cache (timeout)");

            User::setRuntimeCacheOnly(false);
            bool ok = User::setUsersFromSyncPayload(combined);
            LoraNode::setUsersSynced(ok);
            NodeWebServer::setUsersSynced(ok);
            if (ok && !LoraNode::isPagesSynced())
            {
                requestPages();
                lastSyncAttempt = millis();
            }
        }
        return;
    }

    if (workingPacket.startsWith("RESP;USERS;"))
    {
        String payload = workingPacket.substring(String("RESP;USERS;").length());
        User::setRuntimeCacheOnly(false);
        bool ok = User::setUsersFromSyncPayload(payload);
        LoraNode::setUsersSynced(ok);
        NodeWebServer::setUsersSynced(ok);
        if (ok && !LoraNode::isPagesSynced())
        {
            requestPages();
            lastSyncAttempt = millis();
        }
        return;
    }

    if (workingPacket.startsWith("RESP;PAGES;PART;"))
    {
        Serial.println("[PAGE-SYNC] Receiving multipart pages payload");
        const unsigned long nowMs = millis();
        if (nowMs - pagesSyncLastPartMs > 120000)
        {
            for (int i = 0; i < MAX_PAGE_SYNC_PARTS; i++)
            {
                pagesSyncParts[i] = "";
            }
            pagesSyncExpectedParts = 0;
            pagesSyncReceivedParts = 0;
        }
        pagesSyncLastPartMs = nowMs;

        int s1 = workingPacket.indexOf(';');
        int s2 = workingPacket.indexOf(';', s1 + 1);
        int s3 = workingPacket.indexOf(';', s2 + 1);
        int s4 = workingPacket.indexOf(';', s3 + 1);
        int s5 = workingPacket.indexOf(';', s4 + 1);
        if (s1 == -1 || s2 == -1 || s3 == -1 || s4 == -1 || s5 == -1)
        {
            Serial.println("[PAGE-SYNC] Invalid PART header");
            return;
        }

        int partIndex = workingPacket.substring(s3 + 1, s4).toInt();
        int partTotal = workingPacket.substring(s4 + 1, s5).toInt();
        String payload = workingPacket.substring(s5 + 1);

        if (partTotal <= 0 || partTotal > MAX_PAGE_SYNC_PARTS || partIndex <= 0 || partIndex > partTotal)
        {
            Serial.println("[PAGE-SYNC] PART out of range");
            return;
        }

        if (pagesSyncExpectedParts == 0)
        {
            pagesSyncExpectedParts = partTotal;
        }

        if (pagesSyncParts[partIndex - 1].length() == 0)
        {
            pagesSyncParts[partIndex - 1] = payload;
            pagesSyncReceivedParts++;
        }

        Serial.printf("[PAGE-SYNC] PART %d/%d received (len=%d)\n", partIndex, partTotal, payload.length());
        Serial.printf("[PAGE-SYNC] Parts received: %d/%d\n", pagesSyncReceivedParts, pagesSyncExpectedParts);

        if (pagesSyncReceivedParts >= pagesSyncExpectedParts)
        {
            String combined;
            for (int i = 0; i < pagesSyncExpectedParts; i++)
            {
                if (pagesSyncParts[i].length() == 0)
                {
                    continue;
                }
                if (combined.length() > 0 && !combined.endsWith(";") && !pagesSyncParts[i].startsWith(";"))
                {
                    combined += ';';
                }
                combined += pagesSyncParts[i];
                pagesSyncParts[i] = "";
            }

            pagesSyncExpectedParts = 0;
            pagesSyncReceivedParts = 0;

            Serial.printf("[PAGE-SYNC] Combined payload length: %d\n", combined.length());
            if (combined.length() > 0)
            {
                String preview = combined.substring(0, combined.length() > 160 ? 160 : combined.length());
                Serial.println("[PAGE-SYNC] Payload preview: " + preview);
            }
            bool anyPages = false;
            int storedCount = 0;
            int start = 0;
            while (start < combined.length())
            {
                int end = combined.indexOf(';', start);
                if (end == -1) end = combined.length();
                String entry = combined.substring(start, end);
                entry.trim();
                if (entry.length() > 0)
                {
                    int p1 = entry.indexOf('|');
                    int p2 = (p1 > 0) ? entry.indexOf('|', p1 + 1) : -1;
                    if (p1 > 0)
                    {
                        String team = entry.substring(0, p1);
                        String encoded = (p2 > p1) ? entry.substring(p1 + 1, p2) : entry.substring(p1 + 1);
                        String updatedAt = (p2 > p1) ? entry.substring(p2 + 1) : "";
                        String html = urlDecode(encoded);
                        NodeWebServer::storeTeamPage(team, html, updatedAt);
                        anyPages = true;
                        storedCount++;
                        Serial.printf("[PAGE-SYNC] Stored page for team: %s updated=%s\n", team.c_str(), updatedAt.c_str());
                    }
                    else
                    {
                        Serial.println("[PAGE-SYNC] Skipped entry without team separator");
                    }
                }
                start = end + 1;
            }

            Serial.printf("[PAGE-SYNC] Stored pages total: %d\n", storedCount);
            LoraNode::setPagesSynced(anyPages);
            NodeWebServer::setPagesSynced(anyPages);
        }
        return;
    }

    if (workingPacket.startsWith("RESP;PAGE;"))
    {
        const unsigned long nowMs = millis();
        int s1 = workingPacket.indexOf(';');
        int s2 = workingPacket.indexOf(';', s1 + 1);
        int s3 = workingPacket.indexOf(';', s2 + 1);
        int s4 = workingPacket.indexOf(';', s3 + 1);
        int s5 = workingPacket.indexOf(';', s4 + 1);
        int s6 = workingPacket.indexOf(';', s5 + 1);
        if (s1 == -1 || s2 == -1 || s3 == -1 || s4 == -1 || s5 == -1 || s6 == -1)
        {
            Serial.println("[PAGE-SYNC] Invalid RESP;PAGE packet");
            return;
        }

        String teamEncoded = workingPacket.substring(s2 + 1, s3);
        int partIndex = workingPacket.substring(s3 + 1, s4).toInt();
        int partTotal = workingPacket.substring(s4 + 1, s5).toInt();
        String updatedAtEncoded = workingPacket.substring(s5 + 1, s6);
        String chunk = workingPacket.substring(s6 + 1);

        if (partTotal <= 0 || partTotal > MAX_PAGE_ENTRY_PARTS || partIndex <= 0 || partIndex > partTotal)
        {
            Serial.println("[PAGE-SYNC] RESP;PAGE part out of range");
            return;
        }

        String team = urlDecode(teamEncoded);
        String updatedAt = urlDecode(updatedAtEncoded);

        LoraNode::setPagesSynced(false);
        NodeWebServer::setPagesSynced(false);
        pagesSyncRequestMs = nowMs;

        int slot = -1;
        for (int i = 0; i < MAX_PAGE_TEAMS; i++)
        {
            if (pageEntryTeams[i] == team || pageEntryTeams[i].length() == 0)
            {
                slot = i;
                break;
            }
        }

        if (slot < 0)
        {
            Serial.println("[PAGE-SYNC] No slot available for page parts");
            return;
        }

        if (pageEntryTeams[slot].length() == 0)
        {
            pageEntryTeams[slot] = team;
            pageEntryTotals[slot] = partTotal;
            pageEntryReceived[slot] = 0;
            pageEntryUpdatedAt[slot] = updatedAt;
            for (int i = 0; i < MAX_PAGE_ENTRY_PARTS; i++)
            {
                pageEntryChunks[slot][i] = "";
            }
            pageEntryLastPartMs[slot] = nowMs;
        }
        else
        {
            if ((nowMs - pageEntryLastPartMs[slot]) > 120000)
            {
                Serial.printf("[PAGE-SYNC] RESP;PAGE slot timeout, resetting slot %d\n", slot);
                resetPageEntrySlot(slot);
                pageEntryTeams[slot] = team;
                pageEntryTotals[slot] = partTotal;
                pageEntryReceived[slot] = 0;
                pageEntryUpdatedAt[slot] = updatedAt;
            }
            else if (pageEntryTotals[slot] != partTotal || pageEntryUpdatedAt[slot] != updatedAt)
            {
                Serial.printf("[PAGE-SYNC] RESP;PAGE metadata changed, resetting slot %d\n", slot);
                resetPageEntrySlot(slot);
                pageEntryTeams[slot] = team;
                pageEntryTotals[slot] = partTotal;
                pageEntryReceived[slot] = 0;
                pageEntryUpdatedAt[slot] = updatedAt;
            }
        }

        if (pageEntryChunks[slot][partIndex - 1].length() == 0)
        {
            pageEntryChunks[slot][partIndex - 1] = chunk;
            pageEntryReceived[slot]++;
        }

        pageEntryLastPartMs[slot] = nowMs;

        Serial.printf("[PAGE-SYNC] RESP;PAGE team=%s part %d/%d (slot=%d)\n", team.c_str(), partIndex, partTotal, slot);

        if (pageEntryReceived[slot] >= pageEntryTotals[slot])
        {
            String encoded;
            for (int i = 0; i < pageEntryTotals[slot]; i++)
            {
                encoded += pageEntryChunks[slot][i];
            }
            String html = urlDecode(encoded);
            NodeWebServer::storeTeamPage(team, html, updatedAt);
            Serial.printf("[PAGE-SYNC] RESP;PAGE assembled for team: %s (len=%d)\n", team.c_str(), html.length());

            resetPageEntrySlot(slot);

            if (!hasActivePageEntries())
            {
                LoraNode::setPagesSynced(true);
                NodeWebServer::setPagesSynced(true);
            }
        }
        return;
    }

    if (workingPacket.startsWith("RESP;PAGES;"))
    {
        Serial.println("[PAGE-SYNC] Receiving single pages payload");
        String payload = workingPacket.substring(String("RESP;PAGES;").length());
        bool anyPages = false;
        int storedCount = 0;
        Serial.printf("[PAGE-SYNC] Payload length: %d\n", payload.length());
        if (payload.length() > 0)
        {
            String preview = payload.substring(0, payload.length() > 160 ? 160 : payload.length());
            Serial.println("[PAGE-SYNC] Payload preview: " + preview);
        }
        int start = 0;
        while (start < payload.length())
        {
            int end = payload.indexOf(';', start);
            if (end == -1) end = payload.length();
            String entry = payload.substring(start, end);
            entry.trim();
            if (entry.length() > 0)
            {
                int p1 = entry.indexOf('|');
                int p2 = (p1 > 0) ? entry.indexOf('|', p1 + 1) : -1;
                if (p1 > 0)
                {
                    String team = entry.substring(0, p1);
                    String encoded = (p2 > p1) ? entry.substring(p1 + 1, p2) : entry.substring(p1 + 1);
                    String updatedAt = (p2 > p1) ? entry.substring(p2 + 1) : "";
                    String html = urlDecode(encoded);
                    NodeWebServer::storeTeamPage(team, html, updatedAt);
                    anyPages = true;
                    storedCount++;
                    Serial.printf("[PAGE-SYNC] Stored page for team: %s updated=%s\n", team.c_str(), updatedAt.c_str());
                }
                else
                {
                    Serial.println("[PAGE-SYNC] Skipped entry without team separator");
                }
            }
            start = end + 1;
        }

        Serial.printf("[PAGE-SYNC] Stored pages total: %d\n", storedCount);
        LoraNode::setPagesSynced(anyPages);
        NodeWebServer::setPagesSynced(anyPages);
        return;
    }

    if (workingPacket.startsWith("PING;"))
    {
        String pong = "PONG;" + LoraNode::getNodeName() + ";" + String(millis());
        Serial.println("[PING] Replying: " + pong);
        LoraNode::transmitRaw(pong);
        Serial.println(pong);
        return;
    }

    if (workingPacket.startsWith("PONG;"))
    {
        Serial.println("[PING] Received: " + workingPacket);
        return;
    }

    if (workingPacket.startsWith("ACK;"))
    {
        Serial.println("[ACK] Received: " + workingPacket);
        return;
    }

    if (workingPacket.startsWith("BCAST;"))
    {
        int p1 = workingPacket.indexOf(';');
        int p2 = workingPacket.indexOf(';', p1 + 1);
        int p3 = workingPacket.indexOf(';', p2 + 1);
        int p4 = workingPacket.indexOf(';', p3 + 1);
        if (p1 > -1 && p2 > -1 && p3 > -1)
        {
            String msgId;
            String user;
            int ttl = 0;
            String content;

            if (p4 > -1)
            {
                msgId = workingPacket.substring(p1 + 1, p2);
                user = workingPacket.substring(p2 + 1, p3);
                ttl = workingPacket.substring(p3 + 1, p4).toInt();
                content = workingPacket.substring(p4 + 1);
            }
            else
            {
                msgId = String(millis());
                user = workingPacket.substring(p1 + 1, p2);
                ttl = workingPacket.substring(p2 + 1, p3).toInt();
                content = workingPacket.substring(p3 + 1);
            }

            NodeMessage msg;
            msg.msgId = msgId;
            msg.user = user;
            msg.TTL = ttl;
            msg.timestamp = millis();
            msg.object = "BCAST";
            msg.function = "RX";
            msg.parameters = content;
            addMessage(msg);

            if (ttl > 0 && !hasSeenMsgId(msg.msgId))
            {
                rememberMsgId(msg.msgId);
                String forward = "BCAST;" + msgId + ";" + user + ";" + String(ttl - 1) + ";" + content;
                LoraNode::transmitRaw(forward);
            }
        }
        return;
    }

    if (workingPacket.startsWith("BEACON;"))
    {
        String sender = workingPacket.substring(7);

        float rssi = radio.getRSSI();
        float snr = radio.getSNR();
        addOnlineNode(sender, rssi, snr);
        return;
    }

    if (workingPacket.startsWith("MSG;"))
    {
        NodeMessage nodeMessage = nodeMessageFromString(workingPacket);

        Serial.printf("[LoRa RX] Message received: %s\n", workingPacket.c_str());

        if (hasSeenMsgId(nodeMessage.msgId))
        {
            Serial.printf("[LoRa RX] Duplicate msgId ignored: %s\n", nodeMessage.msgId.c_str());
            return;
        }
        rememberMsgId(nodeMessage.msgId);

        handleMessage(nodeMessage);
        addMessage(nodeMessage);

        bool isTargeted = isTargetedForThisNode(nodeMessage);
        if (!isTargeted && nodeMessage.TTL > 0)
        {
            String forwardPacket = String(nodeMessage.timestamp) + ";" + nodeMessage.object + ";" + nodeMessage.function + ";" + nodeMessage.parameters;
            LoraNode::loraSendFW(nodeMessage.msgId, nodeMessage.user, nodeMessage.TTL - 1, forwardPacket);
        }
        return;
    }
}

// =======================
// Beacon broadcast
// =======================
void LoraNode::sendBeacon()
{
    String packet = "BEACON;" + nodeName;
    int state = radio.transmit(packet);

    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println("[LoRa TX] " + packet);
    }
}
// =======================
// Relay broadcast to all nodes
// =======================
void LoraNode::relayBroadcast(const String &username, const String &content, int ttl)
{
    // Format: BCAST;msgId;username;ttl;content
    String packet = "BCAST;" + String(millis()) + ";" + username + ";" + String(ttl) + ";" + content;
    
    Serial.printf("[BROADCAST RELAY] Sending to all nodes: %s | TTL: %d\n", username.c_str(), ttl);
    
    int state = radio.transmit(packet);
    
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println("[LoRa TX BCAST] Broadcast relayed successfully");
    }
    else
    {
        Serial.printf("[LoRa TX BCAST] Failed, code: %d\n", state);
    }
}