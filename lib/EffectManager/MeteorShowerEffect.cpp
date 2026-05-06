#include "MeteorShowerEffect.h"
#include "../TOFSensor/TOFSensor.h"

namespace {
/** Digital orientation tweak for this effect vs global TOF frame (after physical rotation in TOFSensor). */
constexpr int16_t kTofEffectRotationDeg = 270;  // 0 / 90 / 180 / 270

static int16_t depthAtEffectCell(const InteractionData& d, TOFSensor& sensor, uint8_t effectX, uint8_t effectY) {
    uint8_t rx, ry;
    TOFSensor::rotateGrid8x8(effectX, effectY, kTofEffectRotationDeg, rx, ry);
    uint8_t gx, gy;
    sensor.toDisplayAligned(rx, ry, gx, gy);
    return d.depthMap[gy][gx];
}
}  // namespace

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
    speedMultiplier = 0.85f + randomFloat() * 0.45f;
    velocity.set(-targetSpeed * speedMultiplier, (randomFloat() - 0.5f) * 0.5f);
    acceleration.set(0, 0);
    size = 0.85f + randomFloat() * 1.5f;
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

void Meteor::applySeparation(Meteor* meteors, uint8_t count, uint8_t selfIndex, float separationDistance,
                             float separationStrength, uint8_t maxNeighbors, uint8_t maxChecks) {
    PVector steering;
    steering.set(0, 0);
    int total = 0;
    const float separationDistanceSq = separationDistance * separationDistance;
    if (maxNeighbors == 0 || count <= 1) {
        return;
    }
    if (maxChecks == 0) {
        return;
    }
    if (maxNeighbors > maxChecks) {
        maxNeighbors = maxChecks;
    }
    if (maxNeighbors == 0) {
        maxNeighbors = 1;
    }

    uint8_t checks = maxChecks;
    if (checks > count - 1) checks = count - 1;
    uint8_t start = (uint8_t)((selfIndex * 7u + (uint8_t)noiseOffset) % count);

    for (uint8_t c = 0; c < checks; c++) {
        uint8_t i = (uint8_t)((start + c) % count);
        if (i == selfIndex) continue;
        
        float dx = position.x - meteors[i].position.x;
        float dy = position.y - meteors[i].position.y;
        float distanceSq = dx * dx + dy * dy;
        
        if (distanceSq > 0.0001f && distanceSq < separationDistanceSq) {
            // Avoid sqrtf in this hot path. A quadratic falloff is cheap and stable.
            float strength = (separationDistanceSq - distanceSq) / separationDistanceSq;
            steering.x += dx * strength;
            steering.y += dy * strength;
            total++;
            if (total >= maxNeighbors) break;
        }
    }
    
    if (total > 0) {
        float scale = separationStrength / (float)total;
        acceleration.x += steering.x * scale;
        acceleration.y += steering.y * scale;
    }
}

void Meteor::applyFlocking(Meteor* meteors, uint8_t count, uint8_t selfIndex, float neighborDistance,
                           float alignmentStrength, float cohesionStrength, uint8_t maxNeighbors,
                           uint8_t maxChecks) {
    if (count <= 1 || maxNeighbors == 0 || maxChecks == 0) return;

    const float neighborDistanceSq = neighborDistance * neighborDistance;
    uint8_t checks = maxChecks;
    if (checks > count - 1) checks = count - 1;
    uint8_t start = (uint8_t)((selfIndex * 11u + (uint8_t)(noiseOffset * 0.5f)) % count);

    PVector avgVel;
    avgVel.set(0, 0);
    PVector center;
    center.set(0, 0);
    uint8_t neighbors = 0;

    for (uint8_t c = 0; c < checks; c++) {
        uint8_t i = (uint8_t)((start + c) % count);
        if (i == selfIndex) continue;

        float dx = meteors[i].position.x - position.x;
        float dy = meteors[i].position.y - position.y;
        float d2 = dx * dx + dy * dy;
        if (d2 <= 0.0001f || d2 > neighborDistanceSq) continue;

        avgVel.x += meteors[i].velocity.x;
        avgVel.y += meteors[i].velocity.y;
        center.x += meteors[i].position.x;
        center.y += meteors[i].position.y;
        neighbors++;
        if (neighbors >= maxNeighbors) break;
    }

    if (neighbors == 0) return;

    float invN = 1.0f / (float)neighbors;
    avgVel.x *= invN;
    avgVel.y *= invN;
    center.x *= invN;
    center.y *= invN;

    // Alignment nudges velocity toward local average heading.
    acceleration.x += (avgVel.x - velocity.x) * alignmentStrength;
    acceleration.y += (avgVel.y - velocity.y) * alignmentStrength;

    // Cohesion pulls toward local center-of-mass.
    acceleration.x += (center.x - position.x) * cohesionStrength;
    acceleration.y += (center.y - position.y) * cohesionStrength;
}

void Meteor::applyAttractor(const PVector& attractorPos, float strength, float radius) {
    float dx = attractorPos.x - position.x;
    float dy = attractorPos.y - position.y;
    float distanceSq = dx * dx + dy * dy;
    float radiusSq = radius * radius;
    
    if (distanceSq < radiusSq && distanceSq > 0.0001f) {
        float forceStrength = strength * (1.0f - distanceSq / radiusSq);
        float invRadius = (radius > 0.001f) ? (1.0f / radius) : 1.0f;
        acceleration.x += dx * invRadius * forceStrength;
        acceleration.y += dy * invRadius * forceStrength;
    }
}

void Meteor::applyAttractorY(const PVector& attractorPos, float strength, float radius) {
    float dx = attractorPos.x - position.x;
    float dy = attractorPos.y - position.y;
    float distanceSq = dx * dx + dy * dy;
    float radiusSq = radius * radius;
    
    if (distanceSq < radiusSq && distanceSq > 0.0001f) {
        float forceStrength = strength * (1.0f - distanceSq / radiusSq);
        float invRadius = (radius > 0.001f) ? (1.0f / radius) : 1.0f;
        // Only apply Y component
        acceleration.y += dy * invRadius * forceStrength;
    }
}

void Meteor::update(float targetSpeed, float maxVerticalSpeed, float velocityDrag) {
    velocity.x += acceleration.x;
    velocity.y += acceleration.y;
    velocity.x *= velocityDrag;
    velocity.y *= velocityDrag;
    
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
    : Effect(matrix), tofSensor(sensor), tofInteraction(nullptr) {
    
    // Initialize parameters
    baseMeteorSpeed = 1.25f;
    boostedMeteorSpeed = 2.2f;
    currentMeteorSpeed = baseMeteorSpeed;
    baseSpawnRate = 2;
    boostedSpawnRate = 1;
    currentSpawnRate = baseSpawnRate;
    baseSpawnBurst = 1;
    boostedSpawnBurst = 2;
    currentSpawnBurst = baseSpawnBurst;
    baseMaxMeteors = 70;
    boostedMaxMeteors = 88;
    currentMaxMeteors = baseMaxMeteors;
    
    // Motion parameters (scaled for matrix resolution)
    flowCorrectionStrength = 0.06f;
    wobbleStrength = 0.10f;
    wobbleSpeed = 0.004f;
    separationDistance = 8.0f;
    separationStrength = 0.42f;
    flockNeighborDistance = 10.0f;
    alignmentStrength = 0.04f;
    cohesionStrength = 0.012f;
    maxVerticalSpeed = 2.0f;
    velocityDrag = 0.986f;
    trailFadeAmount = 12;
    attractorStrength = 0.12f;
    attractorRadius = 22.0f;
    centerAttractorStrength = 0.02f;
    centerAttractorRadius = 48.0f;
    forwardAcceleration = 0.03f;
    maxActiveAttractors = 10;
    maxSeparationNeighbors = 6;
    maxFlockNeighbors = 5;
    maxNeighborChecks = 20;
    trailPersistence = 178;
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
    
    tofInteractionData = {};

    reset();
}

MeteorShowerEffect::~MeteorShowerEffect() {
    if (tofInteraction) {
        delete tofInteraction;
        tofInteraction = nullptr;
    }
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
    initBackgroundElements();
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
    if (tofInteraction) {
        delete tofInteraction;
        tofInteraction = nullptr;
    }
    if (tofSensor) {
        tofInteraction = new TOFInteractionManager(tofSensor);
        tofInteraction->setDistanceRange(minDetectionDistance, maxDetectionDistance);
    }
}

void MeteorShowerEffect::setDetectionRange(int16_t minDist, int16_t maxDist) {
    minDetectionDistance = minDist;
    maxDetectionDistance = maxDist;
    if (tofInteraction) {
        tofInteraction->setDistanceRange(minDist, maxDist);
    }
}

void MeteorShowerEffect::initMeteors() {
    meteorCount = 0;
    // Start with a denser stream so the effect looks alive immediately.
    uint8_t initial = (baseMaxMeteors > 16) ? 16 : baseMaxMeteors;
    for (uint8_t i = 0; i < initial && i < MAX_METEORS; i++) {
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

void MeteorShowerEffect::initBackgroundElements() {
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();

    for (uint8_t i = 0; i < MAX_BG_STARS; i++) {
        stars[i].x = Meteor::randomFloat() * width;
        stars[i].y = Meteor::randomFloat() * height;
        stars[i].layer = (uint8_t)(Meteor::randomFloat() * 3.0f);
        stars[i].speed = 0.02f + stars[i].layer * 0.03f;
        stars[i].twinklePhase = Meteor::randomFloat() * 6.28318f;
        stars[i].hue = 150 + (uint8_t)(Meteor::randomFloat() * 70.0f);
        stars[i].baseBrightness = 18 + (uint8_t)(Meteor::randomFloat() * 65.0f);
    }

    for (uint8_t i = 0; i < MAX_NEBULA_CLOUDS; i++) {
        clouds[i].x = Meteor::randomFloat() * width;
        clouds[i].y = Meteor::randomFloat() * height;
        clouds[i].radius = 4.0f + Meteor::randomFloat() * 7.0f;
        clouds[i].driftX = -0.015f - Meteor::randomFloat() * 0.03f;
        clouds[i].driftY = (Meteor::randomFloat() - 0.5f) * 0.02f;
        clouds[i].pulsePhase = Meteor::randomFloat() * 6.28318f;
        clouds[i].hue = 156 + (uint8_t)(Meteor::randomFloat() * 62.0f);
    }
}

void MeteorShowerEffect::updateTofData() {
    if (!tofSensor || !tofSensor->isActive() || !tofInteraction) {
        tofGridReady = false;
        return;
    }
    tofInteraction->update();
    tofInteractionData = tofInteraction->getInteractionData();
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
            int16_t depth =
                tofSensor ? depthAtEffectCell(tofInteractionData, *tofSensor, x, y) : 0;
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
        currentSpawnBurst = boostedSpawnBurst;
        currentMaxMeteors = boostedMaxMeteors;
    } else {
        currentMeteorSpeed = baseMeteorSpeed;
        currentSpawnRate = baseSpawnRate;
        currentSpawnBurst = baseSpawnBurst;
        currentMaxMeteors = baseMaxMeteors;
    }
    
    // Spawn new meteors
    spawnCounter++;
    if (spawnCounter >= currentSpawnRate) {
        for (uint8_t b = 0; b < currentSpawnBurst; b++) {
            spawnMeteor();
        }
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
            m->applySeparation(
                meteors, meteorCount, i, separationDistance, separationStrength,
                maxSeparationNeighbors, maxNeighborChecks
            );
            m->applyFlocking(
                meteors, meteorCount, i, flockNeighborDistance, alignmentStrength,
                cohesionStrength, maxFlockNeighbors, maxNeighborChecks
            );
        }
        
        // Apply ToF attractors
        for (uint8_t j = 0; j < activeCount; j++) {
            m->applyAttractor(activeAttractors[j], attractorStrength, attractorRadius);
        }
        
        // If no ToF attractors active, use center attractor for Y-axis
        if (activeCount == 0 && centerAttractorStrength > 0.001f) {
            m->applyAttractorY(centerAttractor, centerAttractorStrength, centerAttractorRadius);
        }
        
        m->update(currentMeteorSpeed, maxVerticalSpeed, velocityDrag);
        
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

void MeteorShowerEffect::updateBackgroundElements() {
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();

    for (uint8_t i = 0; i < MAX_BG_STARS; i++) {
        stars[i].x -= stars[i].speed * currentMeteorSpeed;
        if (stars[i].x < 0) {
            stars[i].x += width;
            stars[i].y = Meteor::randomFloat() * height;
            stars[i].twinklePhase = Meteor::randomFloat() * 6.28318f;
        }
        stars[i].twinklePhase += 0.012f + stars[i].layer * 0.006f;
    }

    for (uint8_t i = 0; i < MAX_NEBULA_CLOUDS; i++) {
        clouds[i].x += clouds[i].driftX * currentMeteorSpeed;
        clouds[i].y += clouds[i].driftY;
        clouds[i].pulsePhase += 0.014f;
        if (clouds[i].x < -clouds[i].radius - 2.0f) {
            clouds[i].x = width + clouds[i].radius + Meteor::randomFloat() * 4.0f;
            clouds[i].y = Meteor::randomFloat() * height;
            clouds[i].pulsePhase = Meteor::randomFloat() * 6.28318f;
        }
        if (clouds[i].y < -clouds[i].radius) clouds[i].y = height + clouds[i].radius;
        if (clouds[i].y > height + clouds[i].radius) clouds[i].y = -clouds[i].radius;
    }
}

void MeteorShowerEffect::drawGradientBackground() {
    uint8_t width = m_matrix->getXResolution();
    uint8_t height = m_matrix->getYResolution();
    constexpr uint8_t kWaveCacheMax = 128;
    static float waveX[kWaveCacheMax];
    static float waveY[kWaveCacheMax];
    static float dustX[kWaveCacheMax];
    static float dustY[kWaveCacheMax];

    if (width > kWaveCacheMax || height > kWaveCacheMax) {
        // Safety fallback for unexpected panel sizes.
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
        return;
    }

    float phase = frameCount * 0.015f;
    for (uint8_t x = 0; x < width; x++) {
        waveX[x] = sinf(0.19f * x + phase);
        dustX[x] = sinf(0.065f * x + phase * 0.47f);
    }
    for (uint8_t y = 0; y < height; y++) {
        waveY[y] = sinf(0.11f * y + phase * 0.73f);
        dustY[y] = sinf(0.092f * y - phase * 0.35f);
    }
    
    // Deep space gradient with animated dust-lane variation.
    for (uint8_t y = 0; y < height; y++) {
        float gradientFactor = (float)y / height;
        float ridge = 0.5f + 0.5f * sinf((gradientFactor * 3.14159f) + phase * 0.22f);
        for (uint8_t x = 0; x < width; x++) {
            float wave = 0.5f + 0.25f * (waveX[x] + waveY[y]);
            float dust = 0.5f + 0.5f * (0.62f * dustX[x] + 0.38f * dustY[y]);
            uint8_t hue = 174 - (uint8_t)(gradientFactor * 28) + (uint8_t)(wave * 4.0f);
            uint8_t sat = 86 + (uint8_t)(gradientFactor * 120) + (uint8_t)(ridge * 14.0f);
            uint8_t val = 8 + (uint8_t)(gradientFactor * 58) + (uint8_t)(wave * 10.0f);
            uint8_t dustAtten = (uint8_t)(dust * 12.0f);
            if (val > dustAtten) val -= dustAtten;

            CRGB color;
            hsv2rgb_rainbow(CHSV(hue, sat, val), color);
            drawEffectPixel(x, y, color);
        }
    }
}

void MeteorShowerEffect::drawBackgroundElements() {
    for (uint8_t i = 0; i < MAX_NEBULA_CLOUDS; i++) {
        const NebulaCloud& c = clouds[i];
        int16_t cx = (int16_t)c.x;
        int16_t cy = (int16_t)c.y;
        int16_t r = (int16_t)(c.radius + 0.9f * sinf(c.pulsePhase));

        // Lightweight layered nebula blobs with pulse.
        CRGB cloudA;
        hsv2rgb_rainbow(CHSV(c.hue, 120, 24), cloudA);
        CRGB cloudB;
        hsv2rgb_rainbow(CHSV(c.hue + 10, 95, 19), cloudB);
        CRGB cloudC;
        hsv2rgb_rainbow(CHSV(c.hue + 18, 110, 15), cloudC);
        drawEffectPixel(cx, cy, cloudA);
        drawEffectPixel(cx - r, cy, cloudB);
        drawEffectPixel(cx + r, cy, cloudB);
        drawEffectPixel(cx, cy - r, cloudB);
        drawEffectPixel(cx, cy + r, cloudB);
        int16_t r2 = (r > 2) ? (r - 2) : 1;
        drawEffectPixel(cx - r2, cy - 1, cloudC);
        drawEffectPixel(cx + r2, cy + 1, cloudC);
        drawEffectPixel(cx - 1, cy + r2, cloudC);
        drawEffectPixel(cx + 1, cy - r2, cloudC);
    }

    for (uint8_t i = 0; i < MAX_BG_STARS; i++) {
        const BgStar& s = stars[i];
        int16_t sx = (int16_t)s.x;
        int16_t sy = (int16_t)s.y;
        uint8_t twinkle = s.baseBrightness + (uint8_t)(12.0f + 10.0f * sinf(s.twinklePhase));
        CRGB starColor;
        hsv2rgb_rainbow(CHSV(s.hue, 40 + s.layer * 35, twinkle), starColor);
        drawEffectPixel(sx, sy, starColor);

        // Bright stars occasionally bloom into tiny crosses.
        if ((i & 0x07u) == 0u && twinkle > 72) {
            CRGB sparkle = starColor;
            sparkle.nscale8_video(96);
            drawEffectPixel(sx - 1, sy, sparkle);
            drawEffectPixel(sx + 1, sy, sparkle);
            drawEffectPixel(sx, sy - 1, sparkle);
            drawEffectPixel(sx, sy + 1, sparkle);
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
        // Keep meteor heads pixel-based to support high particle counts.
        CRGB glowColor = headColor;
        glowColor.nscale8_video(90);
        drawLayerPixel(m_matrix->foreground, x, y, glowColor);

        CRGB coreColor = headColor;
        coreColor.nscale8_video(245);
        drawLayerPixel(m_matrix->foreground, x, y, coreColor);

        // Tiny directional spark tails keep dense streams legible at high counts.
        int16_t tx = x - (int16_t)constrain((int)(m.velocity.x * 1.2f), -2, 2);
        int16_t ty = y - (int16_t)constrain((int)(m.velocity.y * 0.9f), -1, 1);
        CRGB tailColor = headColor;
        tailColor.nscale8_video(78);
        drawLayerPixel(m_matrix->foreground, tx, ty, tailColor);
        if (m.size > 1.65f) {
            drawLayerPixel(m_matrix->foreground, tx - 1, ty, tailColor);
        }
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
    
    // Draw background field first.
    drawGradientBackground();
    updateBackgroundElements();
    drawBackgroundElements();
    
    // Update and draw planets behind meteors.
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
