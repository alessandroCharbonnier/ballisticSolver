/// @file main.cpp
/// @brief Radar Connect — BLE central tap for the FX True Ballistic radar.
///
/// Connects to the radar (confirmed MAC d0:3a:c4:71:d4:ad), dumps the full
/// GATT tree, subscribes to all notify/indicate characteristics, and prints
/// every notification (shot data) as hex.

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string>

static const char* kRadarMac = "d0:3a:c4:71:d4:ad";  // confirmed radar MAC from scan

static NimBLERemoteCharacteristic* gCh1625 = nullptr;
static NimBLERemoteCharacteristic* gCh1626 = nullptr;
static NimBLERemoteCharacteristic* gCh1627 = nullptr;
static volatile bool gShotSeen = false;

static void printHex(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; ++i) Serial.printf("%02X ", data[i]);
  Serial.println();
}

static void readAndPrint(NimBLERemoteCharacteristic* ch) {
  if (!ch || !ch->canRead()) return;
  auto v = ch->readValue();
  Serial.printf("  READ %s (%d bytes): ",
                ch->getUUID().toString().c_str(), static_cast<int>(v.size()));
  printHex(reinterpret_cast<const uint8_t*>(v.data()), v.size());
}

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* c) override {
    Serial.printf("[conn] %s MTU=%d\n",
                  c->getPeerAddress().toString().c_str(), c->getMTU());
  }
  void onDisconnect(NimBLEClient* c, int reason) override {
    Serial.printf("[disc] %s reason=%d\n",
                  c->getPeerAddress().toString().c_str(), reason);
  }
};

static void onNotify(NimBLERemoteCharacteristic* ch,
                     uint8_t* data, size_t len, bool isNotify) {
  Serial.printf("[%s] ", ch->getUUID().toString().c_str());
  printHex(data, len);
  if (ch->getUUID().toString() == "00001624-88ec-688c-644b-3fa706c0bb76")
    gShotSeen = true;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Radar Connect (central) ===");
  Serial.printf("Target MAC: %s\n", kRadarMac);

  NimBLEDevice::init("RadarCentral");
  NimBLEDevice::setMTU(498);

  std::string macStr(kRadarMac);
  NimBLEAddress addr(macStr, BLE_ADDR_RANDOM);  // d0:3a:c4:... = random static address
  NimBLEClient* client = NimBLEDevice::createClient(addr);
  client->setClientCallbacks(new ClientCallbacks(), true);
  client->setConnectionParams(12, 12, 0, 400);

  if (!client->connect()) {
    Serial.println("Connect failed — is the radar on?");
    return;
  }

  delay(500);  // let the link settle after connect
  Serial.printf("MTU now: %d\n", client->getMTU());
  auto services = client->getServices(true);  // force service discovery
  Serial.printf("Services: %d\n", static_cast<int>(services.size()));
  for (auto* svc : services) {
    Serial.printf("  SVC %s\n", svc->getUUID().toString().c_str());
    for (auto* ch : svc->getCharacteristics(true)) {  // force characteristic discovery
      std::string uuid = ch->getUUID().toString();
      Serial.printf("    CHR %s", uuid.c_str());
      if (ch->canRead())              Serial.print(" READ");
      if (ch->canWrite())             Serial.print(" WRITE");
      if (ch->canWriteNoResponse())   Serial.print(" WRITE-NR");
      if (ch->canNotify())            Serial.print(" NOTIFY");
      if (ch->canIndicate())          Serial.print(" INDICATE");
      Serial.println();

      if (uuid.find("1625") != std::string::npos) gCh1625 = ch;
      if (uuid.find("1626") != std::string::npos) gCh1626 = ch;
      if (uuid.find("1627") != std::string::npos) gCh1627 = ch;

      if (ch->canNotify() || ch->canIndicate()) {
        if (ch->subscribe(true, onNotify)) {
          Serial.println("      subscribed");
        } else {
          Serial.println("      subscribe FAILED (may need pairing)");
        }
      }
    }
  }
  // One-time snapshot of all readable characteristics (firmware, model, BC)
  Serial.println("--- READ snapshot ---");
  for (auto* svc : services) {
    for (auto* ch : svc->getCharacteristics()) {
      if (ch->canRead()) readAndPrint(ch);
    }
  }
  Serial.println("Ready — fire shots (type 'r' to reconnect).\n");
}

void loop() {
  if (gShotSeen) {
    gShotSeen = false;
    Serial.println("--- post-shot reads (1625/1626/1627) ---");
    readAndPrint(gCh1625);
    readAndPrint(gCh1626);
    readAndPrint(gCh1627);
  }
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n');
    s.trim();
    if (s == "r") ESP.restart();
  }
  delay(50);
}
