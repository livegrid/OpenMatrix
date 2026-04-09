#include "MeteorShowerEffect.h"
#include "../TOFSensor/TOFSensor.h"

namespace {
constexpr int16_t kTofEffectRotationDeg = 90;  // Set effect remap here: 0/90/180/270
}

// Simple pseudo-random for consistent behavior
static uint32_t noiseSeed = 12345;

// ============================================================================
// Meteor class implementation
// ============================================================================

Meteor::Meteor() {
    position.set(0, 0);
    velocity.set(0, 0);
    acceleration.set(0, 0);
    speedMultiplier = 1.0f;
    size = 3.0f;
    noiseOffset = 0.0f;
    hue = 30;
    saturation = 80;
    brightness = 90;
}

void Meteor::init(float x, float y, float targetSpeed) {
    position.set(x, y);
    speedMultiplier = 0.9f + randomFloat() * 0.3f;
    velocity.set(-targetSpeed * speedMultiplier, randomFloat() - 0.5f);
    acceleration.set(0, 0);
    size = 1.8f + randomFloat() * 3.2f;
    noiseOffset = randomFloat() * 1000.0f;
    
    // Black hole accretion disk color palette: warm yellows, oranges, and reds
    float colorType = randomFloat();
    if (colorType < 0.3f) {
        hue = 15 + (uint8_t)(randomFloat() * 15);  // Bright yellow/orange (HSV ~30-45)
        saturation = 180 + (uint8_t)(randomFloat() * 50);
        brightness = 220 + (uint8_t)(randomFloat() * 35);
    } else if (colorType < 0.7f) {
        hue = 8 + (uint8_t)(randomFloat() * 12);   // Warm orange (HSV ~15-40)
        saturation = 200 + (uint8_t)(randomFloat() * 55);
        brightness = 190 + (uint8_t)(randomFloat() * 50);
    } else {
        hue = 0 + (uint8_t)(randomFloat() * 12);   // Deeper reds/oranges (HSV ~0-25)
        saturation = 220 + (uint8_t)(randomFloat() * 35);
        brightness = 150 + (uint8_t)(randomFloat() * 60);
    }
}

float Meteor::randomFloat() {
    noiseSeed = noiseSeed * 1103515245 + 12345;
    return (float)(noiseSeed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

float Meteor::noise(float x) {
    // Simple value noise implementation
    int xi = (int)x;
    float xf = x - xi;
    
    // Hash function
    noiseSeed = (xi * 1103515245 + 12345);
    float a = (float)(noiseSeed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
    noiseSeed = ((xi + 1) * 1103515245 + 12345);
    float b = (float)(noiseSeed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
    
    // Smooth interpolation
    float t = xf * xf * (3.0f - 2.0f * xf);
    return a + t * (b - a);
}

void Meteor::resetForces() {
    acceleration.set(0, 0);
}

void Meteor::applyBaseFlow(float targetSpeed, float flowCorrectionStrength, float forwardAcceleration) {
    float desiredX = -targetSpeed * speedMultiplier;
    float correctionX = (desiredX - velocity.x) * flowCorrectionStrength;
    float correctionY = (0 - velocity.y) * flowCorrectionStrength;
    acceleration.x += correctionX - forwardAcceleration;
    acceleration.y += correctionY;
}

void Meteor::applyWobble(uint32_t time, float wobbleStrength, float wobbleSpeed) {
    float noiseValue = noise(noiseOffset + time * wobbleSpeed);
    float wobble = (noiseValue - 0.5f) * 2.0f * wobbleStrength;
    acceleration.y += wobble;
}

void Meteor::applySeparation(Meteor* meteors, uint8_t count, uint8_t selfIndex, float separationDistance, float separationStrength) {
    PVector steering;
    steering.set(0, 0);
    int total = 0;
    const float separationDistanceSq = separationDistance * separationDistance;
    const uint8_t maxNeighbors = 8;
    
    for (uint8_t i = 0; i < count; i++) {
        if (i == selfIndex) continue;
        
        float dx = position.x - meteors[i].position.x;
        float dy = position.y - meteors[i].position.y;
        float distanceSq = dx * dx + dy * dy;
        
        if (distanceSq > 0.0001f && distanceSq < separationDistanceSq) {
            float distance = sqrtf(distanceSq);
            float strength = ((separationDistance - distance) / separationDistance) * separationStrength;
            // Normalize and scale
            float invDist = 1.0f / distance;
            steering.x += dx * invDist * strength;
            steering.y += dy * invDist * strength;
            total++;
            if (total >= maxNeighbors) break;
        }
    }
    
    if (total > 0) {
        steering.x /= total;
        steering.y /= total;
        acceleration.x += steering.x;
        acceleration.y += steering.y;
    }
}

void Meteor::applyAttractor(PVector& attractorPos, float strength, float radius) {
    float dx = attractorPos.x - position.x;
    float dy = attractorPos.y - position.y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    if (distance < radius && distance > 0) {
        float invDist = 1.0f / distance;
        float forceStrength = strength * (1.0f - distance / radius);
        acceleration.x += dx * invDist * forceStrength;
        acceleration.y += dy * invDist * forceStrength;
    }
}

void Meteor::applyAttractorY(PVector& attractorPos, float strength, float radius) {
    float dx = attractorPos.x - position.x;
    float dy = attractorPos.y - position.y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    if (distance < radius && distance > 0) {
        float invDist = 1.0f / distance;
        float forceStrength = strength * (1.0f - distance / radius);
        // Only apply Y component
        acceleration.y += dy * invDist * forceStrength;
    }
}

void Meteor::update(float targetSpeed, float maxVerticalSpeed) {
    velocity.x += acceleration.x;
    velocity.y += acceleration.y;
    
    // Ensure meteors keep moving forward (left)
    float minSpeed = -targetSpeed * speedMultiplier * 0.6f;
    if (velocity.x > minSpeed) velocity.x = minSpeed;
    
    // Limit overall speed
    float maxSpeed = targetSpeed * speedMultiplier * 1.8f;
    velocity.limit(maxSpeed);
    
    // Constrain vertical speed
    if (velocity.y < -maxVerticalSpeed) velocity.y = -maxVerticalSpeed;
    if (velocity.y > maxVerticalSpeed) velocity.y = maxVerticalSpeed;
    
    position.x += velocity.x;
    position.y += velocity.y;
    
    acceleration.set(0, 0);
}

bool Meteor::isOffScreen() {
    return position.x < -size;
}

// ============================================================================
// Planet class implementation
// ============================================================================

Planet::Planet() {
    pos.set(0, 0);
    vel.set(0, 0);
    size = 10;
    hue = 160;
    hasRings = false;
    ringRotation = 0;
}

void Planet::spawn(uint8_t maxWidth, uint8_t maxHeight) {
    bool fromTop = Meteor::randomFloat() > 0.5f;
    
    if (fromTop) {
        pos.x = maxWidth * 0.2f + Meteor::randomFloat() * maxWidth * 0.6f;
        pos.y = -15;
        vel.x = (Meteor::randomFloat() - 0.5f) * 0.2f;
        vel.y = 0.1f + Meteor::randomFloat() * 0.15f;
    } else {
        pos.x = Meteor::randomFloat() > 0.5f ? -15 : maxWidth + 15;
        pos.y = maxHeight * 0.2f + Meteor::randomFloat() * maxHeight * 0.6f;
        vel.x = pos.x < 0 ? (0.1f + Meteor::randomFloat() * 0.15f) : (-0.1f - Meteor::randomFloat() * 0.15f);
        vel.y = (Meteor::randomFloat() - 0.5f) * 0.1f;
    }
    
    size = 10 + Meteor::randomFloat() * 14;  // 10-24 pixels for 64x64
    hue = 120 + (uint8_t)(Meteor::randomFloat() * 80);  // Blue to purple hues
    hasRings = Meteor::randomFloat() > 0.5f;
    ringRotation = Meteor::randomFloat() * 6.28f;
}

void Planet::update() {
    pos.x += vel.x;
    pos.y += vel.y;
    ringRotation += 0.01f;
}

bool Planet::isOffScreen(uint8_t maxWidth, uint8_t maxHeight) {
    return pos.y > maxHeight + 25 || pos.x < -25 || pos.x > maxWidth + 25 || pos.y < -25;
}

// ============================================================================
// MeteorShowerEffect class implementation
// ============================================================================

MeteorShowerEffect::MeteorShowerEffect(Matrix* matrix, TOFSensor* sensor) 
    : Effect(matrix), tofSensor(sensor) {
    
    // Initialize parameters
    baseMeteorSpeed = 1.35f;
    boostedMeteorSpeed = 2.4f;
    currentMeteorSpeed = baseMeteorSpeed;
    baseSpawnRate = 3;
    boostedSpawnRate = 2;
    currentSpawnRate = baseSpawnRate;
    baseSpawnBurst = 1;
    boostedSpawnBurst = 1;
    currentSpawnBurst = baseSpawnBurst;
    baseMaxMeteors = 45;
    boostedMaxMeteors = 60;
    currentMaxMeteors = baseMaxMeteors;
    
    // Motion parameters (scaled for matrix resolution)
    flowCorrectionStrength = 0.06f;
    wobbleStrength = 0.12f;
    wobbleSpeed = 0.004f;
    separationDistance = 10.0f;
    separationStrength = 0.6f;
    maxVerticalSpeed = 1.8f;
    trailFadeAmount = 12;
    attractorStrength = 0.1f;
    attractorRadius = 22.0f;
    centerAttractorStrength = 0.0f;
    centerAttractorRadius = 48.0f;
    forwardAcceleration = 0.03f;
    maxActiveAttractors = 14;
    maxSeparationNeighbors = 8;
    trailPersistence = 180;
    meteorCompositeThreshold = 5;
    
    // ToF parameters
    tofGridReady = false;
    minDetectionDistance = TOF_MIN_DETECTION_DIST;
    maxDetectionDistance = TOF_MAX_DETECTION_DIST;
    topRowActive = false;
    
    frameCount = 0;
    rotateEffect180 = true;
    spawnHistoryIndex = 0;
    for (uint8_t i = 0; i < 4; i++) {
        recentSpawnY[i] = -1000.0f;
    }
    meteorCount = 0;
    spawnCounter = 0;
    planetCount = 0;
    planetSpawnCounter = 0;
    
    // Initialize ToF grid
    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            tofGrid[y][x] = 0;
        }
    }
    
    reset();
}

void MeteorShowerEffect::reset() {
    meteorCount = 0;
    spawnCounter = 0;
    planetCount = 0;
    planetSpawnCounter = 0;
    frameCount = 0;
    spawnHistoryIndex = 0;
    for (uint8_t i = 0; i < 4; i++) {
        recentSpawnY[i] = -1000.0f;
    }
    
    initMeteors();
    initTofAttractors();
    initPlanets();
    
    m_matrix->background->fillScreen(0);
    if (m_matrix->foreground) {
        m_matrix->foreground->clear();
    }
}

const char* MeteorShowerEffect::getName() const {
    return "MeteorShower";
}

void MeteorShowerEffect::setTofSensor(TOFSensor* sensor) {
    tofSensor = sensor;
}

void MeteorShowerEffect::setDetectionRange(int16_t minDist, int16_t maxDist) {
    minDetectionDistance = minDist;
    maxDetectionDistance = maxDist;
}

void MeteorShowerEffect::initMeteors() {
    meteorCount = 0;
    // Start with a few meteors
    for (uint8_t i = 0; i < 10 && i < MAX_METEORS; i++) {
        spawnMeteor();
    }
}

void MeteorShowerEffect::initTofAttractors() {
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();
    float cellW = (float)width / TOF_GRID_SIZE;
    float cellH = (float)height / TOF_GRID_SIZE;
    
    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            tofAttractors[y][x].position.set(x * cellW + cellW / 2, y * cellH + cellH / 2);
            tofAttractors[y][x].active = false;
        }
    }
}

void MeteorShowerEffect::initPlanets() {
    planetCount = 0;
    planetSpawnCounter = 0;
    
    // Start with 1-2 planets
    uint8_t numPlanets = 1 + (Meteor::randomFloat() > 0.5f ? 1 : 0);
    for (uint8_t i = 0; i < numPlanets && i < MAX_PLANETS; i++) {
        spawnPlanet();
    }
}

void MeteorShowerEffect::rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY) {
    // Effect-only 8x8 remap (easy to tune by degrees).
    TOFSensor::rotateGrid8x8(x, y, kTofEffectRotationDeg, outX, outY);
}

void MeteorShowerEffect::updateTofData() {
    if (!tofSensor || !tofSensor->isActive()) {
        tofGridReady = false;
        return;
    }
    
    // Read data from sensor
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

void MeteorShowerEffect::updateTofAttractors() {
    if (!tofGridReady) {
        topRowActive = false;
        for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
            for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
                tofAttractors[y][x].active = false;
            }
        }
        return;
    }

    topRowActive = false;
    uint8_t topRowHitCount = 0;
    
    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            int16_t depth = tofGrid[y][x];
            bool isActive = depth > minDetectionDistance && depth < maxDetectionDistance;
            tofAttractors[y][x].active = isActive;
            
            // Boost when the high-x edge of the effect 8x8 grid is active (p5.js port convention).
            if (isActive && x == 7) {
                topRowHitCount++;
            }
        }
    }

    // Require at least 2 active cells to enter boosted stream mode.
    topRowActive = topRowHitCount >= 2;
}

void MeteorShowerEffect::spawnMeteor() {
    if (meteorCount >= currentMaxMeteors) return;
    
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();

    // Natural stream: always start outside the right edge.
    float x = (float)width + 1.5f + Meteor::randomFloat() * 3.0f;
    float y = Meteor::randomFloat() * (float)height;

    // Anti-clump spawn distribution: keep recent spawn Y positions separated.
    const float minGap = (float)height * 0.14f;
    for (uint8_t attempt = 0; attempt < 6; attempt++) {
        float candidateY = Meteor::randomFloat() * (float)height;
        bool tooClose = false;
        for (uint8_t i = 0; i < 4; i++) {
            if (fabsf(candidateY - recentSpawnY[i]) < minGap) {
                tooClose = true;
                break;
            }
        }
        if (!tooClose) {
            y = candidateY;
            break;
        }
    }
    recentSpawnY[spawnHistoryIndex] = y;
    spawnHistoryIndex = (spawnHistoryIndex + 1) & 0x03;
    
    meteors[meteorCount].init(x, y, currentMeteorSpeed);
    meteorCount++;
}

void MeteorShowerEffect::spawnPlanet() {
    if (planetCount >= MAX_PLANETS) return;
    
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();
    
    planets[planetCount].spawn(width, height);
    planetCount++;
}

void MeteorShowerEffect::updateMeteors() {
    // Adjust speed and spawn rate based on ToF interaction
    if (topRowActive) {
        currentMeteorSpeed = boostedMeteorSpeed;
        currentSpawnRate = boostedSpawnRate;
        currentMaxMeteors = boostedMaxMeteors;
    } else {
        currentMeteorSpeed = baseMeteorSpeed;
        currentSpawnRate = baseSpawnRate;
        currentMaxMeteors = baseMaxMeteors;
    }
    
    // Spawn new meteors
    spawnCounter++;
    if (spawnCounter >= currentSpawnRate) {
        spawnMeteor();
        spawnCounter = 0;
    }
    
    // Collect active ToF attractor positions
    PVector activeAttractors[64];
    uint8_t activeCount = 0;
    
    if (tofGridReady) {
        for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
            for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
                if (tofAttractors[y][x].active && activeCount < maxActiveAttractors) {
                    activeAttractors[activeCount] = tofAttractors[y][x].position;
                    activeCount++;
                }
            }
        }
    }
    
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();
    PVector centerAttractor;
    centerAttractor.set(width * 2.0f / 3.0f, height / 2.0f);
    
    // Update each meteor
    for (int i = meteorCount - 1; i >= 0; i--) {
        Meteor* m = &meteors[i];
        
        m->resetForces();
        m->applyBaseFlow(currentMeteorSpeed, flowCorrectionStrength, forwardAcceleration);
        m->applyWobble(frameCount, wobbleStrength, wobbleSpeed);
        if (maxSeparationNeighbors > 0 && meteorCount > 1) {
            m->applySeparation(meteors, meteorCount, i, separationDistance, separationStrength);
        }
        
        // Apply ToF attractors
        for (uint8_t j = 0; j < activeCount; j++) {
            m->applyAttractor(activeAttractors[j], attractorStrength, attractorRadius);
        }
        
        // If no ToF attractors active, use center attractor for Y-axis
        if (activeCount == 0 && centerAttractorStrength > 0.001f) {
            m->applyAttractorY(centerAttractor, centerAttractorStrength, centerAttractorRadius);
        }
        
        m->update(currentMeteorSpeed, maxVerticalSpeed);
        
        // Remove off-screen meteors
        if (m->isOffScreen()) {
            // Swap with last meteor and decrease count
            if (i < meteorCount - 1) {
                meteors[i] = meteors[meteorCount - 1];
            }
            meteorCount--;
        }
    }
}

void MeteorShowerEffect::updatePlanets() {
    // Spawn planets periodically
    planetSpawnCounter++;
    if (planetSpawnCounter >= PLANET_SPAWN_INTERVAL && planetCount < MAX_PLANETS) {
        spawnPlanet();
        planetSpawnCounter = 0;
    }
    
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();
    
    // Update and remove off-screen planets
    for (int i = planetCount - 1; i >= 0; i--) {
        planets[i].update();
        
        if (planets[i].isOffScreen(width, height)) {
            if (i < planetCount - 1) {
                planets[i] = planets[planetCount - 1];
            }
            planetCount--;
        }
    }
}

void MeteorShowerEffect::drawGradientBackground() {
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();
    
    // Deep space gradient with subtle horizontal nebula variation.
    for (uint8_t y = 0; y < height; y++) {
        float gradientFactor = (float)y / height;
        for (uint8_t x = 0; x < width; x++) {
            float wave = 0.5f + 0.5f * sinf(0.19f * x + 0.11f * y + frameCount * 0.015f);
            uint8_t hue = 176 - (uint8_t)(gradientFactor * 32) + (uint8_t)(wave * 3.0f);
            uint8_t sat = 80 + (uint8_t)(gradientFactor * 130) + (uint8_t)(wave * 20.0f);
            uint8_t val = 8 + (uint8_t)(gradientFactor * 67) + (uint8_t)(wave * 12.0f);

            CRGB color;
            hsv2rgb_rainbow(CHSV(hue, sat, val), color);
            drawEffectPixel(x, y, color);
        }
    }
}

void MeteorShowerEffect::drawPlanets() {
    for (uint8_t i = 0; i < planetCount; i++) {
        Planet& p = planets[i];
        int16_t x = (int16_t)p.pos.x;
        int16_t y = (int16_t)p.pos.y;
        int16_t r = (int16_t)(p.size / 2);
        
        // Planet glow (outer)
        CRGB glowColor;
        hsv2rgb_rainbow(CHSV(p.hue, 45, 70), glowColor);
        glowColor.nscale8(65);
        drawEffectCircle(x, y, r + 2, glowColor);
        
        // Planet body
        CRGB bodyColor;
        hsv2rgb_rainbow(CHSV(p.hue, 100, 80), bodyColor);
        bodyColor.nscale8(155);
        drawEffectCircle(x, y, r, bodyColor);

        if (p.hasRings && r > 2) {
            CRGB ringColor;
            hsv2rgb_rainbow(CHSV(p.hue + 10, 100, 95), ringColor);
            ringColor.nscale8(120);
            float ringCos = cosf(p.ringRotation);
            float ringSin = sinf(p.ringRotation);
            float rx = r + 3.0f;
            float ry = r * 0.45f;
            for (float t = 0; t < 6.28318f; t += 0.28f) {
                float ex = cosf(t) * rx;
                float ey = sinf(t) * ry;
                int16_t px = (int16_t)(x + ex * ringCos - ey * ringSin);
                int16_t py = (int16_t)(y + ex * ringSin + ey * ringCos);
                drawEffectPixel(px, py, ringColor);
            }
        }
        
        // Highlight
        CRGB highlightColor;
        hsv2rgb_rainbow(CHSV(p.hue, 40, 170), highlightColor);
        int16_t hx = x - r / 3;
        int16_t hy = y - r / 3;
        int16_t hr = r / 3;
        if (hr > 0) {
            drawEffectCircle(hx, hy, hr, highlightColor);
        }
    }
}

void MeteorShowerEffect::drawMeteors() {
    for (uint8_t i = 0; i < meteorCount; i++) {
        Meteor& m = meteors[i];
        
        CRGB headColor;
        hsv2rgb_rainbow(CHSV(m.hue, m.saturation, m.brightness), headColor);
        
        int16_t x = (int16_t)m.position.x;
        int16_t y = (int16_t)m.position.y;
        int16_t radius = (int16_t)constrain((int)(m.size * 0.35f), 1, 2);

        // Soft glow + core drawn on foreground layer. Trails emerge from persistence.
        CRGB glowColor = headColor;
        glowColor.nscale8_video(110);
        drawLayerCircle(m_matrix->foreground, x, y, radius + 1, glowColor);
        drawLayerCircle(m_matrix->foreground, x, y, radius, headColor);

        CRGB coreColor = headColor;
        coreColor.nscale8_video(245);
        drawLayerPixel(m_matrix->foreground, x, y, coreColor);
    }
}

void MeteorShowerEffect::fadeTrails() {
    m_matrix->background->dim(255 - trailFadeAmount);
}

void MeteorShowerEffect::transformEffectCoordinate(int16_t inX, int16_t inY, int16_t& outX, int16_t& outY) const {
    if (!rotateEffect180) {
        outX = inX;
        outY = inY;
        return;
    }

    int16_t width = m_matrix->getXResolution();
    int16_t height = m_matrix->getYResolution();
    outX = (width - 1) - inX;
    outY = (height - 1) - inY;
}

void MeteorShowerEffect::drawEffectPixel(int16_t x, int16_t y, const CRGB& color) {
    drawLayerPixel(m_matrix->background, x, y, color);
}

void MeteorShowerEffect::drawLayerPixel(GFX_Layer* layer, int16_t x, int16_t y, const CRGB& color) {
    if (!layer) return;
    int16_t width = m_matrix->getXResolution();
    int16_t height = m_matrix->getYResolution();
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }

    int16_t tx, ty;
    transformEffectCoordinate(x, y, tx, ty);
    layer->drawPixel(tx, ty, color);
}

void MeteorShowerEffect::drawEffectCircle(int16_t x, int16_t y, int16_t r, const CRGB& color) {
    drawLayerCircle(m_matrix->background, x, y, r, color);
}

void MeteorShowerEffect::drawLayerCircle(GFX_Layer* layer, int16_t x, int16_t y, int16_t r, const CRGB& color) {
    if (!layer) return;
    int16_t tx, ty;
    transformEffectCoordinate(x, y, tx, ty);
    layer->fillCircle(tx, ty, r, layer->color565(color.r, color.g, color.b));
}

void MeteorShowerEffect::update() {
    frameCount++;
    
    // Update ToF sensor data
    updateTofData();
    updateTofAttractors();
    
    // Draw gradient background first (this creates the base, erasing previous frame)
    drawGradientBackground();
    
    // Update and draw planets (behind meteors)
    updatePlanets();
    drawPlanets();
    
    // Update meteors and render them on the foreground persistence layer.
    updateMeteors();
    if (m_matrix->foreground && m_matrix->gfx_compositor) {
        m_matrix->foreground->dim(trailPersistence);
    }
    drawMeteors();
    if (m_matrix->foreground && m_matrix->gfx_compositor) {
        m_matrix->gfx_compositor->StackWithThreshold(
            *m_matrix->background,
            *m_matrix->foreground,
            meteorCompositeThreshold,
            true
        );
    }
}
