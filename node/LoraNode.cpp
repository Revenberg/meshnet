#include "LoraNode.h"
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
    nodeName = "LoRA_" + mac;

    Serial.println("[LoRa] Init OK, node = " + nodeName);
    addOnlineNode(nodeName, 0, 0);
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

// =======================
// Send message
// =======================
void LoraNode::loraSend(NodeMessage nodeMessage)
{
    loraSendFW(nodeMessage.msgId, nodeMessage.user, 3, nodeMessage.object + ";" + nodeMessage.function + ";" + nodeMessage.parameters);
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
    messages[msgWriteIndex] = nodeMessage;
    msgWriteIndex = (msgWriteIndex + 1) % MAX_MSGS;
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
    nodeMessage.timestamp = str.substring(p4 + 1, p5).toInt();
    nodeMessage.object = str.substring(p5 + 1, p6);
    nodeMessage.function = str.substring(p6 + 1, p7);
    nodeMessage.parameters = str.substring(p7 + 1);

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

}
// =======================
// Handle incoming packets
// =======================
void LoraNode::handlePacket(const String &packet)
{
    if (packet.startsWith("BEACON;"))
    {
        String sender = packet.substring(7);

        // Metadata uitlezen
        float rssi = radio.getRSSI();
        float snr = radio.getSNR();
        addOnlineNode(sender, rssi, snr);
    }
    else if (packet.startsWith("MSG;"))
    {
        NodeMessage nodeMessage = nodeMessageFromString(packet);

        Serial.printf("[LoRa RX] Message received: %s\n", packet.c_str());

        handleMessage(nodeMessage);

        addMessage(nodeMessage);

        if (nodeMessage.TTL > 0)
        {
            LoraNode::loraSendFW(nodeMessage.msgId, nodeMessage.user, nodeMessage.TTL - 1,
                nodeMessage.object + "|" + nodeMessage.function + "|" + nodeMessage.parameters);
        }
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
