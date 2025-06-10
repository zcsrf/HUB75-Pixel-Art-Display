// By mzashh https://github.com/mzashh

#define FIRMWARE_VERSION "v0.4.5b"
#define FILESYSTEM LittleFS

#include <Arduino.h>
#include <FS.h> // File System for Web Server Files
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>

#include <GFX_Layer.hpp>
#include <WebServer.h>

#include "panel_config.h"
#include "webpages.h"
#include "time.h"
#include "esp_sntp.h"

MatrixPanel_I2S_DMA *dma_display = nullptr;

uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(255, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);
uint16_t textWidth = 0; // Width of the scrolling text
uint16_t w, h;
uint8_t colorR = 255;       // Default Red value
uint8_t colorG = 255;       // Default Green value
uint8_t colorB = 255;       // Default Blue value
uint8_t scrollFontSize = 2; // Default font size (1 = small, 2 = normal, 3 = big, 4 = huge)
uint8_t scrollSpeed = 18;   // Default scroll speed (1 = fastest, 150 = slowest)
int16_t xOne, yOne;

const String default_ssid = "Rede-IOT";            // Your WiFi SSID
const String default_wifipassword = "madalenaIOT"; // Your WiFi password
const String default_httpuser = "admin";           // WebUI login id
const String default_httppassword = "admin";       // WebUI login password
const int default_webserverporthttp = 80;
const char *ntpServer = "10.200.0.1"; // NTP server
const long gmtOffset_sec = 0;         // GMT Timezone Offset in seconds (change this to your own)
const int daylightOffset_sec = 0;
const int maxGIFsPerPage = 4; // Change this value to set the maximum number of GIFs per page (keep this at 4)
int textXPosition = 64;       // Will start off screen
int textYPosition = 24;       // center of screen (half of the text height)

unsigned long lastPixelToggle = 0;  // Tracks the last time the second set of pixels was drawn
unsigned long lastScrollUpdate = 0; // Tracks the last time the text was scrolled
unsigned long isAnimationDue;
bool showFirstSet = true;       // Default state: Clock divider colon
bool clockEnabled = true;       // Default state: Clock is enabled
bool gifEnabled = true;         // Default state: GIF playback is enabled
bool scrollTextEnabled = false; // Default state: Scrolling text is disabled
bool loopGifEnabled = true;     // Default state: Loop GIF is enabled (reverese logic)

String inputMessage;
String sliderValue = "100";   // Default brightness value
String scrollText = "Hello";  // Default Text String
String currentGifPath = "";   // Store the current GIF file path
String requestedGifPath = ""; // Path of the GIF requested by the user

char clockTime[6] = "12:00";

String gifDir = "/"; // play all GIFs in this directory on the SD card
char filePath[256] = {0};
File root, gifFile;

#define HOSTNAME "led-panel"

TaskHandle_t screenTaskHandle;
TaskHandle_t serverTaskHandle;

// Tasks Declaration
void TaskScreenDrawer(void *pvParameters);
void TaskServer(void *pvParameters);

struct Config
{
  String ssid;           // wifi ssid
  String wifipassword;   // wifi password
  String httpuser;       // username to access web admin
  String httppassword;   // password to access web admin
  int webserverporthttp; // http port number for web admin
};

Config config;             // configuration
bool shouldReboot = false; // schedule a reboot

WebServer server(80);

bool validTime = false;
struct tm timeinfo;

// function defaults
String listFiles(bool ishtml = false);

AnimatedGIF gif;
File f;
int x_offset, y_offset;

void layer_draw_callback(int16_t x, int16_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data)
{

  dma_display->drawPixel(x, y, dma_display->color565(r_data, g_data, b_data));
}

// Global GFX_Layer object
GFX_Layer gfx_layer_bg(64, 64, layer_draw_callback); // background
GFX_Layer gfx_layer_fg(64, 64, layer_draw_callback); // foreground

GFX_LayerCompositor gfx_compositor(layer_draw_callback);

void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > MATRIX_WIDTH)
    iWidth = MATRIX_WIDTH;

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line

  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  if (gifEnabled) // Check if GIF playback is enabled
  {
    if (pDraw->ucHasTransparency) // if transparency used
    {
      uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
      int x, iCount;
      pEnd = s + pDraw->iWidth;
      x = 0;
      iCount = 0; // count non-transparent pixels
      while (x < pDraw->iWidth)
      {
        c = ucTransparent - 1;
        d = usTemp;
        while (c != ucTransparent && s < pEnd)
        {
          c = *s++;
          if (c == ucTransparent) // done, stop
          {
            s--; // back up to treat it like transparent
          }
          else // opaque
          {
            *d++ = usPalette[c];
            iCount++;
          }
        } // while looking for opaque pixels
        if (iCount) // any opaque pixels?
        {
          for (int xOffset = 0; xOffset < iCount; xOffset++)
          {
            gfx_layer_bg.drawPixel(x + xOffset, y, usTemp[xOffset]); // 565 Color Format
          }
          x += iCount;
          iCount = 0;
        }
        // no, look for a run of transparent pixels
        c = ucTransparent;
        while (c == ucTransparent && s < pEnd)
        {
          c = *s++;
          if (c == ucTransparent)
            iCount++;
          else
            s--;
        }
        if (iCount)
        {
          x += iCount; // skip these
          iCount = 0;
        }
      }
    }
    else // does not have transparency
    {
      s = pDraw->pPixels;
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      for (x = 0; x < pDraw->iWidth; x++)
      {
        gfx_layer_bg.drawPixel(x, y, usPalette[*s++]); // color 565
      }
    }
  }
  else
  {
    gfx_layer_bg.clear(); // Clear the background layer if GIF playback is disabled
  }
} /* GIFDraw() */

void *GIFOpenFile(const char *fname, int32_t *pSize)
{
  Serial.print("Playing gif: ");
  Serial.println(fname);
  f = FILESYSTEM.open(fname);
  if (f)
  {
    *pSize = f.size();
    return (void *)&f;
  }
  return NULL;
} /* GIFOpenFile() */

void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
    f->close();
} /* GIFCloseFile() */

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *f = static_cast<File *>(pFile->fHandle);
  // Note: If you read a file all the way to the last byte, seek() stops working
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
  if (iBytesRead <= 0)
    return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
} /* GIFReadFile() */

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  //  Serial.printf("Seek time = %d us\n", i);
  return pFile->iPos;
} /* GIFSeekFile() */

unsigned long start_tick = 0;

void ShowGIF(char *name)
{
  start_tick = millis();
  unsigned long lastTimeCheck = millis(); // Timer for getLocalTime
  if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    x_offset = (MATRIX_WIDTH - gif.getCanvasWidth()) / 2;
    if (x_offset < 0)
      x_offset = 0;
    y_offset = (MATRIX_HEIGHT - gif.getCanvasHeight()) / 2;
    if (y_offset < 0)
      y_offset = 0;
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    Serial.flush();
    while (gif.playFrame(true, NULL))
    {
      gfx_layer_bg.dim(150);
      gfx_layer_fg.dim(255);
      gfx_compositor.Blend(gfx_layer_bg, gfx_layer_fg); // Combine the bg and the fg layer and draw it onto the panel.

      if ((millis() - start_tick) > 50000)
      { // we'll get bored after about 50 seconds of the same looping gif
        // break; // Will change to the next gif in the list after the set time.
      }
    }

    gif.close();
  }

} /* ShowGIF() */

void rebootESP(String message)
{
  Serial.print("Rebooting ESP32: ");
  Serial.println(message);
  ESP.restart();
}

String listFiles(bool ishtml, int page = 1, int pageSize = maxGIFsPerPage)
{
  String returnText = "";
  int fileIndex = 0;
  int startIndex = (page - 1) * pageSize;
  int endIndex = startIndex + pageSize;

  File root = FILESYSTEM.open("/");
  File foundfile = root.openNextFile();

  if (ishtml)
  {
    returnText += "<!DOCTYPE HTML><html lang=\"en\"><head>";
    returnText += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    returnText += "<meta charset=\"UTF-8\">";
    returnText += "</head><body>";
    returnText += "<table><tr><th>Name</th><th>Size</th><th>Preview</th><th>Actions</th></tr>";
  }

  while (foundfile)
  {
    if (fileIndex >= startIndex && fileIndex < endIndex)
    {
      if (ishtml)
      {
        returnText += "<tr><td>" + String(foundfile.name()) + "</td>";
        returnText += "<td>" + humanReadableSize(foundfile.size()) + "</td>";
        returnText += "<td><img src=\"/file?name=" + String(foundfile.name()) + "&action=show\" width=\"64\"></td>";
        returnText += "<td>";
        returnText += "<button onclick=\"downloadDeleteButton('" + String(foundfile.name()) + "', 'play')\">Play</button>";
        returnText += "<button onclick=\"downloadDeleteButton('" + String(foundfile.name()) + "', 'download')\">Download</button>";
        returnText += "<button onclick=\"downloadDeleteButton('" + String(foundfile.name()) + "', 'delete')\">Delete</button>";
        returnText += "</td></tr>";
      }
    }
    fileIndex++;
    foundfile = root.openNextFile();
  }

  if (ishtml)
  {
    returnText += "</table>";
    // Add the Home button
    returnText += "<button onclick=\"window.location.href='/'\">Home</button>";
    // Add the Previous button if applicable
    if (page > 1)
    {
      returnText += "<button onclick=\"window.location.href='/list?page=" + String(page - 1) + "'\">Previous</button>";
    }
    // Add the Next button if there are more files
    if (fileIndex > endIndex)
    {
      returnText += "<button onclick=\"window.location.href='/list?page=" + String(page + 1) + "'\">Next</button>";
    }
    // GIF select page
    returnText += "<script>";
    returnText += "function downloadDeleteButton(filename, action) {";
    returnText += "    console.log(`downloadDeleteButton called with filename: ${filename}, action: ${action}`);";
    returnText += "    const url = `/file?name=${filename}&action=${action}`;";
    returnText += "    if (action === 'delete') {";
    returnText += "        fetch(url).then(response => response.text()).then(data => {";
    returnText += "            console.log(data);";
    returnText += "            alert('File deleted successfully!');";
    returnText += "            location.reload();";
    returnText += "        }).catch(error => {";
    returnText += "            console.error('Error deleting file:', error);";
    returnText += "            alert('Failed to delete file.');";
    returnText += "        });";
    returnText += "    } else if (action === 'download') {";
    returnText += "        window.open(url, '_blank');";
    returnText += "    } else if (action === 'play') {";
    returnText += "        fetch(url).then(response => response.text()).then(data => {";
    returnText += "            console.log(data);";
    returnText += "            alert('Playing file...');";
    returnText += "        }).catch(error => {";
    returnText += "            console.error('Error playing file:', error);";
    returnText += "            ";
    returnText += "        });";
    returnText += "    }";
    returnText += "}";
    returnText += "</script>";
    returnText += "</body></html>";
  }

  root.close();
  return returnText;
}

String humanReadableSize(const size_t bytes)
{
  if (bytes < 1024)
    return String(bytes) + " B";
  else if (bytes < (1024 * 1024))
    return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024))
    return String(bytes / 1024.0 / 1024.0) + " MB";
  else
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

/************************* Arduino Sketch Setup and Loop() *******************************/
void setup()
{

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
  config.ssid = default_ssid;
  config.wifipassword = default_wifipassword;
  config.httpuser = default_httpuser;
  config.httppassword = default_httppassword;
  config.webserverporthttp = default_webserverporthttp;

  Serial.print("\nConnecting to Wifi: ");
  WiFi.begin(config.ssid.c_str(), config.wifipassword.c_str());
  WiFi.setHostname(HOSTNAME);

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

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // get time from NTP server

  // disableCore0WDT()

  xTaskCreatePinnedToCore(
      TaskScreenDrawer, "TaskScreenDrawer" // A name just for humans
      ,
      8000 // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
      ,
      NULL // Task parameter which can modify the task behavior. This must be passed as pointer to void.
      ,
      0 // Priority
      ,
      &screenTaskHandle, // Task handle is not used here - simply pass NULL
      1);

  xTaskCreatePinnedToCore(TaskServer, "serverTask", 4096, NULL, 3, &serverTaskHandle, 0);
}

bool tickTurn = false;

void loop()
{
  if (shouldReboot)
  {
    rebootESP("Web Admin Initiated Reboot");
  }

  // uint32_t highWaterMark = uxTaskGetStackHighWaterMark(screenTaskHandle);
  // Serial.print("High Water Mark: ");
  // Serial.println(highWaterMark);

  // Get time, handle sync, etc
  if (!getLocalTime(&timeinfo, 15))
  {
    Serial.println("Failed to obtain time");
    validTime = false;
  }
  else
  {
    validTime = true;
    if (tickTurn)
    {
      strftime(clockTime, sizeof(clockTime), "%H:%M", &timeinfo);
      tickTurn = false;
    }
    else
    {
      strftime(clockTime, sizeof(clockTime), "%H %M", &timeinfo);
      tickTurn = true;
    }
  }

  delay(1000);
}

void TaskScreenDrawer(void *pvParameters)
{
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
  dma_display->setBrightness8(sliderValue.toInt()); // 0-255
  dma_display->clearScreen();

  dma_display->begin();

  dma_display->fillScreen(dma_display->color565(0, 0, 0));
  dma_display->setTextSize(1);
  vTaskDelay(pdMS_TO_TICKS(1000));
  dma_display->print("ID:");
  dma_display->print(FIRMWARE_VERSION);
  dma_display->setCursor(0, 16);
  dma_display->print("IP:");
  dma_display->print(WiFi.localIP());
  dma_display->setCursor(0, 38);
  dma_display->print("RSSI:");
  dma_display->println(WiFi.RSSI());
  dma_display->setCursor(0, 52);
  dma_display->print("SSID:");
  dma_display->println(WiFi.SSID());
  vTaskDelay(pdMS_TO_TICKS(1000));
  dma_display->fillScreen(dma_display->color565(0, 0, 0));
  gfx_layer_fg.clear();
  gfx_layer_bg.clear();
  gif.begin(LITTLE_ENDIAN_PIXELS);

  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.print("DisplayReady");

  for (;;)
  {

    root = FILESYSTEM.open(gifDir); // Open the root directory
    if (root)
    {
      // Check if a new GIF is requested via the play button
      if (!requestedGifPath.isEmpty())
      {
        // Play the requested GIF and set it as the current GIF
        currentGifPath = requestedGifPath; // Update the current GIF path
        requestedGifPath = "";             // Clear the requested path

        // Find and play the requested GIF
        while (gifFile = root.openNextFile())
        {
          if (String(gifFile.path()) == currentGifPath)
          {
            break;
          }

          vTaskDelay(pdMS_TO_TICKS(1));
        }

        if (gifFile)
        {
          // Play the requested GIF
          memset(filePath, 0x0, sizeof(filePath));
          strcpy(filePath, gifFile.path());
          ShowGIF(filePath);

          // If looping is enabled, continue looping the requested GIF
          if (loopGifEnabled)
          {
            continue; // Restart the loop for the same GIF
          }
        }
      }
      else if (!currentGifPath.isEmpty())
      {
        // Resume from the last GIF
        while (gifFile = root.openNextFile())
        {
          if (String(gifFile.path()) == currentGifPath)
          {
            break;
          }
          vTaskDelay(pdMS_TO_TICKS(1));
        }
      }
      else
      {
        gifFile = root.openNextFile(); // Open the first file in the directory
      }

      while (gifFile && gifEnabled)
      {
        // why we do a lot of the same stuff (like loading the gif path if it is the same)
        if (!gifFile.isDirectory())
        { // Play the file if it's not a directory
          memset(filePath, 0x0, sizeof(filePath));
          strcpy(filePath, gifFile.path());
          currentGifPath = String(filePath); // Save the current GIF path

          // Show the GIF
          ShowGIF(filePath);

          // If looping is enabled, continue playing the same GIF
          if (loopGifEnabled)
          {
            continue; // Restart the loop for the same GIF
          }
        }

        if (!loopGifEnabled)
        {
          // If looping is disabled, move to the next GIF
          gifFile.close();               // Close the current GIF file
          gifFile = root.openNextFile(); // Open the next file

          // If no more files, reset to the first file
          if (!gifFile)
          {
            root.close();                   // Close the root directory
            root = FILESYSTEM.open(gifDir); // Reopen the root directory
            gifFile = root.openNextFile();  // Start from the first file again
          }
        }
        else
        {
          break; // Exit the loop if looping is disabled
        }

        vTaskDelay(pdMS_TO_TICKS(1));
      }

      root.close(); // Close the root directory
    }
  }
}

void SendWebsite()
{
  gifEnabled = false;
  Serial.println("sending web page");
  server.send(200, "text/html", index_html);
  gifEnabled = true;
}

void toggleGif()
{

  if (server.args() == 1)
  {
    if (server.argName(0) == "state")
    {
      gifEnabled = (server.arg(0) == "on");
      server.send(200);
    }
    else
    {
      server.send(200);
    }
  }
  else
  {
    server.send(400, "text/plain", "Missing 'state' parameter");
  }
}

void toggleClock()
{
  if (server.args() == 1)
  {
    if (server.argName(0) == "state")
    {
      clockEnabled = (server.arg(0) == "on");
      server.send(200);
    }
    else
    {
      server.send(200);
    }
  }
  else
  {
    server.send(400, "text/plain", "Missing 'state' parameter");
  }
}

void toggleLoopGif()
{
  if (server.args() == 1)
  {
    if (server.argName(0) == "state")
    {
      loopGifEnabled = (server.arg(0) == "on");
      server.send(200);
    }
    else
    {
      server.send(200);
    }
  }
  else
  {
    server.send(400, "text/plain", "Missing 'state' parameter");
  }
}

void toggleScrollText()
{
  if (server.args() == 1)
  {
    if (server.hasArg("state"))
    {
      scrollTextEnabled = (server.arg(0) == "on");
      server.send(200);
    }
    else
    {
      server.send(200);
    }
  }
  else
  {
    server.send(400, "text/plain", "Missing 'state' parameter");
  }
}

void adjustSlider()
{
  if (server.args() == 1)
  {
    if (server.argName(0) == "value")
    {
      server.send(200);
      sliderValue = server.arg(0);
      dma_display->setBrightness8(sliderValue.toInt());
    }
    else
    {
      server.send(200);
    }
  }
  else
  {
    server.send(400, "text/plain", "Missing 'value' parameter");
  }
}

void setReboot()
{
  ESP.restart();
  // shouldReboot = true; // not required...
}

void setColor()
{
  if (server.args() == 3)
  {
    if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b"))
    {
      colorR = server.arg("r").toInt();
      colorG = server.arg("g").toInt();
      colorB = server.arg("b").toInt();

      server.send(200);
    }
    else
    {
      server.send(400, "text/plain", "Missing parameters");
    }
  }
  else
  {
    server.send(400, "text/plain", "Missing parameter");
  }
}

void setScrollText()
{
  if (server.args() == 3)
  {
    if (server.hasArg("text") && server.hasArg("fontSize") && server.hasArg("speed"))
    {
      scrollText = server.arg("text");
      scrollFontSize = server.arg("fontSize").toInt();
      scrollSpeed = server.arg("speed").toInt();

      server.send(200);
    }
    else
    {
      server.send(400, "text/plain", "Missing parameters");
    }
  }
  else
  {
    server.send(400, "text/plain", "Missing parameter");
  }
}

void TaskServer(void *pvParameters)
{
  server.on("/", SendWebsite);

  server.onNotFound([]()
                    { server.send(404, "text/plain", "Not found"); });

  server.on("/toggleGIF", toggleGif);

  server.on("/toggleClock", toggleClock);

  server.on("/toggleLoopGif", toggleLoopGif);

  server.on("/toggleScrollText", toggleScrollText);

  server.on("/reboot", setReboot);

  server.on("/slider", adjustSlider);

  server.on("/setColor", setColor);

  server.on("/updateScrollText", setScrollText);

  server.begin();

  for (;;)
  {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}