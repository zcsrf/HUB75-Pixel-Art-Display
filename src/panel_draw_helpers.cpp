// Put where panel draw helpers

#include "panel_draw_helpers.h"

const float centerX = PANEL_RES_Y / 2.0f;
const float centerY = PANEL_RES_X / 2.0f;
const int maxIterations = 255; // Max iteration for fractals

// Random is under Drugs
uint8_t noise[PANEL_RES_X][PANEL_RES_Y]; // the noise array

int turns = 0;

typedef struct
{
    double centerX;
    double centerY;
} MandelbrotZoomCenter;

float randRange(float min, float max)
{
    return min + static_cast<float>(rand()) / (RAND_MAX / (max - min));
}

CRGB hsvToRgb(float h, float s, float v)
{
    int i = int(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    float r, g, b;
    switch (i % 6)
    {
    case 0:
        r = v, g = t, b = p;
        break;
    case 1:
        r = q, g = v, b = p;
        break;
    case 2:
        r = p, g = v, b = t;
        break;
    case 3:
        r = p, g = q, b = v;
        break;
    case 4:
        r = t, g = p, b = v;
        break;
    case 5:
        r = v, g = p, b = q;
        break;
    default:
        r = v, g = p, b = q;
        break;
    }

    return CRGB(r * 255, g * 255, b * 255);
}

CRGB hsvToRgb_nonNormalized(float hf, float sf, float vf)
{
    // Normalize inputs to [0.0, 1.0] range
    float h = hf / 360.0f; // Use 360.0f for float division
    float s = sf / 100.0f;
    float v = vf / 100.0f;

    // Call the core normalized conversion function
    return hsvToRgb(h, s, v);
}
void hsvToRgb888(uint8_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b)
{
    if (s == 0)
    {
        // Gray
        r = g = b = v;
        return;
    }

    uint8_t region = h / 43; // 0–5
    uint8_t remainder = (h - region * 43) * 6;

    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    default:
        r = v;
        g = p;
        b = q;
        break;
    }
}

uint32_t hsvToRgb888_packed(uint8_t h, uint8_t s, uint8_t v)
{
    uint8_t r, g, b;
    hsvToRgb888(h, s, v, r, g, b);
    return (r << 16) | (g << 8) | b;
}

MandelbrotZoomCenter getRandomZoomTarget()
{
    // Common "interesting" center regions
    const double knownCenters[][2] = {
        {-0.743643887037151, 0.131825904205330}, // Seahorse Valley
        {-0.1015, 0.633},                        // Elephant Valley
        {-0.8, 0.156},                           // Near main cardioid tip
        {-1.25066, 0.02012},                     // Spiral region
        {-0.45, -0.57},                          // Double spiral
        {-0.75, 0.0},                            // Main cardioid boundary (often leads to deep zooms)
        {0.28, 0.0},                             // Point near the "elephant valley"
        {-1.0, 0.0},                             // Another point on the main cardioid edge
        {-0.194488, 1.05063},                    // A spiraling region
        {-0.194488, 1.05063},                    // A spiraling region
        {-0.158223, 1.031548},                   // Deep zoom point near a mini-Mandelbrot
        {-0.100918, 0.835471},                   // Another good spiraling point
        {-0.748, 0.06},                          // A point in the "double spiral" region
        {-0.1705, 1.0305},                       // Just off the main bulb, usually good for spirals
    };

    int pick = rand() % (sizeof(knownCenters) / sizeof(knownCenters[0]));

    MandelbrotZoomCenter zoom;

    // Start from a known good coordinate and add a bit of randomness
    zoom.centerX = knownCenters[pick][0] + randRange(-0.0005, 0.0005);
    zoom.centerY = knownCenters[pick][1] + randRange(-0.0005, 0.0005);

    return zoom;
}

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

void generateNoise()
{
    for (int y = 0; y < PANEL_RES_Y; y++)
        for (int x = 0; x < PANEL_RES_X; x++)
        {
            noise[y][x] = (rand() % 256);
        }
}

float smoothNoise(float x, float y, float factor)
{
    // get fractional part of x and y
    float fractX = x - int(x);
    float fractY = y - int(y);

    // wrap around
    int x1 = (int(x) + PANEL_RES_X) % PANEL_RES_X;
    int y1 = (int(y) + PANEL_RES_Y) % PANEL_RES_Y;

    // neighbor values
    int x2 = (x1 + PANEL_RES_X - 1) % PANEL_RES_X;
    int y2 = (y1 + PANEL_RES_Y - 1) % PANEL_RES_Y;

    // smooth the noise with bilinear interpolation
    float value = 0.0f;
    value += fractX * fractY * noise[y1][x1];
    value += (1 - fractX) * fractY * noise[y1][x2];
    value += fractX * (1 - fractY) * noise[y2][x1];
    value += (1 - fractX) * (1 - fractY) * noise[y2][x2];

    return value / factor;
}

float turbulence(float x, float y, float size, float factor)
{
    float value = 0.0f, initialSize = size;

    while (size >= 1)
    {
        value += smoothNoise(x / size, y / size, factor) * size;
        size /= 2.0f;
    }

    return (128.0f * value / initialSize);
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

void randomDotAnimation()
{
    static int turns = 0;
    turns++;
    if (turns > 64 * 64)
    {
        turns = 0;
        gfx_layer_bg.clear();
    }

    gfx_layer_bg.drawPixel(rand() % PANEL_RES_X, rand() % PANEL_RES_Y, randomRGB565());
}

void randomVerticalLineAnimation()
{
    turns++;
    if (turns > 64 * 64)
    {
        turns = 0;
        gfx_layer_bg.clear();
    }

    uint16_t color = randomRGB565();
    uint8_t y = rand() % 64;
    for (int x = 0; x < PANEL_RES_X; ++x)
    {
        gfx_layer_bg.drawPixel(x, y, color);
    }
}

void randomHorizontalLineAnimation()
{
    turns++;
    if (turns > 64 * 64)
    {
        turns = 0;
        gfx_layer_bg.clear();
    }

    uint16_t color = randomRGB565();
    uint8_t x = rand() % 64;
    for (int y = 0; y < PANEL_RES_Y; ++y)
    {
        gfx_layer_bg.drawPixel(x, y, color);
    }
}

void randomSquaresAnimation()
{
    turns++;
    if (turns > 64 * 64)
    {
        turns = 0;
        gfx_layer_bg.clear();
    }

    uint8_t y_b = rand() % 64;
    uint8_t x_b = rand() % 64;

    uint8_t y_f = rand() % 64;
    uint8_t x_f = rand() % 64;

    if (y_f > 64)
        y_f = PANEL_RES_Y;
    if (x_f > 64)
        x_f = PANEL_RES_X;

    uint16_t color = randomRGB565();

    for (int y = y_b; y < y_f; ++y)
    {
        for (int x = x_b; x < x_f; ++x)
        {
            gfx_layer_bg.drawPixel(x, y, color);
        }
    }
}

void colorChangeSquaresAnimation()
{
    static int frame = 0;

    for (int y = 0; y < PANEL_RES_Y; ++y)
    {
        for (int x = 0; x < PANEL_RES_X; ++x)
        {
            uint8_t r = (x + frame) % 256;
            uint8_t g = (y + frame * 2) % 256;
            uint8_t b = (x ^ y ^ frame) % 256;
            gfx_layer_bg.setPixel(x, y, r, g, b);
        }
    }

    ++frame;
}

void colorChangeCirclesAnimation()
{
    static float time = 0;

    time += 0.1f;
    const float SPEED = 0.1f;
    const float WAVELENGTH = 100.0f;

    for (int y = 0; y < PANEL_RES_Y; ++y)
    {
        for (int x = 0; x < PANEL_RES_X; ++x)
        {
            float dx = x - PANEL_RES_X / 2.0f;
            float dy = y - PANEL_RES_Y / 2.0f;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Create smooth hue shift based on distance and time
            float hue = std::fmod((time * SPEED + dist / WAVELENGTH), 1.0f);

            gfx_layer_bg.drawPixel(x, y, hsvToRgb(hue, 1.0f, 1.0f));
        }
    }
}

void colorSegmentedCirclesAnimation()
{
    static float time = 0;
    time += 0.05f;
    float radius = std::sin(time * 1.0f) * 46;
    uint8_t center_x = PANEL_RES_Y / 2.0f;
    uint8_t center_y = PANEL_RES_X / 2.0f;
    CRGB moodColor = hsvToRgb(std::fmod((time * 1 + 1 / 1), 1.0f), 1.0f, 1.0f);

    // Draw circle manually
    for (int y = 0; y < PANEL_RES_Y; ++y)
    {
        for (int x = 0; x < PANEL_RES_X; ++x)
        {
            float dx = x - center_x;
            float dy = y - center_y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= radius)
            {
                gfx_layer_bg.drawPixel(x, y, moodColor);
            }
        }
    }
}

void crazyEyeAnimation()
{
    static float time = 0;
    time += 0.05f;
    float radius = std::sin(time * 1.0f) * 46;
    uint8_t center_x = PANEL_RES_Y / 2.0f;
    uint8_t center_y = PANEL_RES_X / 2.0f;
    CRGB moodColor = CRGB(rand() % 255, rand() % 255, rand() % 255);

    // Draw circle manually
    for (int y = 0; y < PANEL_RES_Y; ++y)
    {
        for (int x = 0; x < PANEL_RES_X; ++x)
        {
            float dx = x - center_x;
            float dy = y - center_y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= radius)
            {
                gfx_layer_bg.drawPixel(x, y, moodColor);
            }
        }
    }
}

void alienDnaSequence()
{
    const int flipsPerLine = 5;

    static bool switches[PANEL_RES_X] = {0};
    static int matrix_i = 0, matrix_x = 0;

    for (matrix_i = 0; matrix_i < PANEL_RES_X; matrix_i += 1) // matrix_i +=2 to add spaces
    {
        CRGB moodColor = CRGB(rand() % 255, rand() % 255, rand() % 255);
        // Print character if switches[i] is 1
        // Else print a blank character
        if (switches[matrix_i])
            gfx_layer_bg.drawPixel(matrix_x, matrix_i, moodColor);
        else
            gfx_layer_bg.drawPixel(matrix_x, matrix_i, 0);
    }

    // Flip the defined amount of Boolean values
    // after each line
    for (matrix_i = 0; matrix_i != flipsPerLine; ++matrix_i)
    {
        matrix_x = rand() % PANEL_RES_X;
        switches[matrix_x] = !switches[matrix_x];
    }
}

// This gets cpu heavy the deeper the zoom and the number of iterations
// A new random cRe+Cim and moveX and moveY will be generated when we "find"
// that the view is not really evolving
void juliaFractalAnimation()
{
    static float zoom = 1, moveX = 0.2509827694398746, moveY = 4.45654687711405e-005; // you can change these to zoom and change position
    static float newRe, newIm, oldRe, oldIm;                                          // real and imaginary parts of new and old
    static float cRe = -0.704029749122184, cIm = -0.26;                               // real and imaginary part of the constant c, determinate shape of the Julia Set
    static int sum = 0;
    static int previousSum = -1;
    static int dropValues = 0;
    static int frame = 0;
    static uint32_t previousDiff = 1;
    uint32_t thisDiff = 0;

    float invZoom = 1.0f / (0.5f * zoom * PANEL_RES_Y);
    int i;

    frame++;
    for (int y = 0; y < PANEL_RES_Y; y++)
    {
        for (int x = 0; x < PANEL_RES_X; x++)
        {
            // calculate the initial real and imaginary part of z, based on the pixel location and zoom and position values
            newRe = 1.5 * (x - centerX) * (invZoom) + moveX;
            newIm = (y - centerY) * (invZoom) + moveY;
            // i will represent the number of iterations
            // start the iteration process
            for (i = 0; i < maxIterations; i++)
            {
                // remember value of previous iteration
                oldRe = newRe;
                oldIm = newIm;
                // the actual iteration, the real and imaginary part are calculated
                newRe = oldRe * oldRe - oldIm * oldIm + cRe;
                newIm = 2 * oldRe * oldIm + cIm;
                // if the point is outside the circle with radius 2: stop
                if ((newRe * newRe + newIm * newIm) > 4)
                    break;
            }

            uint8_t hue = i % 256;
            uint8_t value = (i < maxIterations) ? 255 : 0;

            gfx_layer_bg.drawPixel(x, y, hsvToRgb888_packed(hue, 255, value));
        }
        sum = sum + ((i < maxIterations) ? i : 0);
    }
    zoom = zoom + (0.005f * frame);
    // Calculate this sum diff
    thisDiff = abs(sum - previousSum);

    // If the diff is the same between this and the latest... maybe we are not realy evolving..
    if (thisDiff == previousDiff)
    {
        dropValues++;
        if (dropValues > 25)
        {
            frame = 0;
            zoom = 1;
            moveX = randRange(-0.5f, 0.5f);
            moveY = randRange(-0.5f, 0.5f);
            cRe = randRange(-0.7f, 0.7f);
            cIm = randRange(-0.7f, 0.7f);
            dropValues = 0;
        }
    }
    else
    {
        dropValues--;
        if (dropValues < 0)
        {
            dropValues = 0;
        }
    }
    previousDiff = thisDiff;
    previousSum = sum;
}

void mandelbrotFractalAnimation()
{
    // loop through every pixel
    static float zoom = 1, moveX = 0.2509827694398746, moveY = 4.45654687711405e-005; // you can change these to zoom and change position
    static float pr, pi;                                                              // real and imaginary part of the pixel p
    static float newRe, newIm, oldRe, oldIm;                                          // real and imaginary parts of new and old
    int i = 0;
    static int frame = 0;
    static int sum = 0;
    static int previousSum = -1;
    static int dropValues = 0;
    static uint32_t previousDiff = 1;
    uint32_t thisDiff = 0;

    frame++;
    for (int y = 0; y < PANEL_RES_Y; y++)
    {
        for (int x = 0; x < PANEL_RES_X; x++)
        {
            // calculate the initial real and imaginary part of z, based on the pixel location and zoom and position values
            pr = 1.5 * (x - PANEL_RES_X / 2) / (0.5 * zoom * PANEL_RES_X) + moveX;
            pi = (y - PANEL_RES_Y / 2) / (0.5 * zoom * PANEL_RES_Y) + moveY;
            newRe = newIm = oldRe = oldIm = 0; // these should start at 0,0
            // start the iteration process
            for (i = 0; i < maxIterations; i++)
            {
                // remember value of previous iteration
                oldRe = newRe;
                oldIm = newIm;
                // the actual iteration, the real and imaginary part are calculated
                newRe = oldRe * oldRe - oldIm * oldIm + pr;
                newIm = 2 * oldRe * oldIm + pi;
                // if the point is outside the circle with radius 2: stop
                if ((newRe * newRe + newIm * newIm) > 4)
                    break;
            }

            uint8_t hue = i % 256;
            uint8_t value = (i < maxIterations) ? 255 : 0;

            gfx_layer_bg.drawPixel(x, y, hsvToRgb888_packed(hue, 255, value));
        }
        sum = sum + i;
    }
    zoom = zoom + (0.005f * frame);
    // Calculate this sum diff
    thisDiff = abs(sum - previousSum);

    // If the diff is the same between this and the latest... maybe we are not realy evolving..
    if (thisDiff == previousDiff)
    {
        dropValues++;
        if (dropValues > 25)
        {
            frame = 0;
            zoom = 1;
            MandelbrotZoomCenter center = getRandomZoomTarget();
            moveX = center.centerX;
            moveY = center.centerY;
            dropValues = 0;
        }
    }
    else
    {
        dropValues--;
        if (dropValues < 0)
        {
            dropValues = 0;
        }
    }
    previousDiff = thisDiff;
    previousSum = sum;
}

void noisePortalAnimation()
{
    float time;

    time += 0.5f;
    for (int y = 0; y < PANEL_RES_Y; y++)
        for (int x = 0; x < PANEL_RES_X; x++)
        {
            int color = int(128.0 + (128.0 * sin(time + sqrt((x - PANEL_RES_X / 2.0) * (x - PANEL_RES_X / 2.0) + (y - PANEL_RES_Y / 2.0) * (y - PANEL_RES_Y / 2.0)) / 1.5)));

            switch (rand() % 7)
            {
            case 0:
                gfx_layer_bg.setPixel(x, y, color, color, rand() % 255);
                break;
            case 1:
                gfx_layer_bg.setPixel(x, y, color, rand() % 255, color);
                break;
            case 2:
                gfx_layer_bg.setPixel(x, y, color, rand() % 255, rand() % 255);
                break;
            case 3:
                gfx_layer_bg.setPixel(x, y, rand() % 255, color, color);
                break;
            case 4:
                gfx_layer_bg.setPixel(x, y, rand() % 255, color, rand() % 255);
                break;
            case 5:
                gfx_layer_bg.setPixel(x, y, rand() % 255, rand() % 255, color);
                break;
            case 6:
                gfx_layer_bg.setPixel(x, y, rand() % 255, rand() % 255, rand() % 255);
                break;
            case 7:
                gfx_layer_bg.setPixel(x, y, color, color, color);
                break;
            }
        }
}

void randomUnderDrugsAnimation(uint8_t option)
{
    static float time = 0.0f;

    double sum = 0;
    static double previousSum = -1;
    static double previousDiff = -1;
    static int dropValues = 0;

    int thisDiff;
    time += 0.01f;
    uint8_t color;
    uint8_t color1;
    uint8_t color2;

    for (int y = 0; y < PANEL_RES_Y; y++)
    {
        for (int x = 0; x < PANEL_RES_X; x++)
        {
            color = uint8_t(turbulence(x, y, 64, time));
            color1 = uint8_t(turbulence(x, y, 64, time - 1));
            color2 = uint8_t(turbulence(x, y, 64, time + 1));

            switch (option)
            {
            case 0:
                // Black Eater
                gfx_layer_bg.setPixel(x, y, color, color - 255, 0);

                break;
            case 1:
                // World on fire
                gfx_layer_bg.setPixel(x, y, color, 255 - color1, 0);
                break;
            case 2:
            default:
                // Under Drugs
                gfx_layer_bg.setPixel(x, y, color, 255 - color1, color2);
                break;
            }
        }
        sum = sum + color + color1 + color2;
    }

    thisDiff = (abs(sum - previousSum));

    if (thisDiff == previousDiff)
    {
        dropValues++;
        if (dropValues > 100)
        {
            generateNoise();
            time = 0;
            dropValues = 0;
        }
    }

    previousSum = sum;
    previousDiff = thisDiff;
}

void kaleidoscopeAnimation()
{
    static float time = 0;
    time += 0.01f;
    for (int y = 0; y < PANEL_RES_Y / 2; ++y)
    {
        for (int x = 0; x < PANEL_RES_X / 2; ++x)
        {
            float fx = (float)x / PANEL_RES_X;
            float fy = (float)y / PANEL_RES_Y;
            float value = std::sin(1.0f * (fx * std::cos(time) + fy * std::sin(time)));

            // Normalize and get color
            float hue = (value + 1.0f) / 2.0f; // range [0,1]
            CRGB color = hsvToRgb_nonNormalized(hue * 255, 1.0f * 255, 1.0f * 255);

            // Mirror to create kaleidoscope
            gfx_layer_bg.drawPixel(x, y, color);
            gfx_layer_bg.drawPixel(PANEL_RES_X - 1 - x, y, color);
            gfx_layer_bg.drawPixel(x, PANEL_RES_Y - 1 - y, color);
            gfx_layer_bg.drawPixel(PANEL_RES_X - 1 - x, PANEL_RES_Y - 1 - y, color);
            gfx_layer_bg.drawPixel(y, x, color);
            gfx_layer_bg.drawPixel(PANEL_RES_X - 1 - y, x, color);
            gfx_layer_bg.drawPixel(y, PANEL_RES_Y - 1 - x, color);
            gfx_layer_bg.drawPixel(PANEL_RES_X - 1 - y, PANEL_RES_Y - 1 - x, color);
        }
    }
}

void colorWavesAnimation()
{
    static int cx = rand() % 64; // 64; // WIDTH / 2;
    static int cy = rand() % 64;
    static int turn = 0;
    static float time = 0;
    time += 0.1f;
    for (int y = 0; y < PANEL_RES_Y; ++y)
    {
        for (int x = 0; x < PANEL_RES_X; ++x)
        {
            int dx = x - cx;
            int dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Animate the ripple with moving sine wave
            float wave = std::sin(dist * 0.6f - time);

            // Optional falloff with distance (fades outer rings)
            float fade = std::exp(-dist * 0.07f);

            float brightness = (wave + 1.0f) * 0.5f * fade;
            float hue = std::fmod(dist * 0.03f - time * 0.02f, 1.0f);
            if (hue < 0)
                hue += 1.0f;
            CRGB color = hsvToRgb_nonNormalized(hue * 255, 255, brightness * 255);
            gfx_layer_bg.drawPixel(x, y, color);
        }
    }
    turn++;
    if (turn > 1000)
    {
        turn = 0;
        cx = rand() % 64;
        cy = rand() % 64;
    }
}

void dancingColorBlob()
{
    static float time = 0;
    time += 0.05f;
    float cx = PANEL_RES_X / 2 + std::sin(time * 0.9f) * 15;
    float cy = PANEL_RES_Y / 2 + std::cos(time * 0.7f) * 15;

    float maxRadius = 20 + std::sin(time * 1.3f) * 10;

    for (int y = 0; y < PANEL_RES_Y; ++y)
    {
        for (int x = 0; x < PANEL_RES_X; ++x)
        {
            float dx = x - cx;
            float dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy);

            float blob = std::exp(-dist * dist / (2 * maxRadius)); // Gaussian blob shape

            float hue = std::fmod(time * 0.02f + dist * 0.01f, 1.0f);
            float brightness = blob;

            CRGB color = hsvToRgb_nonNormalized(hue * 255, 255, brightness * 255);
            gfx_layer_bg.drawPixel(x, y, color);
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