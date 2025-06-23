#ifndef PANEL_DRAW_HELPERS_h
#define PANEL_DRAW_HELPERS_h

#include <LittleFS.h>
#include <GFX_Layer.hpp>
#include <WiFi.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <JPEGDEC.h>

#include "device_config.h"
#include "panel_config.h"
#include <AnimatedGIF.h>


extern MatrixPanel_I2S_DMA *dma_display;
extern GFX_Layer gfx_layer_fg;
extern GFX_Layer gfx_layer_bg;
extern Config config;
extern JPEGDEC jpeg;
extern AnimatedGIF gif;
extern SemaphoreHandle_t gfx_layer_mutex;
extern GFX_LayerCompositor gfx_compositor;
extern WiFiClient client;
extern WiFiServer serverTcp;

File findImageByPath(File root, const String &targetPath);
uint16_t randomRGB565();
void stackLayers();
void bootDraw();
void clockDraw();
void drawScrollingText();

void *gifOpenFile(const char *fname, int32_t *pSize);
void gifCloseFile(void *pHandle);
int32_t gifReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen);
int32_t gifSeekFile(GIFFILE *pFile, int32_t iPosition);
void gifDraw(GIFDRAW *pDraw);

void showLocalFile(File file);

int jpegDrawCallback(JPEGDRAW *pDraw);
int jpegFastDrawCallback(JPEGDRAW *pDraw);

void layer_draw_callback(int16_t x, int16_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data);
void layer_draw_callback_alt(int16_t x, int16_t y, uint16_t color);

// Mode this to client handler...
void checkForTcpClient();


#endif