#include "MeteorShowerEffect.h"
#include "../TOFSensor/TOFSensor.h"

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
    size = 2.0f + randomFloat() * 4.0f;
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
    
    for (uint8_t i = 0; i < count; i++) {
        if (i == selfIndex) continue;
        
        float dx = position.x - meteors[i].position.x;
        float dy = position.y - meteors[i].position.y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        if (distance > 0 && distance < separationDistance) {
            float strength = ((separationDistance - distance) / separationDistance) * separationStrength;
            // Normalize and scale
            float invDist = 1.0f / distance;
            steering.x += dx * invDist * strength;
            steering.y += dy * invDist * strength;
            total++;
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
    
    size = 8 + Meteor::randomFloat() * 12;  // 8-20 pixels for 64x64
    hue = 120 + (uint8_t)(Meteor::randomFloat() * 80);  // Blue to purple hues
    hasRings = Meteor::randomFloat() > 0.5f;
    ringRotation = Meteor::randomFloat() * 6.28f;
}

void Planet::update() {
    pos.x += vel.x;
    pos.y += vel.y;
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
    baseMeteorSpeed = 1.5f;
    boostedMeteorSpeed = 3.0f;
    currentMeteorSpeed = baseMeteorSpeed;
    baseSpawnRate = 2;
    boostedSpawnRate = 1;
    currentSpawnRate = baseSpawnRate;
    baseMaxMeteors = 40;
    boostedMaxMeteors = 55;
    currentMaxMeteors = baseMaxMeteors;
    
    // Motion parameters (scaled for matrix resolution)
    flowCorrectionStrength = 0.05f;
    wobbleStrength = 0.1f;
    wobbleSpeed = 0.004f;
    separationDistance = 8.0f;  // Scaled for 64px
    separationStrength = 0.5f;
    maxVerticalSpeed = 1.5f;
    trailFadeAmount = 20;
    attractorStrength = 0.08f;
    attractorRadius = 20.0f;
    centerAttractorStrength = 0.15f;
    centerAttractorRadius = 40.0f;
    forwardAcceleration = 0.02f;
    
    // ToF parameters
    tofGridReady = false;
    minDetectionDistance = 1000;
    maxDetectionDistance = 2000;
    tofRotation = 180;
    topRowActive = false;
    
    frameCount = 0;
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
    
    initMeteors();
    initTofAttractors();
    initPlanets();
    
    m_matrix->background->fillScreen(0);
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

void MeteorShowerEffect::setTofRotation(uint8_t rotation) {
    tofRotation = rotation;
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
            tofGrid[y][x] = tofSensor->getDistance(rx, ry);
        }
    }
    tofGridReady = true;
}

void MeteorShowerEffect::updateTofAttractors() {
    if (!tofGridReady) return;
    
    topRowActive = false;
    
    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            int16_t depth = tofGrid[y][x];
            bool isActive = depth > minDetectionDistance && depth < maxDetectionDistance;
            tofAttractors[y][x].active = isActive;
            
            // Check if top row (x == 7 after rotation means "top" in the original p5.js)
            if (isActive && x == 7) {
                topRowActive = true;
            }
        }
    }
}

void MeteorShowerEffect::spawnMeteor() {
    if (meteorCount >= currentMaxMeteors) return;
    
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();
    
    float x = width;
    float y = Meteor::randomFloat() * height;
    
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
                if (tofAttractors[y][x].active && activeCount < 64) {
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
        m->applySeparation(meteors, meteorCount, i, separationDistance, separationStrength);
        
        // Apply ToF attractors
        for (uint8_t j = 0; j < activeCount; j++) {
            m->applyAttractor(activeAttractors[j], attractorStrength, attractorRadius);
        }
        
        // If no ToF attractors active, use center attractor for Y-axis
        if (activeCount == 0) {
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
    
    // Deep space gradient - purple to blue from top to bottom
    for (uint8_t y = 0; y < height; y++) {
        float gradientFactor = (float)y / height;
        
        // HSV: purple (260) to blue (220), increasing saturation and brightness
        uint8_t hue = 170 - (uint8_t)(gradientFactor * 25);  // ~170 to ~145 in FastLED
        uint8_t sat = 100 + (uint8_t)(gradientFactor * 75);
        uint8_t val = 20 + (uint8_t)(gradientFactor * 40);
        
        CRGB color;
        hsv2rgb_rainbow(CHSV(hue, sat, val), color);
        
        for (uint8_t x = 0; x < width; x++) {
            m_matrix->background->drawPixel(x, y, color);
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
        hsv2rgb_rainbow(CHSV(p.hue, 50, 50), glowColor);
        glowColor.nscale8(50);  // Dim the glow
        m_matrix->background->fillCircle(x, y, r + 2, m_matrix->background->color565(glowColor.r, glowColor.g, glowColor.b));
        
        // Planet body
        CRGB bodyColor;
        hsv2rgb_rainbow(CHSV(p.hue, 100, 80), bodyColor);
        bodyColor.nscale8(150);  // Dim distant planets
        m_matrix->background->fillCircle(x, y, r, m_matrix->background->color565(bodyColor.r, bodyColor.g, bodyColor.b));
        
        // Highlight
        CRGB highlightColor;
        hsv2rgb_rainbow(CHSV(p.hue, 50, 150), highlightColor);
        int16_t hx = x - r / 3;
        int16_t hy = y - r / 3;
        int16_t hr = r / 3;
        if (hr > 0) {
            m_matrix->background->fillCircle(hx, hy, hr, m_matrix->background->color565(highlightColor.r, highlightColor.g, highlightColor.b));
        }
    }
}

void MeteorShowerEffect::drawMeteors() {
    for (uint8_t i = 0; i < meteorCount; i++) {
        Meteor& m = meteors[i];
        
        CRGB color;
        hsv2rgb_rainbow(CHSV(m.hue, m.saturation, m.brightness), color);
        
        int16_t x = (int16_t)m.position.x;
        int16_t y = (int16_t)m.position.y;
        int16_t r = (int16_t)(m.size / 2);
        
        if (r < 1) r = 1;
        
        if (r <= 1) {
            m_matrix->background->drawPixel(x, y, color);
        } else {
            m_matrix->background->fillCircle(x, y, r, m_matrix->background->color565(color.r, color.g, color.b));
        }
    }
}

void MeteorShowerEffect::fadeTrails() {
    m_matrix->background->dim(255 - trailFadeAmount);
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
    
    // Apply fade for meteor trails
    fadeTrails();
    
    // Update and draw meteors
    updateMeteors();
    drawMeteors();
}
