#include <RadioLib.h>
#include <esp_system.h>  // voor chip ID

// Heltec V3 LoRa pin mapping
#define LORA_CS     8
#define LORA_RST    12
#define LORA_BUSY   13
#define LORA_DIO1   14

Module loraModule(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
SX1262 radio(&loraModule);

unsigned long lastTx = 0;
const unsigned long txInterval = 10000; // elke 10 seconden "PING"
String nodeName;

// Hulpfunctie om unieke node-ID te maken
String getNodeName() {
  uint64_t chipid = ESP.getEfuseMac();  // 64-bit uniek ID
  Serial.print("chipid: ");
  Serial.println(chipid);
  return String("NODE_") + String(chipid);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  nodeName = getNodeName();
  Serial.println("Start Heltec V3 LoRa SX1262...");
  Serial.println("Mijn node naam: " + nodeName);

  int state = radio.begin(
    868.0,   // frequentie MHz
    125.0,   // bandwidth kHz
    9,       // spreading factor
    7,       // coding rate
    0x12/*,    // sync word (zelfde op beide nodes!)
    22,      // output power
    8,       // preamble
    0        // gain
*/  );

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio succesvol gestart!");
  } else {
    Serial.print("Fout bij radio.begin(), code: ");
    Serial.println(state);
    while (true);
  }

  Serial.println("Luisteren naar LoRa berichten...");
}

void loop() {
  // 1️⃣ Ontvangen
  String str;
  int state = radio.receive(str);

  if (state == RADIOLIB_ERR_NONE && str.length() > 0) {
    Serial.println("[RX] Ontvangen: " + str);
    Serial.print("   RSSI: ");
    Serial.print(radio.getRSSI());
    Serial.print(" dBm  SNR: ");
    Serial.println(radio.getSNR());

    // 2️⃣ Ping/Pong logica
    if (str.startsWith("PING")) {
      Serial.println("[RX] PING ontvangen → antwoord PONG");
      radio.transmit("PONG van " + nodeName);
    } else if (str.startsWith("PONG")) {
      Serial.println("[RX] " + str + " ontvangen ✔ verbinding werkt!");
    }
  }

  // 3️⃣ Elke 10s een PING sturen
  if (millis() - lastTx > txInterval) {
    String msg = "PING van " + nodeName;
    int txState = radio.transmit(msg);
    if (txState == RADIOLIB_ERR_NONE) {
      Serial.println("[TX] Bericht verzonden: " + msg);
    } else {
      Serial.print("[TX] Fout code: ");
      Serial.println(txState);
    }
    lastTx = millis();
  }
}
