/// @file main.cpp
/// @brief Radar Tap — passive BLE scanner to observe the FX True Ballistic radar.
///
/// Dumps every BLE advertising packet (name, MAC, RSSI, adv type, manufacturer
/// data, service data, service UUIDs) over serial at 115200 baud.
///
/// Goal: find the radar's advertising identity and check whether shot data
/// rides in broadcast advertising — the FX app hints shots arrive unpaired.

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>
#include <string>

// FX True Ballistic service / characteristic UUIDs recovered from the APK.
static const NimBLEUUID kRadarService("8D53DC1D-1DB7-4CD3-868B-8A527460AA84");
static const NimBLEUUID kRadarChar("DA2E7828-FBCE-4E01-AE9E-261174997C48");

static void printHex(const std::string& data) {
  for (size_t i = 0; i < data.size(); ++i) {
    Serial.printf("%02X ", static_cast<uint8_t>(data[i]));
  }
  Serial.println();
}

class ScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* dev) override {
    std::string name = dev->getName();
    if (name.empty()) name = "(no name)";

    bool isRadar =
        name.find("FX") != std::string::npos ||
        name.find("Pocket") != std::string::npos ||
        name.find("True") != std::string::npos ||
        name.find("Chrono") != std::string::npos;

    Serial.printf("%s[ADV] %s | %s | rssi=%d | type=%d\n",
                  isRadar ? ">>> " : "",
                  name.c_str(),
                  dev->getAddress().toString().c_str(),
                  dev->getRSSI(),
                  dev->getAdvType());

    if (dev->haveManufacturerData()) {
      Serial.print("    mfr: ");
      printHex(dev->getManufacturerData());
    }
    if (dev->haveServiceData()) {
      Serial.print("    sd:  ");
      printHex(dev->getServiceData());
    }

    if (dev->haveServiceUUID()) {
      NimBLEUUID u = dev->getServiceUUID();
      Serial.printf("    svc: %s\n", u.toString().c_str());
      if (u == kRadarService || u == kRadarChar) isRadar = true;
    }

    if (isRadar) Serial.println("    <<< FX radar candidate");
  }
};

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Radar Tap — passive BLE scan ===");
  Serial.printf("Radar service: %s\n", kRadarService.toString().c_str());
  Serial.printf("Radar char:    %s\n", kRadarChar.toString().c_str());

  NimBLEDevice::init("RadarTap");
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(new ScanCallbacks(), true);
  scan->setActiveScan(true);   // request scan responses for more data
  scan->start(0, false);       // 0 = scan indefinitely
  Serial.println("Scanning — power on the radar...\n");
}

void loop() {
  delay(1000);
}
