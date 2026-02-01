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
String LoraNode::seenMsgIds[MAX_MSGS];
int LoraNode::seenMsgIndex = 0;

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
    requestPages();
}

void LoraNode::requestUsers()
{
    usersSynced = false;
    String request = "REQ;USERS;" + nodeName;
    Serial.println("[SYNC] Requesting users: " + request);
    Serial.println(request);
}

void LoraNode::requestPages()
{
    pagesSynced = false;
    String request = "REQ;PAGES;" + nodeName;
    Serial.println("[SYNC] Requesting pages: " + request);
    Serial.println(request);
}

bool LoraNode::isUsersSynced() { return usersSynced; }
bool LoraNode::isPagesSynced() { return pagesSynced; }
void LoraNode::setUsersSynced(bool synced) { usersSynced = synced; }
void LoraNode::setPagesSynced(bool synced) { pagesSynced = synced; }

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
    // Check for incoming
    String str;
    int state = radio.receive(str);

    if (state == RADIOLIB_ERR_NONE && str.length() > 0)
    {
        Serial.println("[LoRa RX] " + str);
        handlePacket(str);
        Serial.println("LORA_RX;" + str);
    }

    // Send beacon
    if (millis() - lastBeacon > beaconInterval)
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
    if (packet.startsWith("RESP;USERS;"))
    {
        String payload = packet.substring(String("RESP;USERS;").length());
        User::setRuntimeCacheOnly(true);
        bool ok = User::setUsersFromSyncPayload(payload);
        LoraNode::setUsersSynced(ok);
        NodeWebServer::setUsersSynced(ok);
        return;
    }

    if (packet.startsWith("RESP;PAGES;"))
    {
        String payload = packet.substring(String("RESP;PAGES;").length());
        bool anyPages = false;
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
                if (p1 > 0)
                {
                    String team = entry.substring(0, p1);
                    String encoded = entry.substring(p1 + 1);
                    String html = urlDecode(encoded);
                    NodeWebServer::storeTeamPage(team, html);
                    anyPages = true;
                    Serial.printf("[PAGE-SYNC] Stored page for team: %s\n", team.c_str());
                }
            }
            start = end + 1;
        }

        LoraNode::setPagesSynced(anyPages);
        NodeWebServer::setPagesSynced(anyPages);
        return;
    }

    if (packet.startsWith("PING;"))
    {
        String pong = "PONG;" + LoraNode::getNodeName() + ";" + String(millis());
        Serial.println("[PING] Replying: " + pong);
        LoraNode::transmitRaw(pong);
        Serial.println(pong);
        return;
    }

    if (packet.startsWith("PONG;"))
    {
        Serial.println("[PING] Received: " + packet);
        return;
    }

    if (packet.startsWith("ACK;"))
    {
        Serial.println("[ACK] Received: " + packet);
        return;
    }

    if (packet.startsWith("BCAST;"))
    {
        int p1 = packet.indexOf(';');
        int p2 = packet.indexOf(';', p1 + 1);
        int p3 = packet.indexOf(';', p2 + 1);
        int p4 = packet.indexOf(';', p3 + 1);
        if (p1 > -1 && p2 > -1 && p3 > -1)
        {
            String msgId;
            String user;
            int ttl = 0;
            String content;

            if (p4 > -1)
            {
                msgId = packet.substring(p1 + 1, p2);
                user = packet.substring(p2 + 1, p3);
                ttl = packet.substring(p3 + 1, p4).toInt();
                content = packet.substring(p4 + 1);
            }
            else
            {
                msgId = String(millis());
                user = packet.substring(p1 + 1, p2);
                ttl = packet.substring(p2 + 1, p3).toInt();
                content = packet.substring(p3 + 1);
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

    if (packet.startsWith("BEACON;"))
    {
        String sender = packet.substring(7);

        float rssi = radio.getRSSI();
        float snr = radio.getSNR();
        addOnlineNode(sender, rssi, snr);
        return;
    }

    if (packet.startsWith("MSG;"))
    {
        NodeMessage nodeMessage = nodeMessageFromString(packet);

        Serial.printf("[LoRa RX] Message received: %s\n", packet.c_str());

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