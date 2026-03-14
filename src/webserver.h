#pragma once
/// @file webserver.h
/// @brief AsyncWebServer for configuration UI + REST API + captive portal.

class WebServer_ {
public:
    /// Start the WiFi AP, DNS captive portal, and web server.
    void begin();

    /// Stop WiFi AP and DNS server.
    void stop();

    /// Process DNS requests.  Call from loop().
    void processDNS();

    /// Whether the AP is currently active.
    bool isActive() const { return active_; }

    /// Toggle WiFi on/off.
    void toggle();

private:
    bool active_ = false;
};
