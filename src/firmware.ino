// By mzashh https://github.com/mzashh

#define FIRMWARE_VERSION "v0.4.5b"
#define FILESYSTEM LittleFS

#include <Arduino.h>
#include <FS.h> // File System for Web Server Files
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include <WifiUdp.h>

#include <JPEGDEC.h>

#include <GFX_Layer.hpp>
#include <WebServer.h>

#include "panel_config.h"
#include "device_config.h"
#include "webpages.h"
#include "time.h"
#include "esp_sntp.h"

#include <nvs_flash.h>

#include <Preferences.h>

#define HOSTNAME "led-panel"

Preferences preferences;
MatrixPanel_I2S_DMA *dma_display = nullptr;
WebServer server(80);

AnimatedGIF gif;

tm timeinfo;

struct Config config = bootDefaults;

#define ANIMATION

// function defaults
String listFiles(bool ishtml = false);

SemaphoreHandle_t gfx_layer_mutex = NULL;

TaskHandle_t screenTaskHandle;
TaskHandle_t serverTaskHandle;

// Tasks Declaration
void TaskScreenDrawer(void *pvParameters);
void TaskServer(void *pvParameters);
void TaskScreenInfoLayer(void *pvParameters);

void layer_draw_callback(int16_t x, int16_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data);

// Global GFX_Layer object
GFX_Layer gfx_layer_bg(64, 64, layer_draw_callback); // background
GFX_Layer gfx_layer_fg(64, 64, layer_draw_callback); // foreground

GFX_LayerCompositor gfx_compositor(layer_draw_callback);

// Initial video streaming test
const int PORT = 12345;
const int WIDTH = 64;
const int HEIGHT = 64;
const int FRAME_SIZE = WIDTH * HEIGHT * 2; // 2 bytes per pixel for RGB565
//uint8_t frameBuffer[FRAME_SIZE];

JPEGDEC jpeg;
#define BUFFER_SIZE 4096
uint8_t *jpegBuffer;
//uint8_t jpegBuffer[BUFFER_SIZE];
bool capturing = false;
int bufferPos = 0;

void layer_draw_callback(int16_t x, int16_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data)
{
  dma_display->drawPixel(x, y, dma_display->color565(r_data, g_data, b_data));
}

void layer_draw_callback_alt(int16_t x, int16_t y, uint16_t color)
{
  dma_display->drawPixel(x, y, color);
}


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

  if (config.display.gifEnabled) // Check if GIF playback is enabled
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
  config.status.gif.currentFile = FILESYSTEM.open(fname);
  if (config.status.gif.currentFile)
  {
    *pSize = config.status.gif.currentFile.size();
    return (void *)&config.status.gif.currentFile;
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
  int32_t iBytesRead = iLen;

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

// TODO: Move this up to config
unsigned long start_tick = 0;

void ShowGIF(char *name)
{
  int x_offset, y_offset; // can be local

  start_tick = millis();

  if (gif.open(name, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw))
  {
    x_offset = (MATRIX_WIDTH - gif.getCanvasWidth()) / 2;
    if (x_offset < 0)
      x_offset = 0;
    y_offset = (MATRIX_HEIGHT - gif.getCanvasHeight()) / 2;
    if (y_offset < 0)
      y_offset = 0;

    // Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    // Serial.flush();

    while (gif.playFrame(true, NULL))
    {
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
    gif.close();
  }
} /* ShowGIF() */

uint16_t randomRGB565()
{
  uint8_t r = rand() % 32; // 5 bits
  uint8_t g = rand() % 64; // 6 bits
  uint8_t b = rand() % 32; // 5 bits
  return (r << 11) | (g << 5) | b;
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

String listFiles(bool ishtml, int page = 1, int pageSize = config.gifConfig.maxGIFsPerPage)
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

void SendWebsite()
{
  bool gifEnabledTemp = config.display.gifEnabled;

  config.display.gifEnabled = false;
  Serial.println("sending web page");
  server.send(200, "text/html", index_html);
  config.display.gifEnabled = gifEnabledTemp;
}

void toggleGif()
{

  if (server.args() == 1)
  {
    if (server.argName(0) == "state")
    {
      config.display.gifEnabled = (server.arg(0) == "on");
      server.send(200);
      preferences.putBool("gifState", config.display.gifEnabled);
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
      config.display.clockEnabled = (server.arg(0) == "on");
      server.send(200);
      preferences.putBool("clock", config.display.clockEnabled);
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
      config.display.loopGifEnabled = (server.arg(0) == "on");
      server.send(200);
      preferences.putBool("loopGif", config.display.loopGifEnabled);
      Serial.println("Changing Loop Gif State");
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
      config.display.scrollTextEnabled = (server.arg(0) == "on");
      server.send(200);
      preferences.putBool("scrollText", config.display.scrollTextEnabled);
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
      config.display.displayBrightness = server.arg(0).toInt();
      server.send(200);

      dma_display->setBrightness8(config.display.displayBrightness);
      preferences.putUInt("displayBrig", config.display.displayBrightness);
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
  rebootESP("Web Request");
}

File fsUploadFile;

void handleUpload()
{
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
    {
      filename = "/" + filename;
    }
    Serial.print("handleFileUpload Name: ");
    Serial.println(filename);
    fsUploadFile = FILESYSTEM.open(filename, "w");
    filename = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    Serial.print("handleFileUpload Data: ");
    Serial.println(upload.currentSize);
    if (fsUploadFile)
    {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
    {
      fsUploadFile.close();
    }
    Serial.print("handleFileUpload Size: ");
    Serial.println(upload.totalSize);
  }
}

void handleListFiles()
{
  int page = 1;

  if (server.hasArg("page"))
  {
    page = server.arg("page").toInt();
  }

  server.send(200, "text/html", listFiles(true, page, config.gifConfig.maxGIFsPerPage));
}

void handleListFilesAlt()
{
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory())
  {
    server.send(500, "text/plain", "Failed to open /images");
    return;
  }

  File file = root.openNextFile();
  String json = "[";
  bool first = true;
  while (file)
  {
    if (!file.isDirectory())
    {
      if (!first)
        json += ",";
      json += "\"" + String("") + file.name() + "\"";
      first = false;
    }
    file = root.openNextFile();
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleVersionFlash()
{
  // This functions will provide a JSON of home page dynamic values
  String message = "{\"v\":\"";
  message += "1:2:3";
  message += "\"";

  message += ",\"ff\":\"";
  message += String(humanReadableSize((LittleFS.totalBytes() - LittleFS.usedBytes())));
  message += "\"";

  message += ",\"uf\":\"";
  message += String(humanReadableSize(LittleFS.usedBytes()));
  message += "\"";

  message += ",\"tf\":\"";
  message += String(humanReadableSize(LittleFS.totalBytes()));
  message += "\"";

  message += ",\"gs\":\"";
  message += String(config.display.gifEnabled);
  message += "\"";

  message += ",\"cs\":\"";
  message += String(config.display.clockEnabled);
  message += "\"";

  message += ",\"ls\":\"";
  message += String(config.display.loopGifEnabled);
  message += "\"";

  message += ",\"ts\":\"";
  message += String(config.display.scrollTextEnabled);
  message += "\"";

  message += ",\"ds\":\"";
  message += String(config.display.displayBrightness);
  message += "\"";

  // Text Color
  message += ",\"str\":\"";
  message += String(config.status.textColor.red);
  message += "\"";
  message += ",\"stg\":\"";
  message += String(config.status.textColor.green);
  message += "\"";
  message += ",\"stb\":\"";
  message += String(config.status.textColor.blue);
  message += "\"";

  // Scroll Text Size
  message += ",\"stf\":\"";
  message += String(config.status.scrollText.scrollFontSize);
  message += "\"";

  // Scroll Text Speed
  message += ",\"sts\":\"";
  message += String(config.status.scrollText.scrollSpeed);
  message += "\"";

  // Scroll Text
  message += ",\"stt\":\"";
  message += String(config.status.scrollText.scrollText);

  message += "\"}";

  server.send(200, "application/json", message);
}

void handleFileRequest()
{
  bool gifEnabledTemp = config.display.gifEnabled;

  config.display.gifEnabled = false;

  if (server.hasArg("name") && server.hasArg("action"))
  {
    String fileName = "/" + server.arg("name");
    String fileAction = server.arg("action");

    String logmessage = "Client: " + server.client().remoteIP().toString() + " " + server.uri() +
                        "?name=" + fileName + "&action=" + fileAction;

    if (!LittleFS.exists(fileName))
    {
      Serial.println(logmessage + " ERROR: file does not exist");
      server.send(400, "text/plain", "ERROR: file does not exist");
      return;
    }

    Serial.println(logmessage + " file exists");

    if (fileAction == "download")
    {
      logmessage += " downloaded";
      File downloadFile = LittleFS.open(fileName, "r");
      server.streamFile(downloadFile, "application/octet-stream");
      downloadFile.close();
    }
    else if (fileAction == "delete")
    {
      LittleFS.remove(fileName);
      logmessage += " deleted";
      server.send(200, "text/plain", "Deleted File: " + fileName);
    }
    else if (fileAction == "play")
    {
      config.status.gif.requestedGifPath = fileName;
      config.status.gif.gifFile = LittleFS.open(fileName, "r");
      logmessage += " opening";
      server.send(200, "text/plain", "Opened GIF: " + fileName);
    }
    else if (fileAction == "show")
    {
      logmessage += " previewing";
      delay(100); // simulate processing
      File imageFile = LittleFS.open(fileName, "r");
      server.streamFile(imageFile, "image/gif");
      imageFile.close();
    }
    else
    {
      logmessage += " ERROR: invalid action param supplied";
      server.send(400, "text/plain", "ERROR: invalid action param supplied");
    }

    Serial.println(logmessage);
  }
  else
  {
    server.send(400, "text/plain", "ERROR: name and action params required");
  }

  config.display.gifEnabled = gifEnabledTemp;
}

void setColor()
{
  if (server.args() == 3)
  {
    if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b"))
    {
      config.status.textColor.red = server.arg("r").toInt();
      config.status.textColor.green = server.arg("g").toInt();
      config.status.textColor.blue = server.arg("b").toInt();

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

  if (server.hasArg("text"))
  {
    config.status.scrollText.scrollText = server.arg("text");
  }
  if (server.hasArg("fontSize"))
  {
    config.status.scrollText.scrollFontSize = server.arg("fontSize").toInt();
  }
  if (server.hasArg("speed"))
  {
    config.status.scrollText.scrollSpeed = server.arg("speed").toInt();
  }

  if (!server.hasArg("text") && !server.hasArg("fontSize") && !server.hasArg("speed"))
  {
    server.send(400, "text/plain", "Missing parameters");
  }
  else
  {
    server.send(200);
  }
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
  config.display.gifEnabled = preferences.getBool("gifState", bootDefaults.display.gifEnabled);
  config.display.loopGifEnabled = preferences.getBool("loopGif", bootDefaults.display.loopGifEnabled);
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

  WiFi.begin(config.wifi.ssid.c_str(), config.wifi.password.c_str());
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
      0 // Priority
      ,
      &screenTaskHandle, // Task handle is not used here - simply pass NULL
      0);

  xTaskCreatePinnedToCore(TaskServer, "serverTask", 4096, NULL, 3, &serverTaskHandle, 1);

  xTaskCreate(
      TaskScreenInfoLayer, "TaskScreenInfoLayer",
      1024,
      NULL,
      1,
      NULL);
}


File findGifByPath(File root, const String &targetPath)
{
  File gifFile;
  while ((gifFile = root.openNextFile()))
  {
    if (String(gifFile.path()) == targetPath)
    {
      return gifFile;
    }
    gifFile.close();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  return File(); // Return empty file if not found
}

void playGif(File gifFile)
{
  memset(config.status.gif.filePath, 0x0, sizeof(config.status.gif.filePath));
  strcpy(config.status.gif.filePath, gifFile.path());
  config.status.gif.currentGifPath = String(config.status.gif.filePath);
  ShowGIF(config.status.gif.filePath);
}

void loop()
{
  // uint32_t highWaterMark = uxTaskGetStackHighWaterMark(screenTaskHandle);
  // Serial.print("High Water Mark: ");
  // Serial.println(highWaterMark);

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

  delay(1000);
}

int drawCallback(JPEGDRAW *pDraw)
{

  uint16_t *p = (uint16_t *)pDraw->pPixels;
  for (int y = 0; y < pDraw->iHeight; y++) {
    for (int x = 0; x < pDraw->iWidthUsed; x++) {
      int index = y * pDraw->iWidth + x;  // line-by-line
      uint16_t color = p[index];
      layer_draw_callback_alt(pDraw->x+ x,pDraw->y + y,color);
    }
  }
  return 1; // 
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

  root = FILESYSTEM.open(config.gifConfig.gifDir);

 jpegBuffer = (uint8_t *)malloc(BUFFER_SIZE);
  if (!jpegBuffer) {
    Serial.println("Failed to allocate JPEG buffer");
    while (1);
  }

  // WiFiUDP udp;

  // udp.begin(PORT);
  WiFiClient client;
  WiFiServer serverTcp(12345); // TCP server on port 12345
  serverTcp.begin();

  unsigned int temp = uxTaskGetStackHighWaterMark(nullptr);
  printf("high stack water mark is %u\n", temp);  

  while (!root)
  {
    Serial.print("Can't get root folder");
    vTaskDelay(pdMS_TO_TICKS(1000));
    root = FILESYSTEM.open(config.gifConfig.gifDir);
  }

  // Handle user-requested GIF
  if (!config.status.gif.requestedGifPath.isEmpty())
  {
    config.status.gif.gifFile = findGifByPath(root, config.status.gif.requestedGifPath);
    config.status.gif.currentGifPath = config.status.gif.requestedGifPath;
    config.status.gif.requestedGifPath = "";
  }
  // Resume last played
  else if (!config.status.gif.gifFile && !config.status.gif.currentGifPath.isEmpty())
  {
    config.status.gif.gifFile = findGifByPath(root, config.status.gif.currentGifPath);
  }
  // Fallback: play next available
  else if (!config.status.gif.gifFile)
  {
    config.status.gif.gifFile = root.openNextFile();
  }

  for (;;)
  {

#ifdef ANIMATION

    // noisyScreenRandomizer();
    Serial.flush();

    if (!client || !client.connected())
    {
      client = serverTcp.available();
      Serial.flush();
    }

   while (client && client.available()) {
    Serial.flush();
    uint8_t b = client.read();

    // JPEG start marker: 0xFFD8
    if (!capturing && b == 0xFF && client.peek() == 0xD8) {
      Serial.flush();
      capturing = true;
      bufferPos = 0;
      jpegBuffer[bufferPos++] = b;
      jpegBuffer[bufferPos++] = client.read();  // consume 0xD8
      continue;
    }

    
    if (capturing) {
      if (bufferPos < BUFFER_SIZE)
        jpegBuffer[bufferPos++] = b;

      // JPEG end marker: 0xFFD9
      if (b == 0xD9 && jpegBuffer[bufferPos - 2] == 0xFF) {
        capturing = false;

        // Decode JPEG
        jpeg.openRAM(jpegBuffer, bufferPos, drawCallback);
        jpeg.setPixelType(RGB565_FAST);
        jpeg.decode(0, 0, 0);
        jpeg.close();
        bufferPos = 0;
      }
    }   

     //vTaskDelay(pdMS_TO_TICKS(1));
  }
/* // Old RAW Way of doing
    WiFiClient client = serverTcp.available();

    if (client)
    {
      Serial.println("Client connected");
      while (client.connected())
      {
        size_t bytesRead = 0;
        while (bytesRead < FRAME_SIZE)
        {
          int len = client.read(frameBuffer + bytesRead, FRAME_SIZE - bytesRead);
          if (len < 0)
          {
            Serial.println("Read error");
            client.stop();
            break;
          }
          if (len == 0)
          {
            // No data available yet, give it a moment
            delay(1);
            continue;
          }
          bytesRead += len;
        }

        if (bytesRead == FRAME_SIZE)
        {
          // Draw the frame
          for (int i = 0, pixel = 0; i < FRAME_SIZE; i += 2, pixel++)
          {
            uint16_t rgb565 = frameBuffer[i] | (frameBuffer[i + 1] << 8);

            // Extract RGB components (5 bits R, 6 bits G, 5 bits B)
            uint8_t r = (rgb565 >> 11) & 0x1F;
            uint8_t g = (rgb565 >> 5) & 0x3F;
            uint8_t b = rgb565 & 0x1F;

            // No need to convert... dma_display->drawPixel does that
            r = (r << 3) | (r >> 2);
            g = (g << 2) | (g >> 4);
            b = (b << 3) | (b >> 2);

            int x = pixel % WIDTH;
            int y = pixel / WIDTH;

            //gfx_layer_fg.setPixel(x,y,r,g,b);
            layer_draw_callback(x,y,r,g,b);
          }

        }
      }
        Serial.println("Client disconnected");
        client.stop();

    }
*/
#elif
    if (config.status.gif.gifFile)
    {
      if (!config.status.gif.gifFile.isDirectory())
      {
        playGif(config.status.gif.gifFile);
      }
    }
    else
    {
      // No gif file :'(
    }

    if (!config.display.loopGifEnabled)
    {
      // Go get the next file
      config.status.gif.gifFile = root.openNextFile();

      if (!config.status.gif.gifFile)
      {
        root.close();
        root = FILESYSTEM.open(config.gifConfig.gifDir);
        config.status.gif.gifFile = root.openNextFile();
      }
    }
    else
    {
      // we are looping...
    }

#endif

    // vTaskDelay(pdMS_TO_TICKS(1));
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
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

void TaskScreenInfoLayer(void *pvParameters)
{
  int16_t xOne, yOne;
  uint16_t w, h;
  unsigned long isAnimationDue;

  for (;;)
  {

    if (gfx_layer_mutex != NULL)
    {
      if (xSemaphoreTake(gfx_layer_mutex, portMAX_DELAY) == pdTRUE)
      {

        if (config.display.clockEnabled && config.status.validTime)
        {
          // Display the time in the format HH:MM (12/24H)
          gfx_layer_fg.clear();
          gfx_layer_fg.setTextColor(gfx_layer_fg.color565(config.status.textColor.red, config.status.textColor.green, config.status.textColor.blue));
          gfx_layer_fg.setTextSize(2);
          gfx_layer_fg.setCursor(3, 24);
          gfx_layer_fg.print(config.status.clockTime); // Clock time text is processed in another thread
        }
        else if (config.display.scrollTextEnabled)
        {
          // gfx_layer_fg.clear(); // Clear the foreground layer
          gfx_layer_fg.setTextWrap(false); // Disable text wrapping

          switch (config.status.scrollText.scrollFontSize)
          {
          case 1:
            config.status.scrollText.textYPosition = 27;
            break;
          case 3:
            config.status.scrollText.textYPosition = 20;

            break;
          case 4:
            config.status.scrollText.textYPosition = 16;
            break;
          case 2:
          default:
            config.status.scrollText.textYPosition = 24;
            break;
          }

          byte offSet = 25;
          unsigned long now = millis();
          if (now > isAnimationDue)
          {

            gfx_layer_fg.setTextSize(config.status.scrollText.scrollFontSize); // size 2 == 16 pixels high

            isAnimationDue = now + config.status.scrollText.scrollSpeed;
            config.status.scrollText.textXPosition -= 1;

            // Checking is the very right of the text off screen to the left
            gfx_layer_fg.getTextBounds(config.status.scrollText.scrollText.c_str(), config.status.scrollText.textXPosition, config.status.scrollText.textYPosition, &xOne, &yOne, &w, &h);
            if (config.status.scrollText.textXPosition + w <= 0)
            {
              config.status.scrollText.textXPosition = gfx_layer_fg.width() + offSet;
            }

            gfx_layer_fg.setCursor(config.status.scrollText.textXPosition, config.status.scrollText.textYPosition);

            // Clear the area of text to be drawn to
            gfx_layer_fg.drawRect(0, config.status.scrollText.textYPosition - 12, gfx_layer_fg.width(), 42, gfx_layer_fg.color565(0, 0, 0));
            gfx_layer_fg.fillRect(0, config.status.scrollText.textYPosition - 12, gfx_layer_fg.width(), 42, gfx_layer_fg.color565(0, 0, 0));

            uint8_t w = 0;
            for (w = 0; w < strlen(config.status.scrollText.scrollText.c_str()); w++)
            {
              gfx_layer_fg.setTextColor(gfx_layer_fg.color565(config.status.textColor.red, config.status.textColor.green, config.status.textColor.blue));
              gfx_layer_fg.print(config.status.scrollText.scrollText.c_str()[w]);
              // Serial.println(textYPosition);
            }
          }
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