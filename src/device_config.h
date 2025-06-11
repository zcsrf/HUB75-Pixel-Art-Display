#ifndef DEVICE_CONFIG_h
#define DEVICE_CONFIG_h

#include <Arduino.h>

struct WifiConfig
{
    String ssid;
    String password;
};

struct TimeConfig
{
    String ntpServer;
    long gmtOffsetSec;
    int daylightOffsetSec;
};

struct DisplayConfig
{
    bool clockEnabled;      // should be saved to preferences
    bool scrollTextEnabled; // should be saved to preferences
    bool gifEnabled;        // should be saved to preferences
    bool loopGifEnabled;    // should be saved to preferences
    uint8_t displayBrightness;  // should be saved to preferences
};

struct Config
{
    struct WifiConfig wifi;
    struct TimeConfig time;
    struct DisplayConfig display;
};

struct Config bootDefaults = {
    .wifi = {
        .ssid = "Rede-IOT",        // Your WiFi SSID
        .password = "madalenaIOT", // Your WiFi password
    },
    .time = {
        .ntpServer = "10.200.0.1", // our local time server
        .gmtOffsetSec = 0,         // Lisbon is at 0
        .daylightOffsetSec = 3600, // Summer timezone
    },
    .display = {
        .clockEnabled = false,      // we don't want the clock showing
        .scrollTextEnabled = false, // we also don't want the text showing
        .gifEnabled = true, // We want to see gifs
        .loopGifEnabled = true, // We want our gifs to be on loop
        .displayBrightness = 100, // That should be an okay value
    },
    };

#endif