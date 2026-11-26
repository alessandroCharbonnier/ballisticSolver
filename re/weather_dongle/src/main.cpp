/// @file main.cpp
/// @brief Weather dongle — BLE beacon broadcasting T/P/RH.
///
/// ESP32 classic + BME280 (0x76) + SH1106 OLED (0x3C).
/// Reads weather every 10 s, shows it on the OLED, and advertises it over BLE
/// (manufacturer data) so the phone AND the ballistic device both read it
/// passively. No connection, no pairing — the beacon is identified by its MAC.
///
/// Manufacturer data layout (company ID 0xFFFF, little-endian IEEE754):
///   [FF FF] + WeatherPayload
/// WeatherPayload:
///   float temp_c; float pressure_hpa; float humidity_pct;
///   uint32_t seq; uint32_t millis_ms;

#include <Arduino.h>
#include <Wire.h>
#include <NimBLEDevice.h>
#include <Adafruit_BME280.h>
#include <U8g2lib.h>
#include <cstring>

// ── Pins / addresses (same hardware as the main device, src/config.h) ─────
constexpr uint8_t  PIN_I2C_SDA      = 21;
constexpr uint8_t  PIN_I2C_SCL      = 22;
constexpr uint8_t  BME280_ADDR      = 0x76;
constexpr uint32_t I2C_FREQ_HZ      = 400000;
constexpr uint32_t SEND_INTERVAL_MS = 10000;
constexpr uint16_t COMPANY_ID       = 0xFFFF;  // "testing" company id

#pragma pack(push, 1)
struct WeatherPayload {
  float    temp_c;
  float    pressure_hpa;
  float    humidity_pct;
  uint32_t seq;
  uint32_t millis_ms;
};
#pragma pack(pop)

static Adafruit_BME280 bme;
static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
static NimBLEAdvertising* adv = nullptr;

static bool     s_bme_ok = false;
static uint32_t s_seq    = 0;
static uint32_t s_last   = 0;

static void updateAdvertisement(float t, float p, float h) {
  WeatherPayload payload;
  payload.temp_c       = t;
  payload.pressure_hpa = p;
  payload.humidity_pct = h;
  payload.seq          = s_seq++;
  payload.millis_ms    = millis();

  // Manufacturer data = 2-byte company ID + payload (little-endian).
  uint8_t mfr[2 + sizeof(WeatherPayload)];
  mfr[0] = static_cast<uint8_t>(COMPANY_ID & 0xFF);
  mfr[1] = static_cast<uint8_t>((COMPANY_ID >> 8) & 0xFF);
  memcpy(mfr + 2, &payload, sizeof(payload));

  NimBLEAdvertisementData advData;
  advData.setFlags(0x06);  // LE General Discoverable, BR/EDR not supported
  advData.setManufacturerData(
      std::string(reinterpret_cast<char*>(mfr), sizeof(mfr)));

  NimBLEAdvertisementData scanData;
  scanData.setName("BoreCoreAir");

  adv->stop();
  adv->setAdvertisementData(advData);
  adv->setScanResponseData(scanData);
  adv->start();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_FREQ_HZ);

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);

  s_bme_ok = bme.begin(BME280_ADDR, &Wire);
  if (s_bme_ok) {
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X2,    // temp oversampling
                    Adafruit_BME280::SAMPLING_X16,   // pressure oversampling
                    Adafruit_BME280::SAMPLING_X1,    // humidity oversampling
                    Adafruit_BME280::FILTER_X16,
                    Adafruit_BME280::STANDBY_MS_1000);
    Serial.println("[dongle] BME280 OK (0x76)");
  } else {
    Serial.println("[dongle] BME280 NOT FOUND (0x76)");
  }

  NimBLEDevice::init("BoreCoreAir");
  adv = NimBLEDevice::getAdvertising();
  Serial.println("[dongle] BLE beacon ready");
}

void loop() {
  uint32_t now = millis();

  if (now - s_last >= SEND_INTERVAL_MS) {
    s_last = now;

    u8g2.clearBuffer();
    u8g2.setCursor(0, 10);
    u8g2.print("BoreCore Air");

    if (s_bme_ok) {
      bme.takeForcedMeasurement();
      float t = bme.readTemperature();        // deg C
      float p = bme.readPressure() / 100.0f;  // hPa
      float h = bme.readHumidity();           // %

      u8g2.setCursor(0, 24);
      u8g2.printf("T %.1f C", (double)t);
      u8g2.setCursor(0, 38);
      u8g2.printf("P %.1f hPa", (double)p);
      u8g2.setCursor(0, 52);
      u8g2.printf("H %.1f %%", (double)h);

      updateAdvertisement(t, p, h);
      Serial.printf("[dongle] seq=%u T=%.2f P=%.1f H=%.1f advertised\n",
                    s_seq - 1, (double)t, (double)p, (double)h);
    } else {
      u8g2.setCursor(0, 30);
      u8g2.print("BME280 not found");
      u8g2.setCursor(0, 44);
      u8g2.print("check 0x76 wiring");
    }
    u8g2.sendBuffer();
  }

  delay(10);
}
