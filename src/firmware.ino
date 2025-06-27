// HUB75 Pixel Art Display by https://github.com/zcsrf/HUB75-Pixel-Art-Display
// Based on mzashh project https://github.com/mzashh
// 2025

#include <LittleFS.h>

#include "panel_config.h"
#include "device_config.h"
#include "webpages.h"
#include "web_server_handles.h"
#include "panel_draw_helpers.h"

#include <Arduino.h>

#include <FS.h>

#include <AnimatedGIF.h>
#include <JPEGDEC.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <GFX_Layer.hpp>

#include <WiFi.h>
#include <WebServer.h>

#include "time.h"
#include "esp_sntp.h"

#include <nvs_flash.h>
#include <Preferences.h>

Preferences preferences;

MatrixPanel_I2S_DMA *dma_display = nullptr;

WebServer server(80);

AnimatedGIF gif;

JPEGDEC jpeg;

tm timeinfo;

SemaphoreHandle_t gfx_layer_mutex = NULL;

TaskHandle_t screenTaskHandle;
TaskHandle_t serverTaskHandle;

WiFiClient client;
WiFiServer serverTcp(TCP_PORT); // TCP server on port 12345

// Tasks Declaration
void TaskScreenDrawer(void *pvParameters);
void TaskServer(void *pvParameters);
void TaskScreenInfoLayer(void *pvParameters);

void layer_draw_callback(int16_t x, int16_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data);

// Global GFX_Layer object
GFX_Layer gfx_layer_bg(64, 64, layer_draw_callback); // background
GFX_Layer gfx_layer_fg(64, 64, layer_draw_callback); // foreground

GFX_LayerCompositor gfx_compositor(layer_draw_callback);

uint8_t *jpegBuffer;
// uint8_t jpegBuffer[BUFFER_SIZE];
bool capturing = false;
int bufferPos = 0;

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
        .imagesEnabled = true,      // We want to see images
        .loopImagesEnabled = true,  // We want our images to be on loop
        .displayBrightness = 100,   // That should be an okay value
        .imageTimeoutSeconds = 30,  // How long to display a image when not looping
        .animationEnabled = true,   // If we are showing animations
        .animationIndex = 254,      // To show a specific animation >1
        .animationTime = 1,         // How long to sleep between
    },
    .filesConfig = {
        .filesDir = "/",      // play all Files in this directory
        .maxFilesPerPage = 4, // Change this value to set the maximum number of Files per page (keep this at 4)
    },
    .status = {
        .lastStreamActivity = 0, // Active Stream flag
        .validTime = false,      // If we have or not a valid time
        .clockTime = "12:00",    // Where we store the "current time" do be displayed
        .tickTurn = false,       // To show or not to show the `:`
        .textColor = {.red = 255, .green = 255, .blue = 255},
        .scrollText = {
            .scrollText = "Hello",
            .scrollFontSize = 2, // Default font size (1 = small, 2 = normal, 3 = big, 4 = huge)
            .scrollSpeed = 18,   // Default scroll speed (1 = fastest, 150 = slowest)
            .textXPosition = 64, // Will start off screen
            .textYPosition = 24, // center of screen (half of the text height)
        },
        .fileStatus = {
            .currentFilePath = "",   // Store the current GIF file path
            .requestedFilePath = "", // Path of the GIF requested by the user
            .currentFile = File(),
            .displayFile = File(),
        },
        .fsUploadFile = File(),
    },
};

struct Config config = bootDefaults;

String scanAndConnectToStrongestNetwork()
{
  int i_strongest = -1;
  int32_t rssi_strongest = -100;
  Serial.printf("Start scanning for SSID %s\r\n", config.wifi.ssid);

  int n = WiFi.scanNetworks(); // WiFi.scanNetworks will return the number of networks found
  Serial.println("Scan done.");

  if (n == 0)
  {
    Serial.println("No networks found!");
    return ("");
  }
  else
  {
    Serial.printf("%d networks found:", n);
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.printf("%d: BSSID: %s  %2ddBm, %3d%%  %9s  %s\r\n", i, WiFi.BSSIDstr(i).c_str(), WiFi.RSSI(i), constrain(2 * (WiFi.RSSI(i) + 100), 0, 100), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted", WiFi.SSID(i).c_str());
      if ((String(config.wifi.ssid) == String(WiFi.SSID(i)) && (WiFi.RSSI(i)) > rssi_strongest))
      {
        rssi_strongest = WiFi.RSSI(i);
        i_strongest = i;
      }
    }
  }

  if (i_strongest < 0)
  {
    Serial.printf("No network with SSID %s found!\r\n", config.wifi.ssid);
    return ("");
  }
  Serial.printf("SSID match found at %d. Connecting...\r\n", i_strongest);
  WiFi.begin(config.wifi.ssid, config.wifi.password, 0, WiFi.BSSID(i_strongest));
  return (WiFi.BSSIDstr(i_strongest));
}

void noisyScreenRandomizer()
{

  for (int y = 0; y < 64; y++)
  {
    for (int x = 0; x < 64; x++)
    {
      gfx_layer_bg.drawPixel(x, y, randomRGB565());
    }
  }

  if (gfx_layer_mutex != NULL)
  {
    if (xSemaphoreTake(gfx_layer_mutex, portMAX_DELAY) == pdTRUE)
    {
      // We don't need to dim...
      // gfx_layer_bg.dim(150);
      // gfx_layer_fg.dim(255);

      // make a compositor combine that allows somewhat of a black color mix on the bg
      // gfx_compositor.Blend(gfx_layer_bg, gfx_layer_fg); // Combine the bg and the fg layer and draw it onto the panel.
      // gfx_compositor.Siloette(gfx_layer_bg, gfx_layer_fg); // Combine the bg and the fg layer and draw it onto the panel.
      gfx_compositor.Stack(gfx_layer_bg, gfx_layer_fg); // Combine the bg and the fg layer and draw it onto the panel.

      /*if ((millis() - start_tick) > 50000)
      { // we'll get bored after about 50 seconds of the same looping gif
        // break; // Will change to the next gif in the list after the set time.
      }
      */
      xSemaphoreGive(gfx_layer_mutex); // After accessing the shared resource give the mutex and allow other processes to access it
    }
  }
}

void rebootESP(String message)
{
  Serial.print("Rebooting ESP32: ");
  Serial.println(message);
  ESP.restart();
}

void setup()
{

  // Run only in case of issues with preference storage
  // nvs_flash_erase(); // erase the NVS partition and...
  // nvs_flash_init(); // initialize the NVS partition.

  preferences.begin("led-panel", false);

  // Try to load values from preferences
  // We could probably find a way to do the whole struct at once
  // But this way is readable what we want to "get"
  config.display.clockEnabled = preferences.getBool("clock", bootDefaults.display.clockEnabled);
  config.display.scrollTextEnabled = preferences.getBool("scrollText", bootDefaults.display.scrollTextEnabled);
  config.display.imagesEnabled = preferences.getBool("gifState", bootDefaults.display.imagesEnabled);
  config.display.loopImagesEnabled = preferences.getBool("loopGif", bootDefaults.display.loopImagesEnabled);
  config.display.displayBrightness = preferences.getUInt("displayBrig", bootDefaults.display.displayBrightness);

  Serial.begin(115200);
  Serial.print("Firmware: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.println("Booting ...");

  Serial.println("Mounting LittleFS ...");
  if (!LittleFS.begin(true))
  {
    // if you have not used LittleFS. before on a ESP32, it will show this error.
    // after a reboot LittleFS. will be configured and will happily work.
    Serial.println("ERROR: Cannot mount LittleFS, Rebooting");
    rebootESP("ERROR: Cannot mount LittleFS, Rebooting");
  }

  Serial.print("Flash Free: ");
  Serial.println(humanReadableSize((LittleFS.totalBytes() - LittleFS.usedBytes())));
  Serial.print("Flash Used: ");
  Serial.println(humanReadableSize(LittleFS.usedBytes()));
  Serial.print("Flash Total: ");
  Serial.println(humanReadableSize(LittleFS.totalBytes()));

  Serial.println("Loading Configuration ...");

  Serial.print("\nConnecting to Wifi: ");
  WiFi.setSleep(false);
  WiFi.setHostname(HOSTNAME);

  scanAndConnectToStrongestNetwork();

  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
    Serial.print("."); // Uncomment the 3 lines if you want the ESP32 to wait until the WIFI is connected
  }

  Serial.println("\n\nNetwork Configuration:");
  Serial.println("----------------------");
  Serial.print("         SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("  Wifi Status: ");
  Serial.println(WiFi.status());
  Serial.print("Wifi Strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("          MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("           IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("       Subnet: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("      Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("        DNS 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("        DNS 2: ");
  Serial.println(WiFi.dnsIP(1));
  Serial.print("        DNS 3: ");
  Serial.println(WiFi.dnsIP(2));
  Serial.println();

  configTime(config.time.gmtOffsetSec, config.time.daylightOffsetSec, config.time.ntpServer.c_str()); // get time from NTP server
  Serial.println(ESP.getFreeHeap());
  Serial.printf("Min free heap: %d\n", esp_get_minimum_free_heap_size());

  // disableCore0WDT()

  gfx_layer_mutex = xSemaphoreCreateMutex(); // Create the mutex

  xTaskCreatePinnedToCore(
      TaskScreenDrawer, "TaskScreenDrawer" // A name just for humans
      ,
      12000 // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
      ,
      NULL // Task parameter which can modify the task behavior. This must be passed as pointer to void.
      ,
      15 // Priority
      ,
      &screenTaskHandle, // Task handle is not used here - simply pass NULL
      1);

  xTaskCreatePinnedToCore(TaskServer, "serverTask", 4096, NULL, 3, &serverTaskHandle, 0);

  xTaskCreatePinnedToCore(
      TaskScreenInfoLayer, "TaskScreenInfoLayer",
      1024,
      NULL,
      0,
      NULL,
      0);
}

void loop()
{
  // Get time, handle sync, etc
  if (!getLocalTime(&timeinfo, 15))
  {
    Serial.println("Failed to obtain time");
    config.status.validTime = false;
  }
  else
  {
    config.status.validTime = true;
    if (config.status.tickTurn)
    {
      strftime(config.status.clockTime, sizeof(config.status.clockTime), "%H:%M", &timeinfo);
      config.status.tickTurn = false;
    }
    else
    {
      strftime(config.status.clockTime, sizeof(config.status.clockTime), "%H %M", &timeinfo);
      config.status.tickTurn = true;
    }
  }

  delay(500);
}

void TaskScreenDrawer(void *pvParameters)
{
  File root;

  // Do Panel Init
  HUB75_I2S_CFG mxconfig(
      PANEL_RES_X, // module width
      PANEL_RES_Y, // module height
      PANEL_CHAIN  // Chain length
  );

  mxconfig.gpio.r1 = R1_PIN;
  mxconfig.gpio.g1 = G1_PIN;
  mxconfig.gpio.b1 = B1_PIN;
  mxconfig.gpio.r2 = R2_PIN;
  mxconfig.gpio.g2 = G2_PIN;
  mxconfig.gpio.b2 = B2_PIN;

  mxconfig.gpio.lat = LAT_PIN;
  mxconfig.gpio.oe = OE_PIN;
  mxconfig.gpio.clk = CLK_PIN;

  mxconfig.gpio.a = A_PIN;
  mxconfig.gpio.b = B_PIN;
  mxconfig.gpio.c = C_PIN;
  mxconfig.gpio.d = D_PIN;
  mxconfig.gpio.e = E_PIN;

  // Sets apropriate clock phase
  mxconfig.clkphase = CLK_PHASE;

  // Set your Panel Specific Driver
  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;

  mxconfig.min_refresh_rate = 60;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->setRotation(0); // Flip display by 90°, the value can be 0-4
  dma_display->begin();
  dma_display->setBrightness8(config.display.displayBrightness); // 0-255
  dma_display->clearScreen();

  // Draws some useful info at boot (after wifi connection)
  bootDraw();
  // Time to allow user to see the information
  vTaskDelay(pdMS_TO_TICKS(4000));

  dma_display->fillScreen(dma_display->color565(0, 0, 0));
  gfx_layer_fg.clear();
  gfx_layer_bg.clear();

  gif.begin(LITTLE_ENDIAN_PIXELS);

  Serial.print("DisplayReady");

  root = FILESYSTEM.open(config.filesConfig.filesDir);

  jpegBuffer = (uint8_t *)malloc(BUFFER_SIZE);
  if (!jpegBuffer)
  {
    Serial.println("Failed to allocate JPEG buffer");
    while (1)
    {
    }
  }

  serverTcp.begin();

  unsigned int temp = uxTaskGetStackHighWaterMark(nullptr);
  printf("high stack water mark is %u\n", temp);

  while (!root)
  {
    Serial.print("Can't get root folder");
    vTaskDelay(pdMS_TO_TICKS(1000));
    root = FILESYSTEM.open(config.filesConfig.filesDir);
  }

  // Handle user-requested GIF
  if (!config.status.fileStatus.requestedFilePath.isEmpty())
  {
    config.status.fileStatus.displayFile = findImageByPath(root, config.status.fileStatus.requestedFilePath);
    config.status.fileStatus.currentFilePath = config.status.fileStatus.requestedFilePath;
    config.status.fileStatus.requestedFilePath = "";
  }
  // Resume last played
  else if (!config.status.fileStatus.displayFile && !config.status.fileStatus.currentFilePath.isEmpty())
  {
    config.status.fileStatus.displayFile = findImageByPath(root, config.status.fileStatus.currentFilePath);
  }
  // Fallback: play next available
  else if (!config.status.fileStatus.displayFile)
  {
    config.status.fileStatus.displayFile = root.openNextFile();

    while (config.status.fileStatus.displayFile)
    {
      if (!config.status.fileStatus.displayFile.isDirectory())
      {
        String name = config.status.fileStatus.displayFile.name();
        name.toLowerCase();

        if (name.endsWith(".gif") || name.endsWith(".jpg") || name.endsWith(".mjpeg"))
        {
          break;
        }
      }
      config.status.fileStatus.displayFile = root.openNextFile();
    }
  }

  for (;;)
  {
    // noisyScreenRandomizer();
    checkForTcpClient();

    if (client && client.available())
    {
      while (client.available())
      {
        config.status.lastStreamActivity = millis();

        uint8_t b = client.read();

        // JPEG start marker: 0xFFD8
        if (!capturing && b == 0xFF && client.peek() == 0xD8)
        {
          capturing = true;
          bufferPos = 0;
          jpegBuffer[bufferPos++] = b;
          jpegBuffer[bufferPos++] = client.read(); // consume 0xD8
          continue;
        }

        if (capturing)
        {
          if (bufferPos < BUFFER_SIZE)
            jpegBuffer[bufferPos++] = b;

          // JPEG end marker: 0xFFD9
          if (b == 0xD9 && jpegBuffer[bufferPos - 2] == 0xFF)
          {
            capturing = false;

            // Decode JPEG
            jpeg.openRAM(jpegBuffer, bufferPos, jpegFastDrawCallback);
            jpeg.setMaxOutputSize(1);
            jpeg.decode(0, 0, 0);
            jpeg.close();
            bufferPos = 0;
          }
        }
        // Do not... do not add any delay
        // vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
    else
    {
      if ((millis() - config.status.lastStreamActivity) > WAIT_STREAM_INACTIVITY)
      {
        bufferPos = 0;

        if (config.display.imagesEnabled)
        {
          if (config.status.fileStatus.displayFile)
          {
            if (!config.status.fileStatus.displayFile.isDirectory())
            {
              config.status.fileStatus.currentFilePath = String(config.status.fileStatus.displayFile.path());
              showLocalFile(config.status.fileStatus.displayFile);
            }
          }
          else
          {
            // No gif file :'(
          }

          if (!config.display.loopImagesEnabled)
          {
            // Go get the next file
            config.status.fileStatus.displayFile = root.openNextFile();

            if (!config.status.fileStatus.displayFile)
            {
              root.close();
              root = FILESYSTEM.open(config.filesConfig.filesDir);
              config.status.fileStatus.displayFile = root.openNextFile();
            }
          }
          else
          {
            // we are looping...
          }
        }
        else if (config.display.animationEnabled)
        {
          switch (config.display.animationIndex)
          {
          case 0:
            config.display.animationIndex = rand() % 18;
            break;
          case 1:
            randomDotAnimation();
            break;
          case 2:
            randomVerticalLineAnimation();
            break;
          case 3:
            randomHorizontalLineAnimation();
            break;
          case 4:
            randomSquaresAnimation();
            break;
          case 5:
            colorChangeSquaresAnimation();
            break;
          case 6:
            colorChangeCirclesAnimation();
            break;
          case 7:
            colorSegmentedCirclesAnimation();
            break;
          case 8:
            crazyEyeAnimation();
            break;
          case 9:
            juliaFractalAnimation();
            break;
          case 10:
            mandelbrotFractalAnimation();
            break;
          case 11:
            noisePortalAnimation();
            break;
          case 12:
            randomUnderDrugsAnimation(0);
            break;
          case 13:
            randomUnderDrugsAnimation(1);
            break;
          case 14:
            randomUnderDrugsAnimation(2);
            break;
          case 15:
            kaleidoscopeAnimation();
            break;
          case 16:
            colorWavesAnimation();
            break;
          case 17:
            dancingColorBlob();
            break;
          case 18:
            alienDnaSequence();
            break;

          default:
            mandelbrotFractalAnimation();
            break;
          }

          stackLayers();
          vTaskDelay(pdMS_TO_TICKS(config.display.animationTime));
        }
        else
        {
          gfx_layer_bg.clear(); // Clear the background layer if GIF playback is disabled
          stackLayers();
          vTaskDelay(pdMS_TO_TICKS(10));
        }
      }
    }
  }
}

// This tasks handles the info layer stuff
void TaskScreenInfoLayer(void *pvParameters)
{
  for (;;)
  {
    if (gfx_layer_mutex != NULL)
    {
      if (xSemaphoreTake(gfx_layer_mutex, portMAX_DELAY) == pdTRUE)
      {
        if (config.display.clockEnabled && config.status.validTime)
        {
          clockDraw();
        }
        else if (config.display.scrollTextEnabled)
        {
          drawScrollingText();
        }
        else
        {
          gfx_layer_fg.clear(); // Clear the foreground layer
        }
        xSemaphoreGive(gfx_layer_mutex);
      }
    }
    // If we require the Scrol Text to be faster, reduce the below delay
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// This tasks handles and serves all the web server stuff
void TaskServer(void *pvParameters)
{
  server.serveStatic("/", LittleFS, "/index.html");

  server.onNotFound([]()
                    { server.send(404, "text/plain", "Not found"); });

  server.on("/toggleGIF", handleToggleGif);

  server.on("/toggleClock", handleToggleClock);

  server.on("/toggleLoopGif", handleToggleLoopGif);

  server.on("/toggleScrollText", handleToggleScrollText);

  server.on("/reboot", handleSetReboot);

  server.on("/slider", handleAdjustSlider);

  server.on("/setColor", handleSetColor);

  server.on("/updateScrollText", handleSetScrollText);

  server.on("/setAnimation", handleSetAnimation);

  server.on("/upload", HTTP_POST, []()
            { server.send(200, "text/plain", "File Uploaded OK"); }, handleUpload);

  server.on("/listfiles", handleListFiles);

  server.on("/listFilesAlt", handleListFilesAlt);

  server.on("/list", handleListFiles);

  server.on("/file", HTTP_GET, handleFileRequest);

  server.on("/j/vf", handleVersionFlash);

  server.begin();

  for (;;)
  {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}