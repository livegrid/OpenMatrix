#include "ConstellationEffect.h"
#include "../TOFSensor/TOFSensor.h"

// Static random seed (file-scope for use by both Star and Effect)
static uint32_t constellationNoiseSeed = 98765;

// Standalone random function accessible to ConstellationStar
static float constellationRandomFloat() {
    constellationNoiseSeed = constellationNoiseSeed * 1103515245 + 12345;
    return (float)(constellationNoiseSeed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

static float constellationNoise(float x) {
    // Simple value noise implementation
    int xi = (int)x;
    float xf = x - xi;
    
    // Hash function
    uint32_t seed1 = (xi * 1103515245 + 12345);
    float a = (float)(seed1 & 0x7FFFFFFF) / (float)0x7FFFFFFF;
    uint32_t seed2 = ((xi + 1) * 1103515245 + 12345);
    float b = (float)(seed2 & 0x7FFFFFFF) / (float)0x7FFFFFFF;
    
    // Smooth interpolation
    float t = xf * xf * (3.0f - 2.0f * xf);
    return a + t * (b - a);
}

// ============================================================================
// ConstellationStar implementation
// ============================================================================

void ConstellationStar::init(float px, float py) {
    x = px;
    y = py;
    baseBrightness = 10.0f + constellationRandomFloat() * 40.0f;  // 30-70
    twinkleSpeed = 0.001f + constellationRandomFloat() * 0.008f;  // 0.004-0.012
    twinkleOffset = constellationRandomFloat() * 6.2832f;  // 0 to 2*PI
    baseHue = 120 + (uint8_t)(constellationRandomFloat() * 53);  // FastLED hue: 120-173 (cyan to blue/purple)
    currentHue = baseHue;
    saturation = 51 + (uint8_t)(constellationRandomFloat() * 102);  // 20-60% mapped to 51-153
    currentBrightness = baseBrightness;
}

void ConstellationStar::updateBrightness(uint32_t time, float depthBoost) {
    // Calculate twinkle (sine wave oscillation)
    float twinklePhase = time * twinkleSpeed + twinkleOffset;
    float twinkle = sinf(twinklePhase) * 0.25f + 0.75f;  // 0.5 to 1.0 range
    float targetBright = baseBrightness * twinkle;
    
    // Apply depth boost if hand is detected nearby
    const float boostThreshold = 0.05f;
    if (depthBoost > boostThreshold) {
        float normalizedBoost = (depthBoost - boostThreshold) / (1.0f - boostThreshold);
        float boostAmount = normalizedBoost * normalizedBoost;  // Quadratic for punchy response
        targetBright = targetBright + (100.0f - targetBright) * boostAmount;
    }
    
    // Smooth transition - faster rise, slower fade for nice glow effect
    const float riseSpeed = 0.2f;
    const float fadeSpeed = 0.06f;
    float speed = (targetBright > currentBrightness) ? riseSpeed : fadeSpeed;
    currentBrightness += (targetBright - currentBrightness) * speed;

    // Smooth hue shift when activated (orange-red glow)
    float targetHue = (depthBoost > boostThreshold) ? 12.0f : (float)baseHue;
    float hueSpeed = (targetHue != currentHue) ? speed : fadeSpeed;
    currentHue += (targetHue - currentHue) * hueSpeed;
    if (currentHue < 0) currentHue = 0;
    if (currentHue > 255) currentHue = 255;
    
    // Clamp
    if (currentBrightness < 0) currentBrightness = 0;
    if (currentBrightness > 100) currentBrightness = 100;
}

// ============================================================================
// ConstellationEffect implementation
// ============================================================================

ConstellationEffect::ConstellationEffect(Matrix* matrix, TOFSensor* sensor)
    : Effect(matrix), tofSensor(sensor), stars(nullptr) {
    
    // Get screen dimensions
    screenWidth = m_matrix->getXResolution();
    screenHeight = m_matrix->getYResolution();
    
    // Allocate stars on heap to avoid stack overflow
    stars = new ConstellationStar[MAX_STARS];
    
    // TOF parameters
    tofGridReady = false;
    minDetectionDistance = 1000;
    maxDetectionDistance = 2000;
    tofRotation = 270;  // Default rotation
    
    frameCount = 0;
    lastUpdateTime = 0;
    starCount = 0;
    
    // Initialize grids
    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            tofGrid[y][x] = 0;
            depthField[y][x] = 0;
        }
    }
    
    reset();
}

ConstellationEffect::~ConstellationEffect() {
    if (stars) {
        delete[] stars;
        stars = nullptr;
    }
}

void ConstellationEffect::reset() {
    frameCount = 0;
    lastUpdateTime = millis();
    initStars();
    m_matrix->background->fillScreen(0);
}

const char* ConstellationEffect::getName() const {
    return "Constellation";
}

void ConstellationEffect::setTofSensor(TOFSensor* sensor) {
    tofSensor = sensor;
}

void ConstellationEffect::setDetectionRange(int16_t minDist, int16_t maxDist) {
    minDetectionDistance = minDist;
    maxDetectionDistance = maxDist;
}

void ConstellationEffect::setTofRotation(uint16_t rotation) {
    tofRotation = rotation;
}

void ConstellationEffect::initStars() {
    if (!stars) return;
    
    starCount = MAX_STARS;
    
    for (uint16_t i = 0; i < starCount; i++) {
        float x = constellationRandomFloat() * screenWidth;
        float y = constellationRandomFloat() * screenHeight;
        stars[i].init(x, y);
    }
}

void ConstellationEffect::rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY) {
    switch (tofRotation) {
        case 90:
            outX = 7 - y;
            outY = x;
            break;
        case 180:
            outX = 7 - x;
            outY = 7 - y;
            break;
        case 270:
            outX = y;
            outY = 7 - x;
            break;
        default:
            outX = x;
            outY = y;
            break;
    }
}

void ConstellationEffect::updateTofData() {
    if (!tofSensor || !tofSensor->isActive()) {
        tofGridReady = false;
        // Clear depth field when no sensor
        for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
            for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
                depthField[y][x] = 0;
            }
        }
        return;
    }
    
    // Read data from sensor with rotation
    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            uint8_t rx, ry;
            rotateCoordinates(x, y, rx, ry);
            tofGrid[y][x] = tofSensor->getDistance(rx, ry);
        }
    }
    tofGridReady = true;
}

void ConstellationEffect::buildDepthField() {
    if (!tofGridReady) {
        // Clear depth field when no valid data
        for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
            for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
                depthField[y][x] = 0;
            }
        }
        return;
    }
    
    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            int16_t depth = tofGrid[y][x];
            float value = 0;
            
            // Only activate if depth is in valid detection range
            if (depth > minDetectionDistance && depth < maxDetectionDistance) {
                // Equal response across all in-range distances
                value = 1.0f;
            }
            
            depthField[y][x] = value;
        }
    }
}

float ConstellationEffect::getDepthBoostAt(float x, float y) {
    if (!tofGridReady) return 0;
    
    // Map star position to grid coordinates (0-7 range)
    float gx = (x / screenWidth) * 7.0f;
    float gy = (y / screenHeight) * 7.0f;
    
    // Bilinear interpolation for smooth gradient
    int x0 = (int)gx;
    int y0 = (int)gy;
    int x1 = min(7, x0 + 1);
    int y1 = min(7, y0 + 1);
    float tx = gx - x0;
    float ty = gy - y0;
    
    // Clamp indices
    x0 = max(0, min(7, x0));
    y0 = max(0, min(7, y0));
    
    float v00 = depthField[y0][x0];
    float v10 = depthField[y0][x1];
    float v01 = depthField[y1][x0];
    float v11 = depthField[y1][x1];
    
    float v0 = v00 + (v10 - v00) * tx;
    float v1 = v01 + (v11 - v01) * tx;
    return v0 + (v1 - v0) * ty;
}

void ConstellationEffect::updateStars() {
    if (!stars) return;
    
    uint32_t now = millis();
    
    for (uint16_t i = 0; i < starCount; i++) {
        float depthBoost = getDepthBoostAt(stars[i].x, stars[i].y);
        stars[i].updateBrightness(now, depthBoost);
    }
}

void ConstellationEffect::drawStars() {
    if (!stars) return;
    
    for (uint16_t i = 0; i < starCount; i++) {
        ConstellationStar& star = stars[i];
        
        // Convert HSB brightness (0-100) to FastLED value (0-255)
        uint8_t value = (uint8_t)(star.currentBrightness * 2.55f);
        
        // Create color using FastLED HSV
        CRGB color;
        hsv2rgb_rainbow(CHSV((uint8_t)star.currentHue, star.saturation, value), color);
        
        // Draw single pixel star
        int16_t px = (int16_t)star.x;
        int16_t py = (int16_t)star.y;
        
        if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight) {
            m_matrix->background->drawPixel(px, py, color);
        }
    }
}

void ConstellationEffect::update() {
    frameCount++;
    
    // Update TOF sensor data and build depth field
    updateTofData();
    buildDepthField();
    
    // Clear screen to black
    m_matrix->background->fillScreen(0);
    
    // Update star brightness based on twinkle and depth
    updateStars();
    
    // Draw all stars
    drawStars();
}
