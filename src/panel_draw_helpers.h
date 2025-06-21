#ifndef PANEL_DRAW_HELPERS_h
#define PANEL_DRAW_HELPERS_h

#include <GFX_Layer.hpp>
#include <WiFi.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "device_config.h"

extern MatrixPanel_I2S_DMA *dma_display;
extern GFX_Layer gfx_layer_fg;
extern GFX_Layer gfx_layer_bg;
extern Config config;

uint16_t randomRGB565();
void clockDraw();
void drawScrollingText();
void bootDraw();

#endif