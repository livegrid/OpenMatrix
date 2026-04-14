#include "AsteroidHopperEffect.h"
#include "SpacemanSprites.h"
#include "../TOFSensor/TOFSensor.h"
#include <math.h>

uint32_t AsteroidHopperEffect::noiseSeed = 24681357;

float AsteroidHopperEffect::randomFloat() {
    noiseSeed = noiseSeed * 1103515245 + 12345;
    return (float)(noiseSeed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

AsteroidHopperEffect::AsteroidHopperEffect(Matrix* matrix, TOFSensor* sensor)
    : Effect(matrix),
      tofSensor(sensor),
      tofInteraction(nullptr),
      tofInteractionData{},
      tofGridReady(false),
      handsRaised(false),
      palmActive(false),
      minDetectionDistance(TOF_MIN_DETECTION_DIST),
      maxDetectionDistance(TOF_MAX_DETECTION_DIST),
      minBlobCells(3),
      missingBlobRecentFrames(20),
      lastBlobFrame(0),
      filteredBlobX(0),
      frameCount(0),
      playerX(0),
      playerTargetX(0),
      playerLane(0),
      lives(3),
      score(0),
      gameOver(false),
      prevHandsRaised(false),
      prevPalmActive(false),
      hopCooldownUntil(0),
      hopInputLatchedUntil(0),
      nextHeldHopFrame(0),
      shieldCooldownUntil(0),
      shieldActiveUntil(0),
      invulnerableUntil(0),
      laneCount(6),
      laneSpacing(0),
      playerWidth(0),
      playerHeight(0),
      playerSmoothing(0.3f),
      gameOverFrame(0),
      speedScale(1.0f) {
    matrixWidth = m_matrix->getXResolution();
    matrixHeight = m_matrix->getYResolution();
    playWidth = matrixHeight;
    playHeight = matrixWidth;

    float scaleX = (float)playWidth / 64.0f;
    float scaleY = (float)playHeight / 192.0f;
    playerWidth = (uint16_t)max(5, (int)(9 * scaleX));
    playerHeight = (uint16_t)max(7, (int)(13 * scaleY));
    playerSmoothing = 0.24f;
    laneSpacing = (float)playHeight / (float)(laneCount + 1);
    filteredBlobX = playWidth * 0.5f;

    reset();
}

void AsteroidHopperEffect::setTofSensor(TOFSensor* sensor) {
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

void AsteroidHopperEffect::reset() {
    frameCount = 0;
    m_matrix->background->fillScreen(0);
    resetGame();
}

const char* AsteroidHopperEffect::getName() const {
    return "AsteroidHopper";
}

void AsteroidHopperEffect::resetGame() {
    lives = 4;
    score = 0;
    speedScale = 1.0f;
    gameOver = false;
    gameOverFrame = 0;
    initStars();
    initLanes();
    resetRound(false);
}

void AsteroidHopperEffect::resetRound(bool keepScoreAndLives) {
    (void)keepScoreAndLives;
    playerLane = 0;
    playerX = playWidth * 0.5f;
    playerTargetX = playerX;
    filteredBlobX = playerX;
    hopCooldownUntil = frameCount + 6;
    hopInputLatchedUntil = 0;
    nextHeldHopFrame = frameCount + 10;
    shieldActiveUntil = 0;
    invulnerableUntil = frameCount + 24;
    prevHandsRaised = false;
    prevPalmActive = false;
}

void AsteroidHopperEffect::initStars() {
    for (uint8_t i = 0; i < MAX_STARS; i++) {
        stars[i].x = (uint16_t)(randomFloat() * (float)playWidth);
        stars[i].y = (uint16_t)(randomFloat() * (float)playHeight);
        stars[i].b = (uint8_t)(20 + randomFloat() * 55.0f);
    }
}

void AsteroidHopperEffect::initLanes() {
    for (uint8_t i = 0; i < laneCount; i++) {
        HopperLane& lane = lanes[i];
        lane.safeLane = (i == 0 || i == laneCount - 1 || i == 3);
        lane.dir = (i % 2 == 0) ? 1 : -1;
        lane.speed = (0.16f + 0.045f * (float)i) * speedScale * ((float)playWidth / 64.0f);
        lane.hue = (uint8_t)(8 + i * 22);
        lane.asteroidCount = lane.safeLane ? 0 : (uint8_t)(1 + (i % 3 == 2 ? 1 : 0));
        for (uint8_t a = 0; a < 4; a++) {
            lane.asteroids[a].x = randomFloat() * (float)playWidth;
            lane.asteroids[a].radius = 1.6f + randomFloat() * 1.2f;
        }
        if (!lane.safeLane && lane.asteroidCount > 0) {
            float spacing = (float)playWidth / (float)lane.asteroidCount;
            for (uint8_t a = 0; a < lane.asteroidCount; a++) {
                lane.asteroids[a].x = spacing * a + randomFloat() * spacing * 0.22f;
            }
        }
    }
}

void AsteroidHopperEffect::advanceDifficulty() {
    speedScale += 0.04f;
    if (speedScale > 1.45f) speedScale = 1.45f;
    initLanes();
}

void AsteroidHopperEffect::updateTofData() {
    if (!tofSensor || !tofSensor->isActive() || !tofInteraction) {
        tofGridReady = false;
        handsRaised = false;
        palmActive = false;
        return;
    }
    tofInteraction->update();
    tofInteractionData = tofInteraction->getInteractionData();
    tofGridReady = true;
    handsRaised = tofInteractionData.handsRaised;
    palmActive = tofInteractionData.hasPalm;
}

float AsteroidHopperEffect::laneToY(uint8_t laneIdx) const {
    float yBottom = (float)playHeight - laneSpacing;
    return yBottom - laneSpacing * laneIdx;
}

float AsteroidHopperEffect::wrapDistanceX(float a, float b) const {
    float d = fabsf(a - b);
    float w = (float)playWidth;
    if (d > w * 0.5f) d = w - d;
    return d;
}

void AsteroidHopperEffect::updatePlayer(const HopperBlob& blob) {
    if (blob.valid) {
        filteredBlobX += (blob.x - filteredBlobX) * 0.35f;
        if (fabsf(blob.x - filteredBlobX) < 0.2f * ((float)playWidth / 64.0f)) {
            filteredBlobX = filteredBlobX * 0.75f + blob.x * 0.25f;
        }
        playerTargetX = filteredBlobX;
        lastBlobFrame = frameCount;
    } else if (frameCount - lastBlobFrame > missingBlobRecentFrames) {
        playerTargetX = playWidth * 0.5f;
    }

    playerX += (playerTargetX - playerX) * playerSmoothing;
    float half = playerWidth * 0.5f;
    if (playerX < half + 2.0f) playerX = half + 2.0f;
    if (playerX > (float)playWidth - half - 2.0f) playerX = (float)playWidth - half - 2.0f;

    if (handsRaised) {
        hopInputLatchedUntil = frameCount + 12;
        if (frameCount >= nextHeldHopFrame) {
            tryHopForward();
            nextHeldHopFrame = frameCount + 22;
        }
    }
    if (frameCount <= hopInputLatchedUntil && frameCount >= hopCooldownUntil) {
        tryHopForward();
        hopInputLatchedUntil = 0;
    }
    prevHandsRaised = handsRaised;

    if (palmActive && !prevPalmActive) tryTriggerShield();
    prevPalmActive = palmActive;
}

void AsteroidHopperEffect::tryHopForward() {
    if (frameCount < hopCooldownUntil) return;
    hopCooldownUntil = frameCount + 12;

    if (playerLane >= laneCount - 1) return;
    playerLane++;

    if (playerLane == laneCount - 1) {
        score++;
        advanceDifficulty();
        resetRound(true);
    }
}

void AsteroidHopperEffect::tryTriggerShield() {
    if (frameCount < shieldCooldownUntil) return;
    shieldActiveUntil = frameCount + 54;
    shieldCooldownUntil = frameCount + 100;
}

void AsteroidHopperEffect::updateLanes() {
    for (uint8_t i = 0; i < laneCount; i++) {
        HopperLane& lane = lanes[i];
        if (lane.safeLane) continue;
        for (uint8_t a = 0; a < lane.asteroidCount; a++) {
            lane.asteroids[a].x += lane.speed * (float)lane.dir;
            if (lane.asteroids[a].x < -lane.asteroids[a].radius) {
                lane.asteroids[a].x += (float)playWidth + lane.asteroids[a].radius * 2.0f;
            } else if (lane.asteroids[a].x > (float)playWidth + lane.asteroids[a].radius) {
                lane.asteroids[a].x -= (float)playWidth + lane.asteroids[a].radius * 2.0f;
            }
        }
    }
}

void AsteroidHopperEffect::checkCollisions() {
    if (playerLane >= laneCount) return;
    if (playerLane == 0 || playerLane == laneCount - 1) return;
    if (frameCount < invulnerableUntil) return;

    const HopperLane& lane = lanes[playerLane];
    for (uint8_t a = 0; a < lane.asteroidCount; a++) {
        float touch = lane.asteroids[a].radius + playerWidth * 0.26f;
        if (wrapDistanceX(playerX, lane.asteroids[a].x) <= touch) {
            if (frameCount < shieldActiveUntil) {
                shieldActiveUntil = 0;
                invulnerableUntil = frameCount + 18;
                return;
            }

            if (lives > 0) lives--;
            if (lives == 0) {
                gameOver = true;
                gameOverFrame = frameCount;
                return;
            }
            invulnerableUntil = frameCount + 70;
            playerLane = 0;
            playerX = playWidth * 0.5f;
            playerTargetX = playerX;
            return;
        }
    }
}

void AsteroidHopperEffect::drawGamePixel(int16_t gx, int16_t gy, const CRGB& color) {
    if (gx < 0 || gx >= (int16_t)playWidth || gy < 0 || gy >= (int16_t)playHeight) return;
    int16_t mx = gy;
    int16_t my = gx;
    m_matrix->background->drawPixel(mx, my, color);
}

void AsteroidHopperEffect::drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color) {
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillRect(mx, my, gh, gw, c565);
}

void AsteroidHopperEffect::drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color) {
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillCircle(mx, my, r, c565);
}

void AsteroidHopperEffect::drawBackground() {
    for (uint16_t gy = 0; gy < playHeight; gy++) {
        float t = (float)gy / (float)playHeight;
        uint8_t hue = (uint8_t)(174 - t * 18.0f);
        uint8_t sat = (uint8_t)(95 + t * 80.0f);
        uint8_t val = (uint8_t)(14 + t * 36.0f);
        CRGB c;
        hsv2rgb_rainbow(CHSV(hue, sat, val), c);
        uint16_t c565 = m_matrix->background->color565(c.r, c.g, c.b);
        int16_t mx = gy;
        m_matrix->background->fillRect(mx, 0, 1, playWidth, c565);
    }

    for (uint8_t i = 0; i < MAX_STARS; i++) {
        uint8_t tw = stars[i].b + (uint8_t)(sin((frameCount + i * 13) * 0.06f) * 20.0f + 20.0f);
        uint8_t b = constrain((int)tw, 12, 90);
        drawGamePixel((int16_t)stars[i].x, (int16_t)stars[i].y, CRGB(b, b, b));
    }
}

void AsteroidHopperEffect::drawLanes() {
    for (uint8_t i = 0; i < laneCount; i++) {
        int16_t y = (int16_t)laneToY(i);
        if (i == 0 || i == laneCount - 1) {
            CRGB c;
            hsv2rgb_rainbow(CHSV(100, 140, 70), c);
            drawGameRect(0, y - 1, playWidth, 3, c);
        } else {
            CRGB laneC;
            hsv2rgb_rainbow(CHSV(180, 120, 34), laneC);
            drawGameRect(0, y, playWidth, 1, laneC);
        }
    }
}

void AsteroidHopperEffect::drawGoalGlow() {
    int16_t y = (int16_t)laneToY(laneCount - 1);
    for (int i = 0; i < 3; i++) {
        uint8_t v = (uint8_t)(50 + 25 * i + (sin((frameCount + i * 9) * 0.12f) * 20.0f));
        CRGB c;
        hsv2rgb_rainbow(CHSV(80, 180, v), c);
        drawGameRect(0, y - i, playWidth, 1, c);
    }
}

void AsteroidHopperEffect::drawAsteroids() {
    for (uint8_t i = 1; i < laneCount - 1; i++) {
        const HopperLane& lane = lanes[i];
        int16_t y = (int16_t)laneToY(i);
        for (uint8_t a = 0; a < lane.asteroidCount; a++) {
            uint8_t hue = lane.hue + (uint8_t)(a * 10);
            CRGB body;
            hsv2rgb_rainbow(CHSV(hue, 210, 215), body);
            drawGameCircle((int16_t)lane.asteroids[a].x, y, (int16_t)lane.asteroids[a].radius, body);
            CRGB core;
            hsv2rgb_rainbow(CHSV(hue + 20, 150, 255), core);
            drawGameCircle((int16_t)lane.asteroids[a].x, y, 1, core);
        }
    }
}

void AsteroidHopperEffect::drawPlayer() {
    int16_t px = (int16_t)(playerX + 0.5f);
    int16_t py = (int16_t)(laneToY(playerLane) + 0.5f);

    const uint8_t (*sprite)[20] = kSpacemanSpriteMove;
    bool shielded = frameCount < shieldActiveUntil;
    if (frameCount < invulnerableUntil && ((frameCount / 3) % 2 == 0)) {
        sprite = kSpacemanSpriteHit;
    } else if (handsRaised || shielded) {
        sprite = kSpacemanSpriteBoost;
    }

    drawSpacemanSpriteScaled(sprite, px, py, (int16_t)playerWidth, (int16_t)playerHeight,
                             [this](int16_t gx, int16_t gy, const CRGB& c) { drawGamePixel(gx, gy, c); });

    if (shielded) {
        uint8_t v = (uint8_t)(140 + sin(frameCount * 0.24f) * 60.0f);
        CRGB shield;
        hsv2rgb_rainbow(CHSV(150, 140, v), shield);
        drawGameCircle(px, py, (int16_t)(playerWidth * 0.65f), shield);
    }
}

void AsteroidHopperEffect::drawHUD() {
    for (uint8_t i = 0; i < lives; i++) {
        CRGB life = CRGB(60, 255, 220);
        drawGameRect((int16_t)(playWidth - 3 - i * 4), 2, 2, 2, life);
    }

    uint8_t scoreDots = min(12, (int)score);
    for (uint8_t i = 0; i < scoreDots; i++) {
        CRGB c;
        hsv2rgb_rainbow(CHSV(32 + i * 8, 230, 235), c);
        drawGamePixel((int16_t)(2 + i * 2), 2, c);
    }

    uint8_t laneProgress = playerLane;
    if (laneProgress > laneCount - 1) laneProgress = laneCount - 1;
    for (uint8_t i = 0; i <= laneProgress; i++) {
        CRGB p;
        hsv2rgb_rainbow(CHSV(95, 180, 180), p);
        drawGameRect(1, (int16_t)(playHeight - 4 - i * 2), 1, 1, p);
    }
}

void AsteroidHopperEffect::drawGameOver() {
    m_matrix->background->dim(120);
    int16_t cx = (int16_t)playWidth / 2;
    int16_t cy = (int16_t)playHeight / 2;
    CRGB c = CRGB(255, 40, 40);
    for (int i = -4; i <= 4; i++) {
        drawGamePixel(cx + i, cy + i, c);
        drawGamePixel(cx + i, cy - i, c);
    }
}

void AsteroidHopperEffect::update() {
    frameCount++;
    updateTofData();

    HopperBlob blob{0, false};
    if (tofGridReady && tofInteractionData.hasBlob && tofSensor) {
        float xNorm = constrain(tofInteractionData.blobX, 0.0f, 1.0f);
        float yNorm = constrain(tofInteractionData.blobY, 0.0f, 1.0f);
        uint8_t gx = (uint8_t)constrain((int)(xNorm * 7.999f), 0, 7);
        uint8_t gy = (uint8_t)constrain((int)(yNorm * 7.999f), 0, 7);
        uint8_t nx, ny;
        tofSensor->fromDisplayAligned(gx, gy, nx, ny);
        uint8_t ex, ey;
        TOFSensor::inverseRotateGrid8x8(nx, ny, kTofEffectRotationDeg, ex, ey);
        float effectNormX = (ex + 0.5f) / 8.0f;
        effectNormX = constrain(effectNormX, 0.0f, 1.0f);
        blob.x = effectNormX * (playWidth - 1);
        blob.valid = tofInteractionData.blobSize >= minBlobCells;
    }

    if (!gameOver) {
        updatePlayer(blob);
        updateLanes();
        checkCollisions();
    }

    drawBackground();
    drawGoalGlow();
    drawLanes();
    drawAsteroids();
    drawPlayer();
    drawHUD();

    if (gameOver) {
        drawGameOver();
        if ((frameCount - gameOverFrame) > 120) {
            resetGame();
        }
    }
}
