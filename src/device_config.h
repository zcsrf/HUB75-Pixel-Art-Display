#ifndef DEVICE_CONFIG_h
#define DEVICE_CONFIG_h

#include <Arduino.h>
#include <FS.h>

#define HOSTNAME "led-panel"
#define FIRMWARE_VERSION "v1.0.0" // Firmware version hard-coded
#define FILESYSTEM LittleFS       // File system in use

#define BUFFER_SIZE 4096 // Size of buffer for MJPEG / JPEG packets
#define TCP_PORT 12345   // Port used for mjpeg tcp streaming

#define ANIMATION // Used to test specific animations under development / bypass all the other stuff

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

struct FilesConfig
{
    String filesDir;
    int maxFilesPerPage;
};

struct FileStatus
{
    String currentFilePath;
    String requestedFilePath;
    File currentFile;
    File displayFile;
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
    FileStatus fileStatus;
    File fsUploadFile;
};

struct Config
{
    struct WifiConfig wifi;
    struct TimeConfig time;
    struct DisplayConfig display;
    struct FilesConfig filesConfig;
    struct DeviceStatus status;
};


#endif