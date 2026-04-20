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

void SpaceDriftEffect::pickBodyKindAndHue(uint8_t& kind, uint8_t& hue) {
    // Even-ish distribution across all 5 kinds; Generic absorbs any clamp overflow.
    uint8_t roll = (uint8_t)(randomFloat() * (float)(uint8_t)DriftBodyKind::Count_);
    if (roll >= (uint8_t)DriftBodyKind::Count_) roll = 0;
    kind = roll;
    switch ((DriftBodyKind)kind) {
        // Warm amber/tan bands like Jupiter/Saturn.
        case DriftBodyKind::GasGiant: hue = (uint8_t)(18 + randomFloat() * 28.0f); break;
        // Deep blues with a little teal drift.
        case DriftBodyKind::Ocean:    hue = (uint8_t)(140 + randomFloat() * 28.0f); break;
        // Cool icy hues — pale blues & cyans (palette is mostly desaturated anyway).
        case DriftBodyKind::Ice:      hue = (uint8_t)(130 + randomFloat() * 40.0f); break;
        // Reds/oranges; the vein shading pushes into bright amber separately.
        case DriftBodyKind::Lava:     hue = (uint8_t)(0 + randomFloat() * 16.0f); break;
        // Keep the original rainbow palette for the generic kind.
        default:                      hue = (uint8_t)(8 + randomFloat() * 220.0f); break;
    }
}

void SpaceDriftEffect::respawnBodyNearCamera(DriftBody& b) {
    b.radius = 3.0f + randomFloat() * 9.5f;
    pickBodyKindAndHue(b.kind, b.hue);
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
    pickBodyKindAndHue(fb.kind, fb.hue);
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

void SpaceDriftEffect::drawCircleRow(int16_t sx, int16_t sy, int16_t r, int16_t y, const CRGB& color) {
    if (r <= 0) return;
    int16_t dy = y - sy;
    if (dy < -r || dy > r) return;
    int32_t xx = (int32_t)r * (int32_t)r - (int32_t)dy * (int32_t)dy;
    if (xx < 0) return;
    int16_t dx = (int16_t)sqrtf((float)xx);
    int16_t xStart = sx - dx;
    int16_t xEnd = sx + dx;
    for (int16_t x = xStart; x <= xEnd; x++) drawGamePixel(x, y, color);
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

        switch ((DriftBodyKind)farBodies[i].kind) {
            case DriftBodyKind::GasGiant: drawFarBodyGasGiant(farBodies[i], sx, sy, r, pulse); break;
            case DriftBodyKind::Ocean:    drawFarBodyOcean(farBodies[i], sx, sy, r, pulse, i); break;
            case DriftBodyKind::Ice:      drawFarBodyIce(farBodies[i], sx, sy, r, pulse); break;
            case DriftBodyKind::Lava:     drawFarBodyLava(farBodies[i], sx, sy, r, pulse, i); break;
            default:                      drawFarBodyGeneric(farBodies[i], sx, sy, r, pulse, i); break;
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

void SpaceDriftEffect::drawFarBodyGeneric(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r,
                                          int16_t pulse, uint8_t idx) {
    uint8_t vOuter = (uint8_t)constrain(15 + pulse, 8, 32);
    uint8_t vDisk = (uint8_t)constrain(21 + pulse, 12, 44);
    uint8_t vMid = (uint8_t)constrain(30 + pulse, 18, 58);
    uint8_t vCore = (uint8_t)constrain(38 + pulse, 22, 72);

    CRGB outerGlow, outerDisk, midDisk, coreDisk;
    hsv2rgb_rainbow(CHSV(fb.hue, 105, vOuter), outerGlow);
    hsv2rgb_rainbow(CHSV(fb.hue + 2, 120, vDisk), outerDisk);
    hsv2rgb_rainbow(CHSV(fb.hue + 7, 94, vMid), midDisk);
    hsv2rgb_rainbow(CHSV(fb.hue + 12, 82, vCore), coreDisk);

    drawGameCircleClipped(sx, sy, (int16_t)(r + 1), outerGlow);
    drawGameCircleClipped(sx, sy, r, outerDisk);
    int16_t offA = (r / 7 > 0) ? r / 7 : 1;
    int16_t offB = (r / 9 > 0) ? r / 9 : 1;
    int16_t offC = (r / 4 > 0) ? r / 4 : 1;
    int16_t offD = (r / 5 > 0) ? r / 5 : 1;
    drawGameCircleClipped(sx - offA, sy - offB, (int16_t)(r * 0.70f), midDisk);
    drawGameCircleClipped(sx - offC, sy - offD, (int16_t)(r * 0.40f), coreDisk);

    uint32_t rng = (uint32_t)fb.textureSeed + ((uint32_t)idx << 16);
    uint8_t patchCount = (uint8_t)(2 + r / 12);
    CRGB patch;
    hsv2rgb_rainbow(CHSV(fb.hue + 16, 70, (uint8_t)constrain(vCore + 12, 0, 90)), patch);
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
}

void SpaceDriftEffect::drawFarBodyGasGiant(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r,
                                           int16_t pulse) {
    uint8_t vOuter = (uint8_t)constrain(16 + pulse, 8, 34);
    uint8_t vBase = (uint8_t)constrain(32 + pulse, 20, 62);

    CRGB outerGlow, base;
    hsv2rgb_rainbow(CHSV(fb.hue, 130, vOuter), outerGlow);
    hsv2rgb_rainbow(CHSV(fb.hue, 200, vBase), base);
    drawGameCircleClipped(sx, sy, (int16_t)(r + 1), outerGlow);
    drawGameCircleClipped(sx, sy, r, base);

    // Banded row sweep: 5 alternating bands of slightly shifted hue + brightness.
    // Band height scales with body size so small and large bodies both read as striped.
    int16_t bandH = (int16_t)max<int>(1, r / 3);
    int16_t yStart = (int16_t)max<int>(0, sy - r);
    int16_t yEnd = (int16_t)min<int>((int)playHeight - 1, sy + r);
    for (int16_t y = yStart; y <= yEnd; y++) {
        int16_t bandIdx = (y - (sy - r)) / bandH;
        // Skip even bands so the "base" body shows through and keeps warmth.
        if ((bandIdx & 1) == 0) continue;
        bool dark = (bandIdx & 2) != 0;
        uint8_t bandHueShift = dark ? (uint8_t)(fb.hue - 10) : (uint8_t)(fb.hue + 12);
        uint8_t bandSat = dark ? 220 : 170;
        uint8_t bandVal = dark ? (uint8_t)constrain(18 + pulse, 10, 42)
                                : (uint8_t)constrain(46 + pulse, 30, 82);
        CRGB bandCol;
        hsv2rgb_rainbow(CHSV(bandHueShift, bandSat, bandVal), bandCol);
        drawCircleRow(sx, sy, r, y, bandCol);
    }

    // Bright "storm" highlight — a single small oval near center-offset to echo Jupiter's spot.
    CRGB spot;
    hsv2rgb_rainbow(CHSV(fb.hue + 18, 120, (uint8_t)constrain(64 + pulse, 40, 100)), spot);
    int16_t offC = (r / 4 > 0) ? r / 4 : 1;
    int16_t offD = (r / 5 > 0) ? r / 5 : 1;
    drawGameCircleClipped(sx - offC, sy - offD, (int16_t)max<int>(1, r / 5), spot);
}

void SpaceDriftEffect::drawFarBodyOcean(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r,
                                        int16_t pulse, uint8_t idx) {
    uint8_t vOuter = (uint8_t)constrain(14 + pulse, 8, 32);
    uint8_t vBase = (uint8_t)constrain(26 + pulse, 16, 50);
    uint8_t vDeep = (uint8_t)constrain(34 + pulse, 20, 64);
    uint8_t vHighlight = (uint8_t)constrain(50 + pulse, 30, 90);

    CRGB outerGlow, base, deep, highlight;
    hsv2rgb_rainbow(CHSV(fb.hue, 180, vOuter), outerGlow);
    hsv2rgb_rainbow(CHSV(fb.hue, 230, vBase), base);
    hsv2rgb_rainbow(CHSV(fb.hue + 6, 210, vDeep), deep);
    hsv2rgb_rainbow(CHSV(fb.hue - 8, 140, vHighlight), highlight);

    drawGameCircleClipped(sx, sy, (int16_t)(r + 1), outerGlow);
    drawGameCircleClipped(sx, sy, r, base);
    // Slightly darker "deep" disk offset down-right for subtle sphere shading.
    int16_t shadeOff = (r / 5 > 0) ? r / 5 : 1;
    drawGameCircleClipped(sx + shadeOff, sy + shadeOff, (int16_t)(r * 0.82f), deep);
    // Bright top-left highlight hints at a light source.
    drawGameCircleClipped(sx - shadeOff, sy - shadeOff, (int16_t)max<int>(1, r / 4), highlight);

    // Drifting white cloud speckles. Animate angle over time so clouds rotate slowly.
    CRGB cloud;
    hsv2rgb_rainbow(CHSV(0, 0, (uint8_t)constrain(90 + pulse, 60, 140)), cloud);
    uint32_t rng = (uint32_t)fb.textureSeed + ((uint32_t)idx << 16) + 0x9e3779b9u;
    uint8_t cloudCount = (uint8_t)(3 + r / 8);
    float driftPhase = (float)(frameCount >> 2) * 0.0035f;
    for (uint8_t c = 0; c < cloudCount; c++) {
        rng = rng * 1664525u + 1013904223u;
        float ang = ((rng >> 8) & 1023u) * (6.2831853f / 1024.0f) + driftPhase;
        rng = rng * 1664525u + 1013904223u;
        float rr2 = ((float)((rng >> 10) & 1023u) / 1023.0f) * (r * 0.70f);
        int16_t tx = sx + (int16_t)(cosf(ang) * rr2);
        int16_t ty = sy + (int16_t)(sinf(ang) * rr2);
        int16_t tr = (r / 12 > 0) ? r / 12 : 1;
        drawGameCircleClipped(tx, ty, tr, cloud);
    }
}

void SpaceDriftEffect::drawFarBodyIce(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r,
                                      int16_t pulse) {
    uint8_t vOuter = (uint8_t)constrain(16 + pulse, 10, 36);
    uint8_t vBase = (uint8_t)constrain(56 + pulse, 40, 90);
    uint8_t vShadow = (uint8_t)constrain(30 + pulse, 18, 58);
    uint8_t vHighlight = (uint8_t)constrain(95 + pulse, 70, 130);

    CRGB outerGlow, base, shadow, highlight;
    hsv2rgb_rainbow(CHSV(fb.hue, 40, vOuter), outerGlow);
    hsv2rgb_rainbow(CHSV(fb.hue, 20, vBase), base);
    hsv2rgb_rainbow(CHSV(fb.hue, 110, vShadow), shadow);
    hsv2rgb_rainbow(CHSV(0, 0, vHighlight), highlight);

    drawGameCircleClipped(sx, sy, (int16_t)(r + 1), outerGlow);
    drawGameCircleClipped(sx, sy, r, base);
    int16_t shadeOff = (r / 4 > 0) ? r / 4 : 1;
    // Cool-blue shadow on the far side.
    drawGameCircleClipped(sx + shadeOff, sy + shadeOff, (int16_t)(r * 0.78f), shadow);
    // Crisp white rim-light cap on the near side.
    drawGameCircleClipped(sx - shadeOff, sy - shadeOff, (int16_t)max<int>(1, r / 3), highlight);
    // A couple of tiny sparkle pixels to sell the ice feel.
    CRGB sparkle = CRGB::White;
    drawGamePixel((int16_t)(sx - r / 2), (int16_t)(sy - r / 2), sparkle);
    drawGamePixel((int16_t)(sx + r / 6), (int16_t)(sy - r / 3), sparkle);
}

void SpaceDriftEffect::drawFarBodyLava(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r,
                                       int16_t pulse, uint8_t idx) {
    // Lava breathes on its own — independent pulse from the shared body pulse.
    float breathe = sinf((frameCount + idx * 41u) * 0.06f);
    uint8_t vOuter = (uint8_t)constrain(18 + pulse, 10, 38);
    uint8_t vBase = (uint8_t)constrain(24 + pulse, 14, 44);
    uint8_t vVein = (uint8_t)(140 + (int)(70.0f * breathe));
    uint8_t vBright = (uint8_t)constrain((int)vVein + 40, 160, 255);

    CRGB outerGlow, base, vein, veinHot;
    hsv2rgb_rainbow(CHSV(fb.hue, 230, vOuter), outerGlow);
    hsv2rgb_rainbow(CHSV(fb.hue, 255, vBase), base);
    hsv2rgb_rainbow(CHSV(fb.hue + 20, 210, vVein), vein);
    hsv2rgb_rainbow(CHSV(fb.hue + 32, 160, vBright), veinHot);

    drawGameCircleClipped(sx, sy, (int16_t)(r + 1), outerGlow);
    drawGameCircleClipped(sx, sy, r, base);

    // Glowing vein patches scattered over the surface; one is always hot.
    uint32_t rng = (uint32_t)fb.textureSeed + ((uint32_t)idx << 16) + 0x5f356495u;
    uint8_t patchCount = (uint8_t)(3 + r / 8);
    for (uint8_t c = 0; c < patchCount; c++) {
        rng = rng * 1664525u + 1013904223u;
        float ang = ((rng >> 8) & 1023u) * (6.2831853f / 1024.0f);
        rng = rng * 1664525u + 1013904223u;
        float rr2 = ((float)((rng >> 10) & 1023u) / 1023.0f) * (r * 0.72f);
        int16_t tx = sx + (int16_t)(cosf(ang) * rr2);
        int16_t ty = sy + (int16_t)(sinf(ang) * rr2);
        int16_t tr = (r / 10 > 0) ? r / 10 : 1;
        drawGameCircleClipped(tx, ty, tr, (c == 0) ? veinHot : vein);
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

        switch ((DriftBodyKind)bodies[i].kind) {
            case DriftBodyKind::GasGiant: drawBodyGasGiant(bodies[i], sx, sy, r); break;
            case DriftBodyKind::Ocean:    drawBodyOcean(bodies[i], sx, sy, r, i); break;
            case DriftBodyKind::Ice:      drawBodyIce(bodies[i], sx, sy, r); break;
            case DriftBodyKind::Lava:     drawBodyLava(bodies[i], sx, sy, r, i); break;
            default:                      drawBodyGeneric(bodies[i], sx, sy, r); break;
        }

        if (bodies[i].ringed) {
            CRGB ring;
            hsv2rgb_rainbow(CHSV(bodies[i].hue + 30, 120, 180), ring);
            drawGameRect(sx - r - 2, sy, (int16_t)(r * 2 + 4), 1, ring);
            drawGameRect(sx - r - 1, sy + 1, (int16_t)(r * 2 + 2), 1, ring);
        }
    }
}

void SpaceDriftEffect::drawBodyGeneric(const DriftBody& b, int16_t sx, int16_t sy, int16_t r) {
    CRGB glow, body, core;
    hsv2rgb_rainbow(CHSV(b.hue, 90, 55), glow);
    hsv2rgb_rainbow(CHSV(b.hue, 180, 150), body);
    hsv2rgb_rainbow(CHSV(b.hue + 12, 120, 235), core);
    drawGameCircle(sx, sy, r + 2, glow);
    drawGameCircle(sx, sy, r, body);
    drawGameCircle(sx - r / 3, sy - r / 3, max(1, r / 4), core);
}

void SpaceDriftEffect::drawBodyGasGiant(const DriftBody& b, int16_t sx, int16_t sy, int16_t r) {
    CRGB glow, base;
    hsv2rgb_rainbow(CHSV(b.hue, 130, 55), glow);
    hsv2rgb_rainbow(CHSV(b.hue, 200, 170), base);
    drawGameCircle(sx, sy, r + 2, glow);
    drawGameCircle(sx, sy, r, base);

    // Overlay 1-2 rows of darker/lighter band so even 3-4px bodies still read as striped.
    int16_t bandH = (int16_t)max<int>(1, r / 3);
    int16_t yStart = sy - r;
    int16_t yEnd = sy + r;
    for (int16_t y = yStart; y <= yEnd; y++) {
        int16_t bandIdx = (y - yStart) / bandH;
        if ((bandIdx & 1) == 0) continue;
        bool dark = (bandIdx & 2) != 0;
        CRGB bandCol;
        hsv2rgb_rainbow(
            CHSV(dark ? (uint8_t)(b.hue - 10) : (uint8_t)(b.hue + 12), dark ? 220 : 170,
                 dark ? 100 : 225),
            bandCol);
        drawCircleRow(sx, sy, r, y, bandCol);
    }
}

void SpaceDriftEffect::drawBodyOcean(const DriftBody& b, int16_t sx, int16_t sy, int16_t r, uint8_t idx) {
    CRGB glow, base, deep, highlight, cloud;
    hsv2rgb_rainbow(CHSV(b.hue, 180, 60), glow);
    hsv2rgb_rainbow(CHSV(b.hue, 220, 155), base);
    hsv2rgb_rainbow(CHSV(b.hue + 6, 200, 90), deep);
    hsv2rgb_rainbow(CHSV(b.hue - 8, 150, 230), highlight);
    hsv2rgb_rainbow(CHSV(0, 0, 220), cloud);

    drawGameCircle(sx, sy, r + 2, glow);
    drawGameCircle(sx, sy, r, base);
    int16_t shadeOff = max<int16_t>(1, r / 4);
    drawGameCircle(sx + shadeOff, sy + shadeOff, max<int16_t>(1, (int16_t)(r * 0.75f)), deep);
    drawGameCircle(sx - shadeOff, sy - shadeOff, max<int16_t>(1, r / 4), highlight);

    // 1-3 drifting cloud pixels per body; count scales with size.
    uint8_t cloudCount = (uint8_t)(1 + r / 4);
    uint32_t rng = 0x9e3779b9u + (uint32_t)idx * 2654435761u;
    float driftPhase = (float)(frameCount >> 2) * 0.0055f;
    for (uint8_t c = 0; c < cloudCount; c++) {
        rng = rng * 1664525u + 1013904223u;
        float ang = ((rng >> 8) & 1023u) * (6.2831853f / 1024.0f) + driftPhase;
        rng = rng * 1664525u + 1013904223u;
        float rr2 = ((float)((rng >> 10) & 1023u) / 1023.0f) * (r * 0.6f);
        int16_t cx = sx + (int16_t)(cosf(ang) * rr2);
        int16_t cy = sy + (int16_t)(sinf(ang) * rr2);
        drawGamePixel(cx, cy, cloud);
    }
}

void SpaceDriftEffect::drawBodyIce(const DriftBody& b, int16_t sx, int16_t sy, int16_t r) {
    CRGB glow, base, shadow, highlight;
    hsv2rgb_rainbow(CHSV(b.hue, 40, 70), glow);
    hsv2rgb_rainbow(CHSV(b.hue, 25, 215), base);
    hsv2rgb_rainbow(CHSV(b.hue, 120, 100), shadow);
    hsv2rgb_rainbow(CHSV(0, 0, 255), highlight);

    drawGameCircle(sx, sy, r + 2, glow);
    drawGameCircle(sx, sy, r, base);
    int16_t shadeOff = max<int16_t>(1, r / 4);
    drawGameCircle(sx + shadeOff, sy + shadeOff, max<int16_t>(1, (int16_t)(r * 0.7f)), shadow);
    drawGamePixel((int16_t)(sx - r / 2), (int16_t)(sy - r / 2), highlight);
}

void SpaceDriftEffect::drawBodyLava(const DriftBody& b, int16_t sx, int16_t sy, int16_t r, uint8_t idx) {
    // Pulse veins independently per body so they feel alive.
    float breathe = sinf((frameCount + idx * 37u) * 0.10f);
    uint8_t vVein = (uint8_t)constrain(170 + (int)(60.0f * breathe), 120, 235);
    uint8_t vBright = (uint8_t)constrain((int)vVein + 40, 180, 255);

    CRGB glow, base, vein, veinHot;
    hsv2rgb_rainbow(CHSV(b.hue, 230, 60), glow);
    hsv2rgb_rainbow(CHSV(b.hue, 255, 70), base);
    hsv2rgb_rainbow(CHSV(b.hue + 20, 210, vVein), vein);
    hsv2rgb_rainbow(CHSV(b.hue + 32, 160, vBright), veinHot);

    drawGameCircle(sx, sy, r + 2, glow);
    drawGameCircle(sx, sy, r, base);

    uint8_t veinCount = (uint8_t)(2 + r / 4);
    uint32_t rng = 0x4f1a9u + (uint32_t)idx * 2654435761u;
    for (uint8_t v = 0; v < veinCount; v++) {
        rng = rng * 1664525u + 1013904223u;
        float ang = ((rng >> 8) & 1023u) * (6.2831853f / 1024.0f);
        rng = rng * 1664525u + 1013904223u;
        float rr2 = ((float)((rng >> 10) & 1023u) / 1023.0f) * (r * 0.75f);
        int16_t vx = sx + (int16_t)(cosf(ang) * rr2);
        int16_t vy = sy + (int16_t)(sinf(ang) * rr2);
        drawGamePixel(vx, vy, (v == 0) ? veinHot : vein);
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
