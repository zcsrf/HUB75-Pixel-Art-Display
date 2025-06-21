// Put where panel draw helpers

#include "panel_draw_helpers.h"

uint16_t randomRGB565()
{
  uint8_t r = rand() % 32; // 5 bits
  uint8_t g = rand() % 64; // 6 bits
  uint8_t b = rand() % 32; // 5 bits
  return (r << 11) | (g << 5) | b;
}

// Draws some useful info at boot (after wifi connection)
void bootDraw()
{
    dma_display->fillScreen(dma_display->color565(0, 0, 0));
    dma_display->setTextSize(1);
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
}

// Draws a digital clock in center of the screen
void clockDraw()
{
    // Display the time in the format HH:MM (12/24H)
    gfx_layer_fg.clear();
    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(10, 10, 10));
    gfx_layer_fg.setTextSize(2);
    gfx_layer_fg.setCursor(2, 23);
    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(10, 10, 10));
    gfx_layer_fg.print(config.status.clockTime);
    gfx_layer_fg.setCursor(4, 24);
    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(10, 10, 10));
    gfx_layer_fg.print(config.status.clockTime);
    // gfx_layer_fg.setTextSize(2);
    gfx_layer_fg.setCursor(2, 25);
    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(10, 10, 10));
    gfx_layer_fg.setCursor(4, 25);
    gfx_layer_fg.print(config.status.clockTime);
    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(config.status.textColor.red, config.status.textColor.green, config.status.textColor.blue));
    // gfx_layer_fg.setTextSize(2);
    gfx_layer_fg.setCursor(3, 24);
    gfx_layer_fg.print(config.status.clockTime); // Clock time text is processed in another thread
}

// Draws scolling text
void drawScrollingText()
{
    static int16_t xOne, yOne;
    static uint16_t w, h;
    static unsigned long isAnimationDue = millis();

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