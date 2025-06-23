# HUB75 Pixel Art Display

A Wi-Fi-enabled LED Pixel Art Display based on HUB75E panels, powered by an ESP32.

This is based on the work of [mzashh - HUB75-Pixel-Art-Display](https://github.com/mzashh/HUB75-Pixel-Art-Display) with lots of customized changes

## Changelog

- To be devolped

## Features

- **Wi-Fi Connectivity**: 
  - Connects to the best AP
  - On startup, the panel connects to the Wi-Fi network and displays the firmware version, IP address, RSSI, and Wi-Fi SSID for some time (to be tunned later on)
- **GIF Playback**: Plays GIF files stored in the ESP32's SPI Flash.
- **JPEG Playback**: Plays JPEG stored in the ESP32's SPI Flash
- **MJPEG Streaming**: Allows streaming video to the ESP32 using ffmpeg, mjpeg and TCP.
- **NTP Clock**: Displays the current time (configurable GMT and DST offsets in the firmware).
- **WebUI**:
  - Upload, delete, download, and play Image files.
  - Control brightness via a slider.
  - Change text and clock colors.
  - Adjust text scroll speed and size.
  - Enable or disable Image playback, clock, or scrolling text individually.
  - Toggle Image looping to either loop a single Image or play all stored Image sequentially.
  - Replace the clock with custom scrolling text.
  - Remote rebooting of the ESP32. - To be implemented
  - Modern UI based on Bootstrap
- **API Endpoints**: Allow remote controlling using NodeRed or similar
- **Preferences**: Some variables are stored and recovered after boot.

---

## Planned Features

> Some of these might not be possible together (memory / flash constrains?)

- **MJPEG Playback**:  Plays MJPEG stored in the ESP32's SPI Flash
- **MJPEG Streaming**: Simple Overlay text over streaming MJPEG - Seems to be tricky (cpu constrain?)
- **Animations**:
  - Matrix
  - Kaleidoscope
  - Wormhole
  - Star Wars, HyperDrive
  - Dancing Blob
  - Random under Drugs
  * Plasma
  * Noise portal
  * Mandelbrot
  * Julia Set
- RAW streaming or use python to stream a dashboard using MJPEG

---

## Hardware Requirements

- **HUB75 Panel**: Compatible with the [ESP32-HUB75-MatrixPanel-DMA library](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA). By default, a 64x64 1/32 scan panel is supported.
- **ESP32 Board**: ESP32, ESP32-S2, or ESP32-S3

You might have to change platformio.ini or something else depending on your hardware.

As is, I am using an [aliexpress set](https://pt.aliexpress.com/item/1005007201147335.htm), this is not the best setup, no PSRAM.

Also my Led Panel arrived with damage on two LEDs on one corner.

### Pin Configuration

>Check the panel_config.h, adjust as required.

## Firmware

### Development Tools
- **PlatformIO**:
  - If you plan on streaming stuff, you will need the compile flags.

### Required Libraries
- [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA)
- [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF)
- [GFX_Lite](https://github.com/mrcodetastic/GFX_Lite)
- TODO: Add other libraries -> **bitbank2/JPEGDEC**

### Configuration
- **Wi-Fi**:
  - Change the Wi-Fi SSID and password in `device_config.h` (only 2.4GHz networks are supported).
- **Panel Resolution**:
  - For now we stick with 64x64... Higher than this and we get into ram constrains for much of the stuff.
- **Authentication**:
  - I ditched the authentication...

---

## Case and Assembly

Check [mzashh - HUB75-Pixel-Art-Display](https://github.com/mzashh/HUB75-Pixel-Art-Display) if you want an example of case and diffuser, I still didn't have time to do something for my hardware.

## Credits

- **HUB75-Pixel-Art-Display**: The main repo that motivated me to play arround with this [mzashh - HUB75-Pixel-Art-Display](https://github.com/mzashh/HUB75-Pixel-Art-Display)`
- **GIF Playback**: Based on examples from the [ESP32-HUB75-MatrixPanel-DMA library](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA).

