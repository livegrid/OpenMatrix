#include "GravityFlapEffect.h"
#include "SpacemanSprites.h"
#include "../TOFSensor/TOFSensor.h"
#include <math.h>

uint32_t GravityFlapEffect::noiseSeed = 77777;

float GravityFlapEffect::randomFloat() {
    noiseSeed = noiseSeed * 1103515245 + 12345;
    return (float)(noiseSeed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

GravityFlapEffect::GravityFlapEffect(Matrix* matrix, TOFSensor* sensor)
    : Effect(matrix),
      tofSensor(sensor),
      tofInteraction(nullptr),
      tofInteractionData{},
      tofGridReady(false),
      minDetectionDistance(TOF_MIN_DETECTION_DIST),
      maxDetectionDistance(TOF_MAX_DETECTION_DIST),
      handsRaised(false),
      minBlobCells(3),
      missingBlobRecentFrames(20),
      lastBlobFrame(0),
      filteredBlobX(0),
      frameCount(0),
      playerVY(0),
      playerTargetX(0),
      platformCount(0),
      cameraY(0),
      startY(0),
      heightScore(0),
      maxHeight(0),
      gameOver(false),
      lastBlobGameY(0),
      haveLastBlobY(false),
      lastPulseFrame(0),
      lastBoostFrame(0) {
    matrixWidth = m_matrix->getXResolution();
    matrixHeight = m_matrix->getYResolution();
    playWidth = matrixHeight;
    playHeight = matrixWidth;

    float scaleX = (float)playWidth / 64.0f;
    float scaleY = (float)playHeight / 192.0f;

    playerWidth = (uint16_t)max(4, (int)(8 * scaleX));
    playerHeight = (uint16_t)max(6, (int)(12 * scaleY));
    playerSmoothing = 0.25f;
    filteredBlobX = playWidth * 0.5f;

    gravity = 0.28f * scaleY;
    boostVelocity = -2.6f * scaleY;
    maxFallSpeed = 3.8f * scaleY;
    maxUpSpeed = 4.2f * scaleY;
    pulseCooldownFrames = 10;
    pulseDeltaGame = 6.f * scaleY;
    platformHeight = max(2.f, 3.f * scaleY);
    platformSpacing = 26.f * scaleY;
    platformSpeedMin = 0.25f * scaleX;
    platformSpeedMax = 0.6f * scaleX;
    platformMinW = (uint16_t)max(8, (int)(16 * scaleX));
    platformMaxW = (uint16_t)max(platformMinW + 2, (int)(28 * scaleX));

    reset();
}

void GravityFlapEffect::setTofSensor(TOFSensor* sensor) {
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

void GravityFlapEffect::reset() {
    frameCount = 0;
    m_matrix->background->fillScreen(0);
    resetGame();
}

const char* GravityFlapEffect::getName() const {
    return "GravityFlap";
}

void GravityFlapEffect::drawGamePixel(int16_t gx, int16_t gy, const CRGB& color) {
    if (gx < 0 || gx >= (int16_t)playWidth || gy < 0 || gy >= (int16_t)playHeight) return;
    int16_t mx = gy;
    int16_t my = gx;
    m_matrix->background->drawPixel(mx, my, color);
}

void GravityFlapEffect::drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color) {
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillRect(mx, my, gh, gw, c565);
}

void GravityFlapEffect::drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color) {
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillCircle(mx, my, r, c565);
}

void GravityFlapEffect::updateTofData() {
    if (!tofSensor || !tofSensor->isActive() || !tofInteraction) {
        tofGridReady = false;
        handsRaised = false;
        return;
    }
    tofInteraction->update();
    tofInteractionData = tofInteraction->getInteractionData();
    tofGridReady = true;
    handsRaised = tofInteractionData.handsRaised;
}

void GravityFlapEffect::resetGame() {
    playerX = playWidth / 2.f;
    playerTargetX = playerX;
    playerY = playHeight - 24.f * ((float)playHeight / 192.f);
    startY = playerY;
    playerVY = 0;
    cameraY = playerY - playHeight * 0.6f;
    heightScore = 0;
    maxHeight = 0;
    gameOver = false;
    haveLastBlobY = false;
    lastPulseFrame = 0;
    lastBoostFrame = 0;
    lastBlobFrame = 0;
    initPlatforms();
}

void GravityFlapEffect::spawnPlatform(uint8_t idx, float y) {
    FlapPlatform& p = platforms[idx];
    p.w = (float)platformMinW + randomFloat() * (float)(platformMaxW - platformMinW);
    p.x = 4.f + randomFloat() * ((float)playWidth - p.w - 8.f);
    p.y = y;
    p.speed = platformSpeedMin + randomFloat() * (platformSpeedMax - platformSpeedMin);
    p.dir = randomFloat() > 0.5f ? 1 : -1;
    p.hue = (uint8_t)(randomFloat() * 255.f);
}

void GravityFlapEffect::initPlatforms() {
    platformCount = MAX_PLATFORMS;
    float y = playerY + 10.f * ((float)playHeight / 192.f);
    for (uint8_t i = 0; i < platformCount; i++) {
        spawnPlatform(i, y);
        y -= platformSpacing;
    }
}

void GravityFlapEffect::updatePlatforms() {
    for (uint8_t i = 0; i < platformCount; i++) {
        FlapPlatform& p = platforms[i];
        p.x += p.speed * (float)p.dir;
        if (p.x < 2.f) {
            p.x = 2.f;
            p.dir *= -1;
        } else if (p.x + p.w > (float)playWidth - 2.f) {
            p.x = (float)playWidth - p.w - 2.f;
            p.dir *= -1;
        }
    }

    float highestY = platforms[0].y;
    for (uint8_t i = 1; i < platformCount; i++) {
        if (platforms[i].y < highestY) highestY = platforms[i].y;
    }
    for (uint8_t i = 0; i < platformCount; i++) {
        FlapPlatform& p = platforms[i];
        if (p.y > cameraY + (float)playHeight + 30.f) {
            p.y = highestY - platformSpacing;
            p.x = 4.f + randomFloat() * ((float)playWidth - p.w - 8.f);
            p.hue = (uint8_t)(randomFloat() * 255.f);
        }
    }

    ensurePlatformsBelowPlayer();
}

void GravityFlapEffect::ensurePlatformsBelowPlayer() {
    const float cushion = fmaxf(50.f, (float)playHeight * 0.42f);
    for (uint8_t guard = 0; guard < platformCount + 3; guard++) {
        float maxPy = platforms[0].y;
        for (uint8_t i = 1; i < platformCount; i++) {
            maxPy = fmaxf(maxPy, platforms[i].y);
        }
        if (maxPy >= playerY + cushion) break;

        uint8_t topIdx = 0;
        for (uint8_t i = 1; i < platformCount; i++) {
            if (platforms[i].y < platforms[topIdx].y) {
                topIdx = i;
            }
        }
        spawnPlatform(topIdx, maxPy + platformSpacing);
    }
}

bool GravityFlapEffect::detectBlobPulse(FlapBlob& blob) {
    if (!blob.valid) return false;
    if (frameCount - lastPulseFrame <= pulseCooldownFrames) return false;

    bool pulse = false;
    if (haveLastBlobY && (lastBlobGameY - blob.y) > pulseDeltaGame) {
        pulse = true;
    }
    lastBlobGameY = blob.y;
    haveLastBlobY = true;
    if (pulse) {
        lastPulseFrame = frameCount;
    }
    return pulse;
}

void GravityFlapEffect::updatePlayer(FlapBlob& blob) {
    if (blob.valid) {
        float smoothing = 0.35f;
        filteredBlobX += (blob.x - filteredBlobX) * smoothing;
        if (fabsf(blob.x - filteredBlobX) < 0.18f * (float)playWidth / 64.f) {
            filteredBlobX = filteredBlobX * 0.7f + blob.x * 0.3f;
        }
        playerTargetX = filteredBlobX;
        lastBlobFrame = frameCount;
    } else if (frameCount - lastBlobFrame > missingBlobRecentFrames) {
        playerTargetX = playWidth / 2.f;
    }

    playerX += (playerTargetX - playerX) * playerSmoothing;
    float half = playerWidth * 0.5f;
    if (playerX < half + 2.f) playerX = half + 2.f;
    if (playerX > (float)playWidth - half - 2.f) playerX = (float)playWidth - half - 2.f;

    bool blobPulse = detectBlobPulse(blob);

    if (handsRaised) {
        // Jetpack fires in short bursts while hands stay up: hop-hop-hop, not hover.
        constexpr uint8_t kHopPeriod = 18;
        constexpr uint8_t kHopThrustFrames = 5;
        if ((frameCount % kHopPeriod) < kHopThrustFrames) {
            playerVY = boostVelocity * 1.14f;
            lastBoostFrame = frameCount;
        }
    } else if (blobPulse) {
        playerVY = boostVelocity;
        lastBoostFrame = frameCount;
    }

    playerVY += gravity;
    if (playerVY > maxFallSpeed) playerVY = maxFallSpeed;
    if (playerVY < -maxUpSpeed) playerVY = -maxUpSpeed;

    playerY += playerVY;

    bool grounded = false;
    if (playerVY >= 0) {
        for (uint8_t i = 0; i < platformCount; i++) {
            FlapPlatform& p = platforms[i];
            float halfW = playerWidth * 0.4f;
            if (playerX + halfW < p.x || playerX - halfW > p.x + p.w) continue;
            float topY = p.y;
            float playerBottom = playerY + playerHeight * 0.5f;
            if (playerBottom >= topY && playerBottom <= topY + platformHeight + 1.f) {
                playerY = topY - playerHeight * 0.5f;
                playerVY = 0;
                grounded = true;
                break;
            }
        }
    }

    {
        float maxPyDeath = platforms[0].y;
        for (uint8_t i = 1; i < platformCount; i++) {
            maxPyDeath = fmaxf(maxPyDeath, platforms[i].y);
        }
        const float offWorld = fmaxf(80.f, (float)playHeight * 0.85f);
        if (playerY > maxPyDeath + offWorld) {
            gameOver = true;
        }
    }

    float heightDelta = startY - playerY;
    if (heightDelta > maxHeight) maxHeight = heightDelta;
    heightScore = (uint16_t)maxHeight;

    float camTarget = playerY - (float)playHeight * 0.6f;
    cameraY += (camTarget - cameraY) * 0.1f;
    float camCap = playerY - (float)playHeight * 0.4f;
    if (cameraY > camCap) cameraY = camCap;
}

void GravityFlapEffect::drawBackground() {
    for (uint16_t row = 0; row < playHeight; row++) {
        float t = (float)row / (float)playHeight;
        uint8_t hue = (uint8_t)(148 - t * 22);
        uint8_t sat = (uint8_t)(80 + t * 80);
        uint8_t val = (uint8_t)(28 + t * 50);
        CRGB c;
        hsv2rgb_rainbow(CHSV(hue, sat, val), c);
        uint16_t c565 = m_matrix->background->color565(c.r, c.g, c.b);
        int16_t mx = (int16_t)row;
        m_matrix->background->fillRect(mx, 0, 1, playWidth, c565);
    }
}

void GravityFlapEffect::drawWorld() {
    for (uint8_t i = 0; i < platformCount; i++) {
        FlapPlatform& p = platforms[i];
        int16_t sy = (int16_t)(p.y - cameraY);
        int16_t sx = (int16_t)p.x;
        int16_t sw = (int16_t)max(1.f, p.w);
        int16_t sh = (int16_t)max(1.f, platformHeight);
        if (sy + sh < -2 || sy > (int16_t)playHeight + 2) continue;

        CRGB body;
        hsv2rgb_rainbow(CHSV(p.hue, 200, 235), body);
        drawGameRect(sx, sy, sw, sh, body);
        CRGB edge;
        hsv2rgb_rainbow(CHSV(p.hue, 160, 200), edge);
        drawGameRect(sx, sy + sh - 1, sw, 1, edge);
    }

    int16_t px = (int16_t)(playerX + 0.5f);
    int16_t py = (int16_t)((playerY - cameraY) + 0.5f);
    const uint8_t (*sprite)[20] = kSpacemanSpriteMove;
    bool boosting = (frameCount - lastBoostFrame < 8);
    if (boosting) {
        sprite = kSpacemanSpriteBoost;
    }
    int16_t dw = (int16_t)playerWidth;
    int16_t dh = (int16_t)playerHeight;
    drawSpacemanSpriteScaled(sprite, px, py, dw, dh,
                             [this](int16_t gx, int16_t gy, const CRGB& c) { drawGamePixel(gx, gy, c); });
}

void GravityFlapEffect::drawHUD() {
    CRGB white = CRGB::White;
    uint16_t w = (uint16_t)min((int)heightScore / 4, (int)playWidth - 4);
    for (uint16_t i = 0; i < w; i++) {
        drawGamePixel((int16_t)(2 + i), 2, white);
    }
}

void GravityFlapEffect::update() {
    frameCount++;
    updateTofData();

    FlapBlob blob = {0, 0, 0, false};
    if (tofGridReady && tofInteractionData.hasBlob && tofSensor) {
        float xNorm = constrain(tofInteractionData.blobX, 0.f, 1.f);
        float yNorm = constrain(tofInteractionData.blobY, 0.f, 1.f);
        uint8_t gx = (uint8_t)constrain((int)(xNorm * 7.999f), 0, 7);
        uint8_t gy = (uint8_t)constrain((int)(yNorm * 7.999f), 0, 7);
        uint8_t nx, ny;
        tofSensor->fromDisplayAligned(gx, gy, nx, ny);
        uint8_t ex, ey;
        TOFSensor::inverseRotateGrid8x8(nx, ny, kTofEffectRotationDeg, ex, ey);
        float effectNormX = (ex + 0.5f) / 8.f;
        effectNormX = constrain(effectNormX, 0.f, 1.f);
        float effectNormY = (ey + 0.5f) / 8.f;
        effectNormY = constrain(effectNormY, 0.f, 1.f);
        blob.x = effectNormX * (float)(playWidth - 1);
        blob.y = effectNormY * (float)(playHeight - 1);
        blob.size = tofInteractionData.blobSize;
        blob.valid = tofInteractionData.blobSize >= minBlobCells;
    }

    if (!gameOver) {
        updatePlayer(blob);
        updatePlatforms();
    }

    drawBackground();
    drawWorld();
    drawHUD();

    if (blob.valid && !gameOver) {
        CRGB ind;
        hsv2rgb_rainbow(CHSV(40, 200, 230), ind);
        drawGameCircle((int16_t)blob.x, (int16_t)(playHeight - 2), 1, ind);
    }

    if (gameOver && (frameCount % 90u) == 0u) {
        resetGame();
    }
}
