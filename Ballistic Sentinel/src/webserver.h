#pragma once
/// @file webserver.h
/// @brief AsyncWebServer for configuration UI + REST API.

class WebServer_ {
public:
    /// Start the WiFi AP and web server.
    void begin();

    /// Stop WiFi AP.
    void stop();

    /// Whether the AP is currently active.
    bool isActive() const { return active_; }

    /// Toggle WiFi on/off.
    void toggle();

private:
    bool active_ = false;
};
