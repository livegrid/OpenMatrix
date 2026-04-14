#include "SpaceDriftEffect.h"
#include "SpacemanSprites.h"
#include "../TOFSensor/TOFSensor.h"
#include <math.h>

uint32_t SpaceDriftEffect::noiseSeed = 88776655;

float SpaceDriftEffect::randomFloat() {
    noiseSeed = noiseSeed * 1103515245 + 12345;
    return (float)(noiseSeed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

SpaceDriftEffect::SpaceDriftEffect(Matrix* matrix, TOFSensor* sensor)
    : Effect(matrix),
      tofSensor(sensor),
      tofInteraction(nullptr),
      tofInteractionData{},
      tofGridReady(false),
      minDetectionDistance(TOF_MIN_DETECTION_DIST),
      maxDetectionDistance(TOF_MAX_DETECTION_DIST),
      matrixWidth(0),
      matrixHeight(0),
      playWidth(0),
      playHeight(0),
      frameCount(0),
      camX(0),
      camY(0),
      camVX(0),
      camVY(0),
      axisX(0),
      axisY(0),
      idleDriftPhase(0),
      worldSpanX(0),
      worldSpanY(0),
      rocketSpawnCountdown(0) {
    matrixWidth = m_matrix->getXResolution();
    matrixHeight = m_matrix->getYResolution();

    // Keep portrait play-space so shared spaceman sprite feels natural.
    playWidth = matrixHeight;
    playHeight = matrixWidth;

    worldSpanX = max(170.0f, (float)playWidth * 4.8f);
    worldSpanY = max(300.0f, (float)playHeight * 2.3f);

    reset();
}

void SpaceDriftEffect::setTofSensor(TOFSensor* sensor) {
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

void SpaceDriftEffect::reset() {
    frameCount = 0;
    camX = 0;
    camY = 0;
    camVX = 0;
    camVY = 0;
    axisX = 0;
    axisY = 0;
    idleDriftPhase = 0;
    m_matrix->background->fillScreen(0);
    resetWorld();
}

const char* SpaceDriftEffect::getName() const {
    return "SpaceDrift";
}

void SpaceDriftEffect::resetWorld() {
    for (uint8_t i = 0; i < MAX_STARS; i++) {
        stars[i].x = (randomFloat() - 0.5f) * worldSpanX;
        stars[i].y = (randomFloat() - 0.5f) * worldSpanY;
        stars[i].layer = (uint8_t)(randomFloat() * 3.0f);
        stars[i].hue = (uint8_t)(160 + randomFloat() * 65.0f);
        stars[i].baseBrightness = (uint8_t)(26 + randomFloat() * 80.0f);
    }

    for (uint8_t i = 0; i < MAX_CLOUDS; i++) {
        clouds[i].x = (randomFloat() - 0.5f) * worldSpanX;
        clouds[i].y = (randomFloat() - 0.5f) * worldSpanY;
        clouds[i].radius = 24.0f + randomFloat() * 48.0f;
        clouds[i].hue = (uint8_t)(150 + randomFloat() * 70.0f);
        clouds[i].driftX = (int8_t)(randomFloat() * 3.0f) - 1;
        clouds[i].driftY = (int8_t)(randomFloat() * 3.0f) - 1;
    }

    for (uint8_t i = 0; i < MAX_FAR_BODIES; i++) {
        respawnFarBodyAtEdge(farBodies[i]);
        // Advance each body partway along its path so the scene is already populated on reset.
        float preload = 40.0f + randomFloat() * 210.0f;
        farBodies[i].x += farBodies[i].vx * preload;
        farBodies[i].y += farBodies[i].vy * preload;
    }

    for (uint8_t i = 0; i < MAX_ROCKETS; i++) {
        rockets[i].active = false;
    }
    rocketSpawnCountdown = (uint16_t)(160 + randomFloat() * 200.0f);

    for (uint8_t i = 0; i < MAX_BODIES; i++) {
        respawnBodyAtEdge(bodies[i]);
        float preload = 25.0f + randomFloat() * 150.0f;
        bodies[i].x += bodies[i].vx * preload;
        bodies[i].y += bodies[i].vy * preload;
    }

    for (uint8_t i = 0; i < MAX_ALIENS; i++) {
        respawnAlienNearCamera(aliens[i]);
        aliens[i].x += (randomFloat() - 0.5f) * worldSpanX * 0.6f;
        aliens[i].y += (randomFloat() - 0.5f) * worldSpanY * 0.7f;
    }
}

void SpaceDriftEffect::respawnBodyNearCamera(DriftBody& b) {
    b.radius = 3.0f + randomFloat() * 9.5f;
    b.hue = (uint8_t)(8 + randomFloat() * 220.0f);
    b.ringed = randomFloat() > 0.55f;
    b.vx = (randomFloat() - 0.5f) * 0.045f;
    b.vy = (randomFloat() - 0.5f) * 0.040f;
    b.ttl = (uint16_t)(1400 + randomFloat() * 2200.0f);
    b.x = camX + (randomFloat() - 0.5f) * worldSpanX;
    b.y = camY + (randomFloat() - 0.5f) * worldSpanY;
}

void SpaceDriftEffect::respawnFarBody(DriftFarBody& fb) {
    // Slightly smaller far bodies reduce clipping artifacts and feel more natural on this panel size.
    fb.radius = 16.0f + randomFloat() * 24.0f;
    fb.parallax = 0.10f + randomFloat() * 0.08f;
    fb.hue = (uint8_t)(4 + randomFloat() * 230.0f);
    fb.ringed = randomFloat() > 0.42f;
    fb.textureSeed = (uint16_t)(randomFloat() * 65535.0f);
    fb.vx = 0.0f;
    fb.vy = 0.0f;
    fb.x = camX;
    fb.y = camY;
}

void SpaceDriftEffect::respawnBodyAtEdge(DriftBody& b) {
    respawnBodyNearCamera(b);

    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;
    float margin = b.radius + 10.0f;
    uint8_t side = (uint8_t)(randomFloat() * 4.0f);  // 0=L,1=R,2=T,3=B
    if (side > 3) side = 3;

    float sx = 0.0f;
    float sy = 0.0f;
    float baseSpeed = 0.030f + randomFloat() * 0.060f;
    float tangent = (randomFloat() - 0.5f) * 0.026f;

    if (side == 0) {
        sx = -margin;
        sy = randomFloat() * (float)playHeight;
        b.vx = baseSpeed;
        b.vy = tangent;
    } else if (side == 1) {
        sx = (float)playWidth + margin;
        sy = randomFloat() * (float)playHeight;
        b.vx = -baseSpeed;
        b.vy = tangent;
    } else if (side == 2) {
        sx = randomFloat() * (float)playWidth;
        sy = -margin;
        b.vx = tangent;
        b.vy = baseSpeed;
    } else {
        sx = randomFloat() * (float)playWidth;
        sy = (float)playHeight + margin;
        b.vx = tangent;
        b.vy = -baseSpeed;
    }

    b.x = camX + (sx - centerX);
    b.y = camY + (sy - centerY);
    b.ttl = 0;
}

void SpaceDriftEffect::respawnFarBodyAtEdge(DriftFarBody& fb) {
    respawnFarBody(fb);

    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;
    float margin = fb.radius + 18.0f;
    uint8_t side = (uint8_t)(randomFloat() * 4.0f);  // 0=L,1=R,2=T,3=B
    if (side > 3) side = 3;

    float sx = 0.0f;
    float sy = 0.0f;
    float baseSpeed = 0.0034f + randomFloat() * 0.0052f;
    float tangent = (randomFloat() - 0.5f) * 0.0024f;

    if (side == 0) {
        sx = -margin;
        sy = randomFloat() * (float)playHeight;
        fb.vx = baseSpeed;
        fb.vy = tangent;
    } else if (side == 1) {
        sx = (float)playWidth + margin;
        sy = randomFloat() * (float)playHeight;
        fb.vx = -baseSpeed;
        fb.vy = tangent;
    } else if (side == 2) {
        sx = randomFloat() * (float)playWidth;
        sy = -margin;
        fb.vx = tangent;
        fb.vy = baseSpeed;
    } else {
        sx = randomFloat() * (float)playWidth;
        sy = (float)playHeight + margin;
        fb.vx = tangent;
        fb.vy = -baseSpeed;
    }

    fb.x = camX * fb.parallax + (sx - centerX);
    fb.y = camY * fb.parallax + (sy - centerY);
}

void SpaceDriftEffect::respawnAlienNearCamera(DriftAlien& a) {
    a.size = (uint8_t)(3 + randomFloat() * 4.0f);
    a.hue = (uint8_t)(90 + randomFloat() * 140.0f);
    a.vx = (randomFloat() - 0.5f) * 0.07f;
    a.vy = (randomFloat() - 0.5f) * 0.07f;
    a.ttl = (uint16_t)(950 + randomFloat() * 1800.0f);
    a.x = camX + (randomFloat() - 0.5f) * worldSpanX;
    a.y = camY + (randomFloat() - 0.5f) * worldSpanY;
    a.blinkPhase = (uint8_t)(randomFloat() * 255.0f);
}

void SpaceDriftEffect::updateTofData() {
    if (!tofSensor || !tofSensor->isActive() || !tofInteraction) {
        tofGridReady = false;
        tofInteractionData = {};
        return;
    }
    tofInteraction->update();
    tofInteractionData = tofInteraction->getInteractionData();
    tofGridReady = true;
}

void SpaceDriftEffect::updateCameraFromInteraction() {
    float xInput = 0.0f;
    float yInput = 0.0f;

    if (tofGridReady && tofInteractionData.hasBlob) {
        float normX = constrain(tofInteractionData.blobX, 0.0f, 1.0f);
        xInput = (normX - 0.5f) * 2.0f;
    }

    if (tofGridReady && tofInteractionData.distanceHint != TofDistanceHint::NoPerson &&
        tofInteractionData.stanceDepthMm > 0) {
        float center = ((float)minDetectionDistance + (float)maxDetectionDistance) * 0.5f;
        float half = ((float)maxDetectionDistance - (float)minDetectionDistance) * 0.5f;
        if (half < 1.0f) half = 1.0f;
        // Closer body stance pushes "forward" in world Y.
        yInput = (center - (float)tofInteractionData.stanceDepthMm) / half;
    }

    if (fabsf(xInput) < 0.10f) xInput = 0.0f;
    if (fabsf(yInput) < 0.09f) yInput = 0.0f;
    xInput = constrain(xInput, -1.0f, 1.0f);
    yInput = constrain(yInput, -1.0f, 1.0f);

    // Keep the world alive even when no one is in front of the sensor.
    if ((!tofGridReady || !tofInteractionData.hasBlob) &&
        (tofInteractionData.distanceHint == TofDistanceHint::NoPerson)) {
        idleDriftPhase += 0.022f;
        xInput += sinf(idleDriftPhase) * 0.08f;
        yInput += cosf(idleDriftPhase * 0.7f) * 0.07f;
    }

    // Flip both axes (sensor / panel orientation vs. expected drift).
    xInput = -xInput;
    yInput = -yInput;

    axisX += (xInput - axisX) * 0.14f;
    axisY += (yInput - axisY) * 0.12f;

    camVX += axisX * 0.105f;
    camVY += axisY * 0.105f;
    camVX *= 0.925f;
    camVY *= 0.925f;

    camVX = constrain(camVX, -2.35f, 2.35f);
    camVY = constrain(camVY, -2.35f, 2.35f);

    camX += camVX;
    camY += camVY;
}

void SpaceDriftEffect::wrapAroundCamera(float& x, float& y, float factorX, float factorY) {
    float cx = camX * factorX;
    float cy = camY * factorY;
    float halfX = worldSpanX * 0.5f;
    float halfY = worldSpanY * 0.5f;

    while (x - cx > halfX) x -= worldSpanX;
    while (x - cx < -halfX) x += worldSpanX;
    while (y - cy > halfY) y -= worldSpanY;
    while (y - cy < -halfY) y += worldSpanY;
}

void SpaceDriftEffect::applyPalmPush() {
    if (!tofGridReady || !tofInteractionData.hasPalm || !tofSensor) return;

    uint8_t nx, ny;
    tofSensor->fromDisplayAligned(tofInteractionData.palmX, tofInteractionData.palmY, nx, ny);
    uint8_t ex, ey;
    TOFSensor::inverseRotateGrid8x8(nx, ny, kTofEffectRotationDeg, ex, ey);

    float palmGX = ((float)ex + 0.5f) / 8.0f * (float)(playWidth - 1);
    float palmGY = ((float)ey + 0.5f) / 8.0f * (float)(playHeight - 1);
    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;

    float pulseWorldX = camX + (palmGX - centerX);
    float pulseWorldY = camY + (palmGY - centerY);
    float pushRadius = 54.0f;

    for (uint8_t i = 0; i < MAX_BODIES; i++) {
        float dx = bodies[i].x - pulseWorldX;
        float dy = bodies[i].y - pulseWorldY;
        float d2 = dx * dx + dy * dy;
        if (d2 < 1.0f || d2 > pushRadius * pushRadius) continue;
        float d = sqrtf(d2);
        float k = (1.0f - d / pushRadius) * 0.065f;
        bodies[i].vx += (dx / d) * k;
        bodies[i].vy += (dy / d) * k;
    }

    for (uint8_t i = 0; i < MAX_ALIENS; i++) {
        float dx = aliens[i].x - pulseWorldX;
        float dy = aliens[i].y - pulseWorldY;
        float d2 = dx * dx + dy * dy;
        if (d2 < 1.0f || d2 > pushRadius * pushRadius) continue;
        float d = sqrtf(d2);
        float k = (1.0f - d / pushRadius) * 0.085f;
        aliens[i].vx += (dx / d) * k;
        aliens[i].vy += (dy / d) * k;
    }
}

void SpaceDriftEffect::spawnRocket(uint8_t slot) {
    if (slot >= MAX_ROCKETS) return;
    DriftRocket& r = rockets[slot];
    float angle = randomFloat() * 6.2831853f;
    float speed = 0.34f + randomFloat() * 0.34f;
    r.vx = cosf(angle) * speed;
    r.vy = sinf(angle) * speed;
    float dist = 95.0f + randomFloat() * 130.0f;
    r.x = camX - r.vx * dist;
    r.y = camY - r.vy * dist;
    r.hue = (uint8_t)(8 + randomFloat() * 40.0f);
    r.ttl = (uint16_t)(260 + randomFloat() * 220.0f);
    r.active = true;
}

void SpaceDriftEffect::trySpawnRocket() {
    if (rocketSpawnCountdown > 0) {
        rocketSpawnCountdown--;
        return;
    }
    rocketSpawnCountdown = (uint16_t)(200 + randomFloat() * 380.0f);
    if (randomFloat() > 0.62f) return;
    for (uint8_t i = 0; i < MAX_ROCKETS; i++) {
        if (!rockets[i].active) {
            spawnRocket(i);
            return;
        }
    }
}

void SpaceDriftEffect::updateWorldObjects() {
    for (uint8_t i = 0; i < MAX_CLOUDS; i++) {
        clouds[i].x += clouds[i].driftX * 0.006f;
        clouds[i].y += clouds[i].driftY * 0.006f;
        wrapAroundCamera(clouds[i].x, clouds[i].y, 0.35f, 0.35f);
    }

    for (uint8_t i = 0; i < MAX_STARS; i++) {
        float f = 0.26f + stars[i].layer * 0.30f;
        wrapAroundCamera(stars[i].x, stars[i].y, f, f);
    }

    for (uint8_t i = 0; i < MAX_FAR_BODIES; i++) {
        farBodies[i].x += farBodies[i].vx;
        farBodies[i].y += farBodies[i].vy;
        float sx = playWidth * 0.5f + (farBodies[i].x - camX * farBodies[i].parallax);
        float sy = playHeight * 0.5f + (farBodies[i].y - camY * farBodies[i].parallax);
        float margin = farBodies[i].radius + 24.0f;
        if (sx < -margin || sx > (float)playWidth + margin || sy < -margin ||
            sy > (float)playHeight + margin) {
            respawnFarBodyAtEdge(farBodies[i]);
        }
    }

    for (uint8_t i = 0; i < MAX_BODIES; i++) {
        bodies[i].x += bodies[i].vx;
        bodies[i].y += bodies[i].vy;
        bodies[i].vx *= 0.9992f;
        bodies[i].vy *= 0.9992f;
        float sx = playWidth * 0.5f + (bodies[i].x - camX);
        float sy = playHeight * 0.5f + (bodies[i].y - camY);
        float margin = bodies[i].radius + 14.0f;
        if (sx < -margin || sx > (float)playWidth + margin || sy < -margin ||
            sy > (float)playHeight + margin) {
            respawnBodyAtEdge(bodies[i]);
        }
    }

    for (uint8_t i = 0; i < MAX_ALIENS; i++) {
        if (aliens[i].ttl > 0) aliens[i].ttl--;
        if (aliens[i].ttl == 0) {
            respawnAlienNearCamera(aliens[i]);
            aliens[i].x += camVX * 28.0f;
            aliens[i].y += camVY * 28.0f;
        }
        float wobbleX = sinf((frameCount + i * 17) * 0.03f) * 0.010f;
        float wobbleY = cosf((frameCount + i * 21) * 0.024f) * 0.010f;
        aliens[i].x += aliens[i].vx + wobbleX;
        aliens[i].y += aliens[i].vy + wobbleY;
        aliens[i].vx *= 0.995f;
        aliens[i].vy *= 0.995f;
        wrapAroundCamera(aliens[i].x, aliens[i].y, 0.95f, 0.95f);
    }

    if ((frameCount % 135u) == 0u) {
        uint8_t ai = (uint8_t)(randomFloat() * MAX_ALIENS);
        respawnAlienNearCamera(aliens[ai]);
    }

    trySpawnRocket();
    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;
    constexpr float kRocketParallax = 0.56f;
    for (uint8_t i = 0; i < MAX_ROCKETS; i++) {
        if (!rockets[i].active) continue;
        rockets[i].x += rockets[i].vx;
        rockets[i].y += rockets[i].vy;
        if (rockets[i].ttl > 0) rockets[i].ttl--;
        float sx = centerX + (rockets[i].x - camX * kRocketParallax);
        float sy = centerY + (rockets[i].y - camY * kRocketParallax);
        if (rockets[i].ttl == 0 || sx < -40.0f || sx > (float)playWidth + 40.0f || sy < -40.0f ||
            sy > (float)playHeight + 40.0f) {
            rockets[i].active = false;
        }
    }
}

void SpaceDriftEffect::drawGamePixel(int16_t gx, int16_t gy, const CRGB& color) {
    if (gx < 0 || gx >= (int16_t)playWidth || gy < 0 || gy >= (int16_t)playHeight) return;
    int16_t mx = gy;
    int16_t my = gx;
    m_matrix->background->drawPixel(mx, my, color);
}

void SpaceDriftEffect::drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color) {
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillRect(mx, my, gh, gw, c565);
}

void SpaceDriftEffect::drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color) {
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillCircle(mx, my, r, c565);
}

void SpaceDriftEffect::drawGameCircleClipped(int16_t gx, int16_t gy, int16_t r, const CRGB& color) {
    if (r <= 0) return;
    if (gx + r < 0 || gx - r >= (int16_t)playWidth || gy + r < 0 || gy - r >= (int16_t)playHeight) return;

    int16_t yStart = gy - r;
    int16_t yEnd = gy + r;
    if (yStart < 0) yStart = 0;
    if (yEnd >= (int16_t)playHeight) yEnd = (int16_t)playHeight - 1;

    int32_t rr = (int32_t)r * (int32_t)r;
    for (int16_t y = yStart; y <= yEnd; y++) {
        int16_t dy = y - gy;
        int32_t xx = rr - (int32_t)dy * (int32_t)dy;
        if (xx < 0) continue;
        int16_t dx = (int16_t)sqrtf((float)xx);
        int16_t xStart = gx - dx;
        int16_t xEnd = gx + dx;
        if (xStart < 0) xStart = 0;
        if (xEnd >= (int16_t)playWidth) xEnd = (int16_t)playWidth - 1;
        for (int16_t x = xStart; x <= xEnd; x++) {
            drawGamePixel(x, y, color);
        }
    }
}

void SpaceDriftEffect::drawBackground() {
    for (uint16_t gy = 0; gy < playHeight; gy++) {
        float t = (float)gy / (float)playHeight;
        uint8_t hue = (uint8_t)(180 - t * 22.0f);
        uint8_t sat = (uint8_t)(95 + t * 90.0f);
        uint8_t val = (uint8_t)(12 + t * 28.0f);
        CRGB c;
        hsv2rgb_rainbow(CHSV(hue, sat, val), c);
        uint16_t c565 = m_matrix->background->color565(c.r, c.g, c.b);
        int16_t mx = gy;
        m_matrix->background->fillRect(mx, 0, 1, playWidth, c565);
    }
}

void SpaceDriftEffect::drawFarBodies() {
    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;

    for (uint8_t i = 0; i < MAX_FAR_BODIES; i++) {
        float f = farBodies[i].parallax;
        int16_t sx = (int16_t)(centerX + (farBodies[i].x - camX * f));
        int16_t sy = (int16_t)(centerY + (farBodies[i].y - camY * f));
        int16_t r = (int16_t)farBodies[i].radius;
        if (sx < -r - 14 || sx > (int16_t)playWidth + r + 14 || sy < -r - 14 || sy > (int16_t)playHeight + r + 14)
            continue;

        int16_t pulse = (int16_t)(6.0f * sinf((frameCount + i * 31u) * 0.04f));
        uint8_t vOuter = (uint8_t)constrain(15 + pulse, 8, 32);
        uint8_t vDisk = (uint8_t)constrain(21 + pulse, 12, 44);
        uint8_t vMid = (uint8_t)constrain(30 + pulse, 18, 58);
        uint8_t vCore = (uint8_t)constrain(38 + pulse, 22, 72);

        CRGB outerGlow, outerDisk, midDisk, coreDisk;
        hsv2rgb_rainbow(CHSV(farBodies[i].hue, 105, vOuter), outerGlow);
        hsv2rgb_rainbow(CHSV(farBodies[i].hue + 2, 120, vDisk), outerDisk);
        hsv2rgb_rainbow(CHSV(farBodies[i].hue + 7, 94, vMid), midDisk);
        hsv2rgb_rainbow(CHSV(farBodies[i].hue + 12, 82, vCore), coreDisk);

        // Use clipped circle raster for far bodies so they remain visible when center is offscreen.
        drawGameCircleClipped(sx, sy, (int16_t)(r + 1), outerGlow);
        drawGameCircleClipped(sx, sy, r, outerDisk);
        int16_t offA = (r / 7 > 0) ? r / 7 : 1;
        int16_t offB = (r / 9 > 0) ? r / 9 : 1;
        int16_t offC = (r / 4 > 0) ? r / 4 : 1;
        int16_t offD = (r / 5 > 0) ? r / 5 : 1;
        drawGameCircleClipped(sx - offA, sy - offB, (int16_t)(r * 0.70f), midDisk);
        drawGameCircleClipped(sx - offC, sy - offD, (int16_t)(r * 0.40f), coreDisk);

        // Sparse stable mottling texture: brighter/lighter inclusions like the small body style.
        uint32_t rng = (uint32_t)farBodies[i].textureSeed + ((uint32_t)i << 16);
        uint8_t patchCount = (uint8_t)(2 + r / 12);
        CRGB patch;
        hsv2rgb_rainbow(CHSV(farBodies[i].hue + 16, 70, (uint8_t)constrain(vCore + 12, 0, 90)), patch);
        for (uint8_t c = 0; c < patchCount; c++) {
            rng = rng * 1664525u + 1013904223u;
            float ang = ((rng >> 8) & 1023u) * (6.2831853f / 1024.0f);
            rng = rng * 1664525u + 1013904223u;
            float rr2 = ((float)((rng >> 10) & 1023u) / 1023.0f) * (r * 0.55f);
            int16_t tx = sx + (int16_t)(cosf(ang) * rr2);
            int16_t ty = sy + (int16_t)(sinf(ang) * rr2);
            int16_t tr = (r / 14 > 0) ? r / 14 : 1;
            drawGameCircleClipped(tx, ty, tr, patch);
        }

        if (farBodies[i].ringed) {
            CRGB ring;
            uint8_t ringV = (uint8_t)constrain(30 + pulse, 16, 58);
            hsv2rgb_rainbow(CHSV(farBodies[i].hue + 25, 88, ringV), ring);
            int16_t y0 = sy + r / 4;
            int16_t y1 = y0 + 1;
            for (int16_t x = sx - r - 4; x <= sx + r + 4; x++) drawGamePixel(x, y0, ring);
            for (int16_t x = sx - r - 3; x <= sx + r + 3; x++) drawGamePixel(x, y1, ring);
        }
    }
}

void SpaceDriftEffect::drawClouds() {
    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;

    for (uint8_t i = 0; i < MAX_CLOUDS; i++) {
        float f = 0.35f;
        int16_t sx = (int16_t)(centerX + (clouds[i].x - camX * f));
        int16_t sy = (int16_t)(centerY + (clouds[i].y - camY * f));
        int16_t r = (int16_t)clouds[i].radius;

        CRGB c0, c1;
        hsv2rgb_rainbow(CHSV(clouds[i].hue, 120, 24), c0);
        hsv2rgb_rainbow(CHSV(clouds[i].hue + 10, 90, 16), c1);
        drawGameCircle(sx, sy, r, c0);
        drawGameCircle(sx + r / 3, sy - r / 4, (int16_t)(r * 0.65f), c1);
        drawGameCircle(sx - r / 3, sy + r / 5, (int16_t)(r * 0.55f), c1);
    }
}

void SpaceDriftEffect::drawStars() {
    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;

    for (uint8_t i = 0; i < MAX_STARS; i++) {
        float f = 0.26f + stars[i].layer * 0.30f;
        int16_t sx = (int16_t)(centerX + (stars[i].x - camX * f));
        int16_t sy = (int16_t)(centerY + (stars[i].y - camY * f));
        if (sx < 0 || sx >= (int16_t)playWidth || sy < 0 || sy >= (int16_t)playHeight) continue;

        uint8_t twinkle = stars[i].baseBrightness +
                          (uint8_t)(16.0f + 15.0f * sinf((frameCount + i * 19) * 0.07f));
        CRGB c;
        hsv2rgb_rainbow(CHSV(stars[i].hue, 40 + stars[i].layer * 40, twinkle), c);
        drawGamePixel(sx, sy, c);
    }
}

void SpaceDriftEffect::drawBodies() {
    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;

    for (uint8_t i = 0; i < MAX_BODIES; i++) {
        int16_t sx = (int16_t)(centerX + (bodies[i].x - camX));
        int16_t sy = (int16_t)(centerY + (bodies[i].y - camY));
        int16_t r = (int16_t)bodies[i].radius;

        CRGB glow, body, core;
        hsv2rgb_rainbow(CHSV(bodies[i].hue, 90, 55), glow);
        hsv2rgb_rainbow(CHSV(bodies[i].hue, 180, 150), body);
        hsv2rgb_rainbow(CHSV(bodies[i].hue + 12, 120, 235), core);
        drawGameCircle(sx, sy, r + 2, glow);
        drawGameCircle(sx, sy, r, body);
        drawGameCircle(sx - r / 3, sy - r / 3, max(1, r / 4), core);

        if (bodies[i].ringed) {
            CRGB ring;
            hsv2rgb_rainbow(CHSV(bodies[i].hue + 30, 120, 180), ring);
            drawGameRect(sx - r - 2, sy, (int16_t)(r * 2 + 4), 1, ring);
            drawGameRect(sx - r - 1, sy + 1, (int16_t)(r * 2 + 2), 1, ring);
        }
    }
}

void SpaceDriftEffect::drawRockets() {
    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;
    constexpr float kRocketParallax = 0.56f;

    for (uint8_t i = 0; i < MAX_ROCKETS; i++) {
        if (!rockets[i].active) continue;
        float sx = centerX + (rockets[i].x - camX * kRocketParallax);
        float sy = centerY + (rockets[i].y - camY * kRocketParallax);
        float vmag = sqrtf(rockets[i].vx * rockets[i].vx + rockets[i].vy * rockets[i].vy);
        if (vmag < 0.02f) continue;
        float nx = rockets[i].vx / vmag;
        float ny = rockets[i].vy / vmag;

        int16_t hx = (int16_t)(sx + 0.5f);
        int16_t hy = (int16_t)(sy + 0.5f);
        int16_t len = 7;
        for (int16_t s = 0; s <= len; s++) {
            float t = (float)s / (float)len;
            int16_t px = hx - (int16_t)(nx * (float)s);
            int16_t py = hy - (int16_t)(ny * (float)s);
            uint8_t v = (uint8_t)(40.0f + t * 215.0f);
            CRGB c;
            hsv2rgb_rainbow(CHSV(rockets[i].hue, 200, v), c);
            drawGamePixel(px, py, c);
        }
        CRGB tip;
        hsv2rgb_rainbow(CHSV(rockets[i].hue + 4, 140, 255), tip);
        drawGamePixel(hx, hy, tip);
    }
}

void SpaceDriftEffect::drawAliens() {
    float centerX = playWidth * 0.5f;
    float centerY = playHeight * 0.5f;

    for (uint8_t i = 0; i < MAX_ALIENS; i++) {
        int16_t sx = (int16_t)(centerX + (aliens[i].x - camX * 0.95f));
        int16_t sy = (int16_t)(centerY + (aliens[i].y - camY * 0.95f));
        int16_t r = aliens[i].size;
        bool blink = ((frameCount + aliens[i].blinkPhase) % 120) < 7;

        CRGB body, belly;
        hsv2rgb_rainbow(CHSV(aliens[i].hue, 200, 210), body);
        hsv2rgb_rainbow(CHSV(aliens[i].hue + 20, 120, 180), belly);
        drawGameCircle(sx, sy, r, body);
        drawGameCircle(sx, sy + r / 3, max(1, r / 2), belly);

        CRGB eye = blink ? CRGB(255, 90, 130) : CRGB::White;
        drawGamePixel(sx - 1, sy - 1, eye);
        drawGamePixel(sx + 1, sy - 1, eye);

        CRGB antenna;
        hsv2rgb_rainbow(CHSV(aliens[i].hue + 40, 120, 180), antenna);
        drawGamePixel(sx, sy - r - 1, antenna);
        drawGamePixel(sx - 1, sy - r, antenna);
        drawGamePixel(sx + 1, sy - r, antenna);
    }
}

void SpaceDriftEffect::drawSpaceman() {
    int16_t centerX = (int16_t)playWidth / 2;
    int16_t centerY = (int16_t)playHeight / 2;

    float motionX = camVX * 7.8f + axisX * 3.2f;
    float motionY = camVY * 7.8f + axisY * 3.2f;
    int16_t px = centerX + (int16_t)constrain(motionX, -7.0f, 7.0f);
    int16_t py = centerY + (int16_t)constrain(motionY, -10.0f, 10.0f);

    int16_t drawW = max<int16_t>(5, (int16_t)(playWidth * 0.19f));
    int16_t drawH = max<int16_t>(8, (int16_t)(playHeight * 0.095f));

    const uint8_t (*sprite)[20] = kSpacemanSpriteMove;
    if (tofGridReady && tofInteractionData.hasPalm) {
        sprite = kSpacemanSpriteBoost;
    }

    drawSpacemanSpriteScaled(sprite, px, py, drawW, drawH,
                             [this](int16_t gx, int16_t gy, const CRGB& c) { drawGamePixel(gx, gy, c); });

    // Intentionally no halo ring: keep the character crisp in the center.
}

void SpaceDriftEffect::drawAmbientHud() {
    uint8_t speed = (uint8_t)min(10.0f, sqrtf(camVX * camVX + camVY * camVY) * 8.0f);
    for (uint8_t i = 0; i < speed; i++) {
        CRGB c;
        hsv2rgb_rainbow(CHSV(150 + i * 5, 170, 170), c);
        drawGamePixel((int16_t)(2 + i * 2), 2, c);
    }
}

void SpaceDriftEffect::update() {
    frameCount++;
    updateTofData();
    updateCameraFromInteraction();
    applyPalmPush();
    updateWorldObjects();

    drawBackground();
    drawFarBodies();
    drawClouds();
    drawStars();
    drawBodies();
    drawRockets();
    drawAliens();
    drawSpaceman();
    drawAmbientHud();
}
