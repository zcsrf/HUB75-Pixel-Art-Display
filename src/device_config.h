#ifndef DEVICE_CONFIG_h
#define DEVICE_CONFIG_h

#include <Arduino.h>
#include <FS.h> // File System for Web Server Files

struct Color
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct ScrollTextStatus
{
    String scrollText;
    uint8_t scrollFontSize;
    uint8_t scrollSpeed;
    int textXPosition;
    int textYPosition;
};

struct GifConfig
{
    String gifDir;
    int maxGIFsPerPage;
};

struct GifStatus
{
    String currentGifPath;
    String requestedGifPath;
    char filePath[256];
    File currentFile;
};

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
    bool clockEnabled;         // should be saved to preferences
    bool scrollTextEnabled;    // should be saved to preferences
    bool gifEnabled;           // should be saved to preferences
    bool loopGifEnabled;       // should be saved to preferences
    uint8_t displayBrightness; // should be saved to preferences
};

struct DeviceStatus
{
    bool validTime;
    char clockTime[6];
    bool tickTurn;
    Color textColor;
    ScrollTextStatus scrollText;
    GifStatus gif;
};

struct Config
{
    struct WifiConfig wifi;
    struct TimeConfig time;
    struct DisplayConfig display;
    struct GifConfig gifConfig;
    struct DeviceStatus status;
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
        .gifEnabled = true,         // We want to see gifs
        .loopGifEnabled = true,     // We want our gifs to be on loop
        .displayBrightness = 100,   // That should be an okay value
    },
    .gifConfig = {
        .gifDir = "/", // play all GIFs in this directory on the SD card
        .maxGIFsPerPage = 4, // Change this value to set the maximum number of GIFs per page (keep this at 4)
    },
    .status = {.validTime = false,   // If we have or not a valid time
               .clockTime = "12:00", // Where we store the "current time" do be displayed
               .tickTurn = false, // To show or not to show the `:`
               .textColor = {.red = 255, .green = 255, .blue = 255},
               .scrollText = {
                   .scrollText = "Hello",
                   .scrollFontSize = 2, // Default font size (1 = small, 2 = normal, 3 = big, 4 = huge)
                   .scrollSpeed = 18,   // Default scroll speed (1 = fastest, 150 = slowest)
                   .textXPosition = 64, // Will start off screen
                   .textYPosition = 24, // center of screen (half of the text height)
               },
               .gif = {
                   .currentGifPath = "",   // Store the current GIF file path
                   .requestedGifPath = "", // Path of the GIF requested by the user
                   .filePath = {0},        // Our open file path
                   .currentFile = {0},
               }},
};

#endif