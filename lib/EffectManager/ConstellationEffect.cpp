#include "ConstellationEffect.h"
#include "../TOFSensor/TOFSensor.h"

namespace {
constexpr int16_t kTofEffectRotationDeg = 180;  // Set effect remap here: 0/90/180/270
}

// Sin lookup: 0-255 phase -> twinkle multiplier ~64-255 (0.25 to 1.0 of base)
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

// Knuth multiplicative hash for deterministic per-star variation
static inline uint32_t starHash(uint16_t i, uint8_t shift) {
    return (i * 2654435761u) >> shift;
}

// ============================================================================
// Star personality derived from index
// ============================================================================

uint8_t ConstellationEffect::getBrightnessClass(uint16_t i) const {
    // 70% dim, 25% medium, 5% bright
    uint8_t roll = starHash(i, 24) % 100;
    if (roll < 70) return 0;       // dim
    if (roll < 95) return 1;       // medium
    return 2;                      // bright
}

uint8_t ConstellationEffect::getBaseBrightness(uint16_t i) const {
    uint8_t cls = getBrightnessClass(i);
    uint32_t h = starHash(i, 20);
    switch (cls) {
        case 0:  return 8 + (h % 23);     // 8-30
        case 1:  return 40 + (h % 51);    // 40-90
        default: return 140 + (h % 81);   // 140-220
    }
}

uint8_t ConstellationEffect::getBaseHue(uint16_t i) const {
    return HUE_STAR + (uint8_t)(starHash(i, 24) % HUE_STAR_RANGE) - (HUE_STAR_RANGE / 2);
}

uint8_t ConstellationEffect::getBaseSaturation(uint16_t i) const {
    uint8_t cls = getBrightnessClass(i);
    switch (cls) {
        case 0:  return SAT_DIM;
        case 1:  return SAT_MEDIUM;
        default: return SAT_BRIGHT;
    }
}

uint8_t ConstellationEffect::getTwinklePhase(uint16_t i, uint32_t time) const {
    // Variable speed per star: shift 5-8 gives cycle times of ~8-16 seconds
    uint8_t speedDiv = 5 + (starHash(i, 21) % 4);
    uint32_t phase = (time >> speedDiv) + (i * 7919u);
    return (uint8_t)(phase & 0xFF);
}

// ============================================================================
// ConstellationEffect implementation
// ============================================================================

ConstellationEffect::ConstellationEffect(Matrix* matrix, TOFSensor* sensor)
    : Effect(matrix), tofSensor(sensor), starCount(0) {

    screenWidth = m_matrix->getXResolution();
    screenHeight = m_matrix->getYResolution();

    tofGridReady = false;
    minDetectionDistance = TOF_MIN_DETECTION_DIST;
    maxDetectionDistance = TOF_MAX_DETECTION_DIST;

    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            tofGrid[y][x] = 0;
            depthField[y][x] = 0;
        }
    }

    for (uint8_t i = 0; i < MAX_SHOOTING_STARS; i++) {
        shootingStars[i].active = false;
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

void ConstellationEffect::initStars() {
    starCount = MAX_STARS;

    for (uint16_t i = 0; i < starCount; i++) {
        px[i] = (uint16_t)random(0, screenWidth);
        py[i] = (uint16_t)random(0, screenHeight);
        brightness[i] = 0;
        currentHue[i] = getBaseHue(i);
        currentSat[i] = getBaseSaturation(i);
    }
}

// ============================================================================
// TOF sensor
// ============================================================================

void ConstellationEffect::rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY) {
    // Effect-only 8x8 remap (easy to tune by degrees).
    TOFSensor::rotateGrid8x8(x, y, kTofEffectRotationDeg, outX, outY);
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
            uint8_t dx, dy;
            tofSensor->toDisplayAligned(rx, ry, dx, dy);
            tofGrid[y][x] = tofSensor->getDistance(dx, dy);
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

    int16_t range = maxDetectionDistance - minDetectionDistance;
    if (range <= 0) range = 1;

    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            int16_t depth = tofGrid[y][x];
            float value = 0;
            if (depth > minDetectionDistance && depth < maxDetectionDistance) {
                // Gradient: closer = stronger (1.0 at min, 0.0 at max)
                value = 1.0f - (float)(depth - minDetectionDistance) / (float)range;
                if (value < 0) value = 0;
                if (value > 1.0f) value = 1.0f;
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

// ============================================================================
// Star update and draw
// ============================================================================

void ConstellationEffect::updateStars() {
    constexpr uint8_t BOOST_THRESHOLD = 13;  // ~0.05 * 255
    constexpr uint16_t BOOST_SCALE = 255 - BOOST_THRESHOLD;

    uint32_t now = millis();

    for (uint16_t i = 0; i < starCount; i++) {
        float depthBoost = getDepthBoostAt((float)px[i], (float)py[i]);
        uint16_t depthBoost255 = (uint16_t)(depthBoost * 255);

        // Twinkle: SIN_TABLE modulates base brightness
        uint8_t twinkleMul = SIN_TABLE[getTwinklePhase(i, now)];
        uint8_t baseBright = getBaseBrightness(i);
        int16_t targetBright = (int16_t)((baseBright * (int)twinkleMul) >> 8);

        // Depth boost: gently ramp up when hand detected
        if (depthBoost255 > BOOST_THRESHOLD) {
            uint16_t norm = ((uint16_t)depthBoost255 - BOOST_THRESHOLD) * 255 / BOOST_SCALE;
            // Gentle boost: blend toward 200 (not full 255) for a warm glow, not a blast
            targetBright = targetBright + ((200 - targetBright) * (int)norm >> 8);
        }

        if (targetBright < 0) targetBright = 0;
        else if (targetBright > 255) targetBright = 255;

        // Smooth brightness: fast rise, slow fade
        int16_t diff = (int16_t)targetBright - (int16_t)brightness[i];
        if (diff > 0) {
            int16_t rise = max(1, diff >> BRIGHTNESS_RISE_SHIFT);
            brightness[i] = (uint8_t)min(255, (int)brightness[i] + rise);
        } else if (diff < 0) {
            int16_t fade = min(-1, diff >> BRIGHTNESS_FADE_SHIFT);
            brightness[i] = (uint8_t)max(0, (int)brightness[i] + fade);
        }

        // Hue: shift toward warm gold when hand detected, back to base when not
        uint8_t baseH = getBaseHue(i);
        uint8_t targetHue = (depthBoost255 > BOOST_THRESHOLD) ? HUE_ACTIVATED : baseH;

        int16_t hueDiff = (int16_t)targetHue - (int16_t)currentHue[i];
        if (hueDiff > 0) {
            currentHue[i] = (uint8_t)min(255, (int)currentHue[i] + max(1, hueDiff >> BRIGHTNESS_RISE_SHIFT));
        } else if (hueDiff < 0) {
            currentHue[i] = (uint8_t)max(0, (int)currentHue[i] + min(-1, hueDiff >> BRIGHTNESS_FADE_SHIFT));
        }

        // Saturation: increase when activated for warmer color
        uint8_t baseSat = getBaseSaturation(i);
        uint8_t targetSat = (depthBoost255 > BOOST_THRESHOLD) ? SAT_ACTIVATED : baseSat;

        int16_t satDiff = (int16_t)targetSat - (int16_t)currentSat[i];
        if (satDiff > 0) {
            currentSat[i] = (uint8_t)min(255, (int)currentSat[i] + max(1, satDiff >> BRIGHTNESS_RISE_SHIFT));
        } else if (satDiff < 0) {
            currentSat[i] = (uint8_t)max(0, (int)currentSat[i] + min(-1, satDiff >> BRIGHTNESS_FADE_SHIFT));
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
        hsv2rgb_rainbow(CHSV(currentHue[i], currentSat[i], brightness[i]), color);
        m_matrix->background->drawPixel(x, y, color);
    }
}

// ============================================================================
// Shooting stars
// ============================================================================

void ConstellationEffect::spawnShootingStar() {
    for (uint8_t i = 0; i < MAX_SHOOTING_STARS; i++) {
        if (!shootingStars[i].active) {
            shootingStars[i].active = true;
            shootingStars[i].x = screenWidth - 1;
            shootingStars[i].y = random(2, screenHeight - 2);
            shootingStars[i].dx = -(2 + random(0, 3));   // -2 to -4 px/frame
            shootingStars[i].dy = random(0, 3) - 1;      // -1, 0, or 1
            shootingStars[i].length = 8 + random(0, 13);  // 8-20 pixels
            shootingStars[i].life = screenWidth / 2 + random(0, screenWidth / 2);
            return;
        }
    }
}

void ConstellationEffect::updateShootingStars() {
    // Chance to spawn
    if (random(0, SHOOT_SPAWN_CHANCE) == 0) {
        spawnShootingStar();
    }

    for (uint8_t i = 0; i < MAX_SHOOTING_STARS; i++) {
        if (!shootingStars[i].active) continue;

        shootingStars[i].x += shootingStars[i].dx;
        shootingStars[i].y += shootingStars[i].dy;
        shootingStars[i].life--;

        // Deactivate if off-screen or life expired
        if (shootingStars[i].x < -shootingStars[i].length ||
            shootingStars[i].y < 0 || shootingStars[i].y >= screenHeight ||
            shootingStars[i].life == 0) {
            shootingStars[i].active = false;
        }
    }
}

void ConstellationEffect::drawShootingStars() {
    for (uint8_t i = 0; i < MAX_SHOOTING_STARS; i++) {
        if (!shootingStars[i].active) continue;

        int16_t hx = shootingStars[i].x;
        int16_t hy = shootingStars[i].y;
        uint8_t len = shootingStars[i].length;
        int8_t dx = shootingStars[i].dx;
        int8_t dy = shootingStars[i].dy;

        // Draw trail from head backward (opposite of movement direction)
        for (uint8_t p = 0; p < len; p++) {
            int16_t tx = hx - dx * p / abs(dx);  // step back along trail
            // For diagonal trails, interpolate y
            int16_t ty = hy;
            if (dy != 0 && dx != 0) {
                ty = hy - dy * p / abs(dx);
            }

            if (tx < 0 || tx >= screenWidth || ty < 0 || ty >= screenHeight) continue;

            // Linear brightness falloff along trail
            uint8_t trailBright = 255 - (uint16_t)p * 255 / len;
            // Slightly warm white for shooting stars
            CRGB color;
            hsv2rgb_rainbow(CHSV(30, 15, trailBright), color);
            m_matrix->background->drawPixel(tx, ty, color);
        }
    }
}

// ============================================================================
// Main update
// ============================================================================

void ConstellationEffect::update() {
    updateTofData();
    buildDepthField();

    m_matrix->background->fillScreen(0);

    updateStars();
    drawStars();

    updateShootingStars();
    drawShootingStars();
}
