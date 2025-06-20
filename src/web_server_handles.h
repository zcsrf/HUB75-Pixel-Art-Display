#ifndef WEB_SERVER_HANDLES_h
#define WEB_SERVER_HANDLES_h

#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LittleFS.h>

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "device_config.h"

extern WebServer server;
extern Config config;
extern Preferences preferences;
extern MatrixPanel_I2S_DMA *dma_display;

String humanReadableSize(const size_t bytes);
void handleToggleGif();
void handleToggleClock();
void handleToggleLoopGif();
void handleToggleScrollText();
void handleAdjustSlider();
void handleSetReboot();
void handleUpload();
void handleListFiles();
void handleListFilesAlt();
void handleVersionFlash();
void handleFileRequest();
void handleSetColor();
void handleSetScrollText();

#endif