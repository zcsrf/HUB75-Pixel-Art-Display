// Put where panel draw helpers

#include "panel_draw_helpers.h"

File findImageByPath(File root, const String &targetPath)
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

uint16_t randomRGB565()
{
    uint8_t r = rand() % 32; // 5 bits
    uint8_t g = rand() % 64; // 6 bits
    uint8_t b = rand() % 32; // 5 bits
    return (r << 11) | (g << 5) | b;
}

void stackLayers()
{
    if (gfx_layer_mutex != NULL)
    {
        if (xSemaphoreTake(gfx_layer_mutex, portMAX_DELAY) == pdTRUE)
        {
            // gfx_layer_bg.dim(150);
            // gfx_layer_fg.dim(255);
            gfx_compositor.Stack(gfx_layer_bg, gfx_layer_fg); // Combine the bg and the fg layer and draw it onto the panel.
            xSemaphoreGive(gfx_layer_mutex);                  // After accessing the shared resource give the mutex and allow other processes to access it
        }
    }
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
    /*
    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(10, 10, 10));
    gfx_layer_fg.setTextSize(2);
    gfx_layer_fg.setCursor(2, 24);
    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(10, 10, 10));
    gfx_layer_fg.print(config.status.clockTime);
    gfx_layer_fg.setCursor(4, 24);
    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(10, 10, 10));
    gfx_layer_fg.print(config.status.clockTime);
    // gfx_layer_fg.setTextSize(2);
    gfx_layer_fg.setCursor(3, 22);
    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(10, 10, 10));
    gfx_layer_fg.setCursor(3, 25);
    gfx_layer_fg.print(config.status.clockTime);
    */

    gfx_layer_fg.clear();

    uint16_t shadowColor = gfx_layer_fg.color565(10, 10, 10); // Black

    // Draw shadow in 4 directions around the text
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0)
                continue; // Skip center
            gfx_layer_fg.setCursor(3 + dx, 24 + dy);
            gfx_layer_fg.setTextColor(shadowColor);
            gfx_layer_fg.print(config.status.clockTime);
        }
    }

    gfx_layer_fg.setTextColor(gfx_layer_fg.color565(config.status.textColor.red, config.status.textColor.green, config.status.textColor.blue));
    gfx_layer_fg.setTextSize(2);
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

void *gifOpenFile(const char *fname, int32_t *pSize)
{
    Serial.print("Playing gif: ");
    Serial.println(fname);
    config.status.fileStatus.currentFile = FILESYSTEM.open(fname);
    if (config.status.fileStatus.currentFile)
    {
        *pSize = config.status.fileStatus.currentFile.size();
        return (void *)&config.status.fileStatus.currentFile;
    }
    return NULL;
} /* GIFOpenFile() */

void gifCloseFile(void *pHandle)
{
    File *f = static_cast<File *>(pHandle);
    if (f != NULL)
        f->close();
}

int32_t gifReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
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
}

int32_t gifSeekFile(GIFFILE *pFile, int32_t iPosition)
{
    // int i = micros();
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();
    // i = micros() - i;
    //   Serial.printf("Seek time = %d us\n", i);
    return pFile->iPos;
}

void gifDraw(GIFDRAW *pDraw)
{
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[320];
    int x, y, iWidth;

    iWidth = pDraw->iWidth;
    if (iWidth > PANEL_RES_X)
        iWidth = PANEL_RES_X;

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

// Move this to Client handler
void checkForTcpClient()
{
    if (!client || !client.connected())
    {
        client = serverTcp.available();
    }
}

void showLocalFile(File file)
{
    // Refresh our "timeout" counter
    unsigned long displayTime = millis();

    if (String(file.path()).endsWith(".gif"))
    {
        if (gif.open(file.path(), gifOpenFile, gifCloseFile, gifReadFile, gifSeekFile, gifDraw))
        {
            // Keep looping on the gif... do logic to exit the loop, reset or close gif if needed
            while (1)
            {
                // We need to break the GIF playback instantly on certain conditions (tcp packets...)
                while (gif.playFrame(true, NULL) && (!client || !client.connected()))
                {
                    checkForTcpClient();
                    stackLayers();

                    if (!config.display.imagesEnabled) // Check if GIF playback is enabled
                    {
                        gfx_layer_bg.clear(); // Clear the background layer if GIF playback is disabled
                        return;
                    }
                }

                // Wow...we might have a client sending tcp packets... lets skip the gif stuff and go back to screenDraw loop
                if (client && client.available())
                {
                    gif.close();
                    return;
                }

                // Don't close if we are looping.... don't waste time, open and closing and ..
                // Logic is cheaper...
                if (config.display.loopImagesEnabled)
                {
                    // You are in loop and you didn't request another file
                    if (config.status.fileStatus.requestedFilePath.isEmpty())
                    {
                        gif.reset();
                    }
                    // You are in loop but did request another file
                    else
                    {
                        gif.close();
                        File root = FILESYSTEM.open(config.filesConfig.filesDir);
                        config.status.fileStatus.displayFile = findImageByPath(root, config.status.fileStatus.requestedFilePath);
                        return;
                    }
                }
                // We are not looping the image... just wait till timeout to go to next one.
                else
                {
                    // If image timeout is reached we can close and handle the next file
                    if (((millis() - displayTime) > config.display.imageTimeoutSeconds * 1000))
                    {
                        gif.close();
                        return;
                    }
                }
            }
        }
    }
    else if (String(file.path()).endsWith(".jpeg") || String(file.path()).endsWith(".jpg"))
    {

        Serial.print("Loaded a JPG");

        config.status.fileStatus.currentFile = FILESYSTEM.open(String(file.path()));

        if (!config.status.fileStatus.currentFile || !config.status.fileStatus.currentFile.available())
        {
            Serial.println("File open failed or empty!");
            return;
        }

        int len = config.status.fileStatus.currentFile.size();
        if (len <= 0)
        {
            Serial.println("File size invalid or zero");
            return;
        }

        uint8_t *jpgData = (uint8_t *)malloc(len);
        if (!jpgData)
        {
            Serial.println("Malloc failed!");
            return;
        }

        int bytesRead = config.status.fileStatus.currentFile.read(jpgData, len);
        config.status.fileStatus.currentFile.close();

        if (bytesRead != len)
        {
            Serial.println("Read incomplete or corrupted");
            free(jpgData);
            return;
        }

        if (jpeg.openRAM(jpgData, len, jpegDrawCallback))
        {
            jpeg.setPixelType(RGB565_LITTLE_ENDIAN); // bb_spi_lcd uses big-endian RGB565 pixels
            jpeg.decode(0, 0, 0);                    // decode at (0,0), scale=0 (1:1)
            jpeg.close();
        }

        free(jpgData);

        do
        {
            checkForTcpClient();

            stackLayers();

            // Wow...we might have a client sending tcp packets... lets skip the gif stuff and go back to screenDraw loop
            if (client && client.available())
            {
                return;
            }

            // Don't close if we are looping.... don't waste time, open and closing and ..
            // Logic is cheaper...
            if (!config.status.fileStatus.requestedFilePath.isEmpty())
            {
                File root = FILESYSTEM.open(config.filesConfig.filesDir);
                config.status.fileStatus.displayFile = findImageByPath(root, config.status.fileStatus.requestedFilePath);
                return;
            }

            if (!config.display.loopImagesEnabled && ((millis() - displayTime) > config.display.imageTimeoutSeconds * 1000))
            {
                Serial.println("Loop is disabled");
                return;
            }

            // JPG is displayed for enouth time
            /*if ((millis() - displayTime ) > 5000)
            {
                Serial.println("JPEG Timeout");
                return;
            }*/

            if (!config.display.imagesEnabled) // Check if GIF playback is enabled
            {
                Serial.println("Image is not enable");
                gfx_layer_bg.clear(); // Clear the background layer if GIF playback is disabled
                stackLayers();
                return;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        } while (!(client && client.available()));
    }
    else
    {
        Serial.print("Selected file is not a gif: ");
        Serial.println(file.path());
    }
}

int jpegFastDrawCallback(JPEGDRAW *pDraw)
{

    uint16_t *p = (uint16_t *)pDraw->pPixels;
    for (int y = 0; y < pDraw->iHeight; y++)
    {
        for (int x = 0; x < pDraw->iWidthUsed; x++)
        {
            int index = y * pDraw->iWidth + x; // line-by-line
            uint16_t color = p[index];
            layer_draw_callback_alt(pDraw->x + x, pDraw->y + y, color);
        }
    }
    return 1; //
}

int jpegDrawCallback(JPEGDRAW *pDraw)
{
    uint16_t *p = (uint16_t *)pDraw->pPixels;
    for (int y = 0; y < pDraw->iHeight; y++)
    {
        for (int x = 0; x < pDraw->iWidthUsed; x++)
        {
            int index = y * pDraw->iWidth + x; // line-by-line
            uint16_t color = p[index];
            gfx_layer_bg.drawPixel(pDraw->x + x, pDraw->y + y, color); // color 565s
        }
    }
    return 1;
}

void layer_draw_callback(int16_t x, int16_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data)
{
    dma_display->drawPixel(x, y, dma_display->color565(r_data, g_data, b_data));
}

void layer_draw_callback_alt(int16_t x, int16_t y, uint16_t color)
{
    dma_display->drawPixel(x, y, color);
}