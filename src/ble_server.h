#pragma once
/// @file ble_server.h
/// @brief NimBLE peripheral (phone config + telemetry) + weather-dongle scan.
///
/// The device runs BLE for two jobs:
///   1. A passive, periodic scan that reads the weather dongle's BLE beacon.
///   2. An on-demand peripheral (advertising) for the phone app: JSON-over-GATT
///      config / telemetry / command, gated by a 4-digit app-layer PIN.
///
/// Internal state lives as file-scope statics in ble_server.cpp; this class is
/// a thin, single-instance facade.

#include <cstdint>
#include "sensors.h"

class BleServer {
public:
    /// Initialise NimBLE + the periodic weather scan. Advertising stays off
    /// until startAdvertising() (menu "BT Toggle").
    void begin();

    /// Call from loop(): weather-scan cadence + staleness fallback.
    void update();

    /// Phone-connectable advertising on/off (menu toggle). Saves battery when off.
    void startAdvertising();
    void stopAdvertising();
    bool isAdvertising() const;

    /// Publish telemetry JSON (sensor + correction snapshot) as a notification.
    void publishTelemetry(const SensorData& sd, float distance,
                          float v_corr, float h_corr, bool staged);

    /// App-layer PIN (shown on OLED).
    uint32_t pin() const;

    /// True while the weather dongle's data is fresh.
    bool weatherLive() const;

private:
    void applyManualWeather_();
};
