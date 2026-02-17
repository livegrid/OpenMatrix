#include "ConstellationEffect.h"
#include "../TOFSensor/TOFSensor.h"

// Sin lookup: 0-255 phase -> twinkle multiplier ~64-255 (0.25 to 1.0 of base)
// sin(2*pi*x/256) * 95 + 160  => range ~65-255
const uint8_t ConstellationEffect::SIN_TABLE[256] = {
    160, 163, 166, 169, 172, 175, 178, 181, 184, 187, 190, 193, 196, 198, 201, 204,
    207, 210, 212, 215, 218, 220, 223, 225, 228, 230, 233, 235, 237, 240, 242, 244,
    246, 248, 250, 252, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    254, 252, 250, 248, 246, 244, 242, 240, 237, 235, 233, 230, 228, 225, 223, 220,
    218, 215, 212, 210, 207, 204, 201, 198, 196, 193, 190, 187, 184, 181, 178, 175,
    172, 169, 166, 163, 160, 157, 154, 151, 148, 145, 142, 139, 136, 133, 130, 127,
    124, 122, 119, 116, 113, 111, 108, 105, 102, 100, 97, 95, 92, 90, 87, 85, 82, 80,
    78, 75, 73, 71, 69, 66, 64, 66, 69, 71, 73, 75, 78, 80, 82, 85, 87, 90, 92, 95,
    97, 100, 102, 105, 108, 111, 113, 116, 119, 122, 124, 127, 130, 133, 136, 139,
    142, 145, 148, 151, 154, 157
};

// Knuth multiplicative hash for deterministic per-star variation from index
static inline uint32_t starHash(uint16_t i, uint8_t shift) {
    return (i * 2654435761u) >> shift;
}

uint8_t ConstellationEffect::getBaseHue(uint16_t i) const {
    return HUE_BASE + (uint8_t)(starHash(i, 24) % HUE_RANGE);
}

uint8_t ConstellationEffect::getTwinklePhase(uint16_t i, uint32_t time) const {
    // Stagger phase by index; use millis for animation
    uint32_t phase = (time >> 2) + (i * 7919u);  // 7919 prime for spread
    return (uint8_t)(phase & 0xFF);
}

uint8_t ConstellationEffect::getSaturation(uint16_t i) const {
    return SAT_BASE + (uint8_t)(starHash(i, 20) % SAT_RANGE);
}

// ============================================================================
// ConstellationEffect implementation
// ============================================================================

ConstellationEffect::ConstellationEffect(Matrix* matrix, TOFSensor* sensor)
    : Effect(matrix), tofSensor(sensor), starCount(0) {

    screenWidth = m_matrix->getXResolution();
    screenHeight = m_matrix->getYResolution();

    tofGridReady = false;
    minDetectionDistance = 1000;
    maxDetectionDistance = 2000;
    tofRotation = 270;

    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            tofGrid[y][x] = 0;
            depthField[y][x] = 0;
        }
    }

    reset();
}

void ConstellationEffect::reset() {
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
    starCount = MAX_STARS;

    for (uint16_t i = 0; i < starCount; i++) {
        px[i] = (uint16_t)random(0, screenWidth);
        py[i] = (uint16_t)random(0, screenHeight);
        brightness[i] = 0;
        currentHue[i] = getBaseHue(i);
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
        for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
            for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
                depthField[y][x] = 0;
            }
        }
        return;
    }

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
            if (depth > minDetectionDistance && depth < maxDetectionDistance) {
                value = 1.0f;
            }
            depthField[y][x] = value;
        }
    }
}

float ConstellationEffect::getDepthBoostAt(float x, float y) {
    if (!tofGridReady) return 0;

    float gx = (screenWidth > 1) ? (x / (float)(screenWidth - 1)) * (float)(TOF_GRID_SIZE - 1) : 0;
    float gy = (screenHeight > 1) ? (y / (float)(screenHeight - 1)) * (float)(TOF_GRID_SIZE - 1) : 0;

    int x0 = (int)gx;
    int y0 = (int)gy;
    int x1 = min((int)(TOF_GRID_SIZE - 1), x0 + 1);
    int y1 = min((int)(TOF_GRID_SIZE - 1), y0 + 1);
    float tx = gx - x0;
    float ty = gy - y0;

    x0 = max(0, min((int)(TOF_GRID_SIZE - 1), x0));
    y0 = max(0, min((int)(TOF_GRID_SIZE - 1), y0));

    float v00 = depthField[y0][x0];
    float v10 = depthField[y0][x1];
    float v01 = depthField[y1][x0];
    float v11 = depthField[y1][x1];

    float v0 = v00 + (v10 - v00) * tx;
    float v1 = v01 + (v11 - v01) * tx;
    return v0 + (v1 - v0) * ty;
}

void ConstellationEffect::updateStars() {
    constexpr uint8_t BOOST_THRESHOLD = 13;  // ~0.05 * 255
    constexpr uint16_t BOOST_SCALE = 255 - BOOST_THRESHOLD;

    uint32_t now = millis();

    for (uint16_t i = 0; i < starCount; i++) {
        float depthBoost = getDepthBoostAt((float)px[i], (float)py[i]);
        uint16_t depthBoost255 = (uint16_t)(depthBoost * 255);

        // Twinkle: SIN_TABLE gives 64-255 multiplier; base brightness ~25-70 (scaled to 0-255)
        uint8_t twinkleMul = SIN_TABLE[getTwinklePhase(i, now)];
        uint8_t baseBright = 64 + (uint8_t)(starHash(i, 22) % 46);  // ~25-70% range
        int16_t targetBright = (int16_t)((baseBright * (int)twinkleMul) >> 8);

        // Depth boost: when hand detected, ramp up toward 255
        if (depthBoost255 > BOOST_THRESHOLD) {
            uint16_t norm = ((uint16_t)depthBoost255 - BOOST_THRESHOLD) * 255 / BOOST_SCALE;
            uint16_t normSq = (norm * norm) >> 8;  // Quadratic for punch
            targetBright = targetBright + ((255 - targetBright) * (int)normSq >> 8);
        }

        if (targetBright < 0) targetBright = 0;
        else if (targetBright > 255) targetBright = 255;

        // Integer smoothing: fast rise, slow fade (like PlanktonField)
        int16_t diff = (int16_t)targetBright - (int16_t)brightness[i];
        if (diff > 0) {
            int16_t rise = max(1, diff >> BRIGHTNESS_RISE_SHIFT);
            brightness[i] = (uint8_t)min(255, (int)brightness[i] + rise);
        } else if (diff < 0) {
            int16_t fade = min(-1, diff >> BRIGHTNESS_FADE_SHIFT);
            brightness[i] = (uint8_t)max(0, (int)brightness[i] + fade);
        }

        // Hue transition: baseHue -> HUE_ACTIVATED when hand detected
        uint8_t baseH = getBaseHue(i);
        uint8_t targetHue = (depthBoost255 > BOOST_THRESHOLD) ? HUE_ACTIVATED : baseH;

        int16_t hueDiff = (int16_t)targetHue - (int16_t)currentHue[i];
        if (hueDiff > 0) {
            int16_t rise = max(1, hueDiff >> BRIGHTNESS_RISE_SHIFT);
            currentHue[i] = (uint8_t)min(255, (int)currentHue[i] + rise);
        } else if (hueDiff < 0) {
            int16_t fade = min(-1, hueDiff >> BRIGHTNESS_FADE_SHIFT);
            currentHue[i] = (uint8_t)max(0, (int)currentHue[i] + fade);
        }
    }
}

void ConstellationEffect::drawStars() {
    for (uint16_t i = 0; i < starCount; i++) {
        if (brightness[i] == 0) continue;

        uint16_t x = px[i];
        uint16_t y = py[i];
        if (x >= screenWidth || y >= screenHeight) continue;

        CRGB color;
        hsv2rgb_rainbow(CHSV(currentHue[i], getSaturation(i), brightness[i]), color);
        m_matrix->background->drawPixel(x, y, color);
    }
}

void ConstellationEffect::update() {
    updateTofData();
    buildDepthField();

    m_matrix->background->fillScreen(0);

    updateStars();
    drawStars();
}
