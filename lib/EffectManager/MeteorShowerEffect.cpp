#include "MeteorShowerEffect.h"
#include "SpacemanSprites.h"
#include "StepBackSprite.h"
#include "../TOFSensor/TOFSensor.h"
#include <math.h>

namespace {
constexpr int16_t kTofEffectRotationDeg = 0;
constexpr uint32_t kFpsEstimate = 30;
constexpr uint32_t kRampStartFrames = 30 * kFpsEstimate;   // 30 s
constexpr uint32_t kRampPeakFrames = 120 * kFpsEstimate;   // 2 min

float smoothstep01(float t) {
    t = constrain(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
}  // namespace

static uint32_t meteorGameSeed = 12345;

float MeteorShowerEffect::randomFloat() {
    meteorGameSeed = meteorGameSeed * 1103515245 + 12345;
    return (float)(meteorGameSeed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

MeteorShowerEffect::MeteorShowerEffect(Matrix* matrix, TOFSensor* sensor)
    : Effect(matrix),
      tofSensor(sensor),
      tofInteraction(nullptr),
      tofInteractionData{} {
    matrixWidth = m_matrix->getXResolution();
    matrixHeight = m_matrix->getYResolution();
    playWidth = matrixHeight;
    playHeight = matrixWidth;

    float scaleX = (float)playWidth / 64.0f;
    float scaleY = (float)playHeight / 192.0f;

    playerWidth = (uint16_t)(10 * scaleX);
    playerHeight = (uint16_t)(14 * scaleY);
    playerYOffset = (uint16_t)(7 * scaleY);
    playerSmoothing = 0.28f;
    playerHitFlipFrames = 18;

    bulletSpeed = 4.0f * scaleY;
    bulletWidth = max(1, (int)(2 * scaleX));
    bulletHeight = max(2, (int)(4 * scaleY));
    bulletCooldownFrames = 18;

    meteorBaseSpeed = 1.05f * scaleY;
    baselineMeteorSpeed = meteorBaseSpeed;
    meteorSpawnInterval = 55;
    baselineSpawnInterval = meteorSpawnInterval;
    maxActiveMeteors = 6;
    baselineMaxMeteors = maxActiveMeteors;

    tofGridReady = false;
    minDetectionDistance = TOF_MIN_DETECTION_DIST;
    maxDetectionDistance = TOF_MAX_DETECTION_DIST;
    handsRaised = false;
    minBlobCells = 3;
    missingBlobRecentFrames = 20;
    lastBlobFrame = 0;
    filteredBlobX = playWidth * 0.5f;

    frameCount = 0;
    planetCount = 0;
    planetSpawnCounter = 0;
    meteorSpawnCounter = 0;

    gameplayFrames = 0;
    reset();
}

MeteorShowerEffect::~MeteorShowerEffect() {
    if (tofInteraction) {
        delete tofInteraction;
        tofInteraction = nullptr;
    }
}

void MeteorShowerEffect::reset() {
    frameCount = 0;
    gameplayFrames = 0;
    initStars();
    initPlanets();
    resetGame();
    m_matrix->background->fillScreen(0);
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

void MeteorShowerEffect::drawGamePixel(int16_t gx, int16_t gy, const CRGB& color) {
    if (gx < 0 || gx >= (int16_t)playWidth || gy < 0 || gy >= (int16_t)playHeight) return;
    int16_t mx = gy;
    int16_t my = gx;
    m_matrix->background->drawPixel(mx, my, color);
}

void MeteorShowerEffect::drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color) {
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillRect(mx, my, gh, gw, c565);
}

void MeteorShowerEffect::drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color) {
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillCircle(mx, my, r, c565);
}

void MeteorShowerEffect::updateTofData() {
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

void MeteorShowerEffect::resetGame() {
    resetPlayer();
    resetMeteors();

    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
        playerBullets[i].active = false;
    }

    bulletCooldown = 0;
    score = 0;
    lives = 3;
    gameOver = false;
    meteorSpawnCounter = 0;
    gameplayFrames = 0;
    meteorBaseSpeed = baselineMeteorSpeed;
    meteorSpawnInterval = baselineSpawnInterval;
    maxActiveMeteors = baselineMaxMeteors;
}

void MeteorShowerEffect::resetPlayer() {
    playerX = playWidth / 2.0f;
    playerTargetX = playerX;
    filteredBlobX = playerX;
    playerY = playHeight - playerYOffset - playerHeight / 2.0f;
    playerHitTimer = 0;
}

void MeteorShowerEffect::resetMeteors() {
    for (uint8_t i = 0; i < MAX_METEORS; i++) {
        meteors[i].active = false;
    }
}

void MeteorShowerEffect::updateDifficultyRamp() {
    float ramp = 0.0f;
    if (gameplayFrames > kRampStartFrames) {
        uint32_t elapsed = gameplayFrames - kRampStartFrames;
        uint32_t duration = kRampPeakFrames - kRampStartFrames;
        if (duration > 0) {
            ramp = smoothstep01((float)elapsed / (float)duration);
        }
    }

    // Peak at 2 min: ~35% faster, ~45% more frequent spawns, +2 concurrent meteors.
    meteorBaseSpeed = baselineMeteorSpeed * (1.0f + ramp * 0.35f);

    int interval = (int)((float)baselineSpawnInterval - ramp * (float)(baselineSpawnInterval - 30));
    meteorSpawnInterval = (uint8_t)max(30, interval);

    maxActiveMeteors = baselineMaxMeteors + (uint8_t)(ramp * (float)(MAX_METEORS - baselineMaxMeteors));
}

void MeteorShowerEffect::initStars() {
    for (uint8_t i = 0; i < MAX_STARS; i++) {
        stars[i].x = (uint16_t)(randomFloat() * playWidth);
        stars[i].y = (uint16_t)(randomFloat() * playHeight);
        stars[i].brightness = 30 + (uint8_t)(randomFloat() * 40);
    }
}

void MeteorShowerEffect::initPlanets() {
    planetCount = 0;
    planetSpawnCounter = 0;
    if (randomFloat() > 0.5f) {
        spawnPlanet();
    }
}

void MeteorShowerEffect::spawnPlanet() {
    if (planetCount >= MAX_PLANETS) return;

    MeteorPlanet& p = planets[planetCount];
    bool fromTop = randomFloat() > 0.5f;

    if (fromTop) {
        p.pos.x = playWidth * 0.2f + randomFloat() * playWidth * 0.6f;
        p.pos.y = -10;
        p.vel.x = (randomFloat() - 0.5f) * 0.08f;
        p.vel.y = 0.04f + randomFloat() * 0.08f;
    } else {
        p.pos.x = randomFloat() > 0.5f ? -10 : playWidth + 10;
        p.pos.y = playHeight * 0.2f + randomFloat() * playHeight * 0.6f;
        p.vel.x = p.pos.x < 0 ? (0.04f + randomFloat() * 0.08f) : (-0.04f - randomFloat() * 0.08f);
        p.vel.y = (randomFloat() - 0.5f) * 0.04f;
    }

    p.size = 6 + randomFloat() * 8;
    p.hue = 120 + (uint8_t)(randomFloat() * 80);
    p.hasRings = randomFloat() > 0.6f;
    planetCount++;
}

void MeteorShowerEffect::updatePlanets() {
    planetSpawnCounter++;
    if (planetSpawnCounter >= 180 && planetCount < MAX_PLANETS) {
        spawnPlanet();
        planetSpawnCounter = 0;
    }

    for (int i = planetCount - 1; i >= 0; i--) {
        planets[i].pos.x += planets[i].vel.x;
        planets[i].pos.y += planets[i].vel.y;

        if (planets[i].pos.y > playHeight + 20 ||
            planets[i].pos.x < -20 ||
            planets[i].pos.x > playWidth + 20 ||
            planets[i].pos.y < -20) {
            if (i < planetCount - 1) {
                planets[i] = planets[planetCount - 1];
            }
            planetCount--;
        }
    }
}

void MeteorShowerEffect::updatePlayer(MeteorBlobResult& blob) {
    if (blob.valid) {
        float smoothing = 0.35f;
        filteredBlobX += (blob.x - filteredBlobX) * smoothing;

        if (fabsf(blob.x - filteredBlobX) < 0.18f) {
            filteredBlobX = (filteredBlobX * 0.7f) + (blob.x * 0.3f);
        }

        playerTargetX = filteredBlobX;
        lastBlobFrame = frameCount;
    } else if (frameCount - lastBlobFrame > missingBlobRecentFrames) {
        playerTargetX = playWidth / 2.0f;
    }

    playerX += (playerTargetX - playerX) * playerSmoothing;

    float halfWidth = playerWidth / 2.0f;
    if (playerX < halfWidth) playerX = halfWidth;
    if (playerX > playWidth - halfWidth) playerX = playWidth - halfWidth;

    if (playerHitTimer > 0) {
        playerHitTimer--;
    }
}

void MeteorShowerEffect::spawnMeteor() {
    uint8_t activeCount = 0;
    for (uint8_t i = 0; i < MAX_METEORS; i++) {
        if (meteors[i].active) activeCount++;
    }
    if (activeCount >= maxActiveMeteors) return;

    float scaleX = (float)playWidth / 64.0f;
    float scaleY = (float)playHeight / 192.0f;
    float margin = 8.0f * scaleX;

    for (uint8_t i = 0; i < MAX_METEORS; i++) {
        if (meteors[i].active) continue;

        GameMeteor& m = meteors[i];
        m.radius = (4.5f + randomFloat() * 2.5f) * scaleX;
        if (m.radius < 3.0f) m.radius = 3.0f;
        float offscreen = m.radius + 10.0f * scaleX;

        // Aim toward a random point in the lower play area.
        float targetX = margin + randomFloat() * (playWidth - margin * 2.0f);
        float targetY = playHeight * 0.55f + randomFloat() * playHeight * 0.35f;

        float entry = randomFloat();
        if (entry < 0.35f) {
            // Mostly straight from above with a mild diagonal.
            m.x = margin + randomFloat() * (playWidth - margin * 2.0f);
            m.y = -offscreen - randomFloat() * 6.0f * scaleY;
        } else if (entry < 0.68f) {
            // From upper-left outside the canvas.
            m.x = -offscreen - randomFloat() * 14.0f * scaleX;
            m.y = -offscreen * 0.4f + randomFloat() * playHeight * 0.22f;
        } else {
            // From upper-right outside the canvas.
            m.x = playWidth + offscreen + randomFloat() * 14.0f * scaleX;
            m.y = -offscreen * 0.4f + randomFloat() * playHeight * 0.22f;
        }

        float dx = targetX - m.x;
        float dy = targetY - m.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < 1.0f) dist = 1.0f;

        float speed = meteorBaseSpeed * (0.9f + randomFloat() * 0.25f);
        m.vx = (dx / dist) * speed;
        m.vy = (dy / dist) * speed;

        // Keep trajectories readable: cap horizontal component (~15–28° off vertical).
        float maxHoriz = fabsf(m.vy) * (0.28f + randomFloat() * 0.22f);
        if (m.vx > maxHoriz) m.vx = maxHoriz;
        if (m.vx < -maxHoriz) m.vx = -maxHoriz;

        // Re-normalize so meteors stay at target speed after the angle clamp.
        float actualSpeed = sqrtf(m.vx * m.vx + m.vy * m.vy);
        if (actualSpeed > 0.001f) {
            float scale = speed / actualSpeed;
            m.vx *= scale;
            m.vy *= scale;
        }

        float colorType = randomFloat();
        if (colorType < 0.35f) {
            m.hue = 12 + (uint8_t)(randomFloat() * 18);
        } else if (colorType < 0.7f) {
            m.hue = 24 + (uint8_t)(randomFloat() * 16);
        } else {
            m.hue = (uint8_t)(randomFloat() * 10);
        }

        m.active = true;
        return;
    }
}

void MeteorShowerEffect::updateMeteors() {
    meteorSpawnCounter++;
    if (meteorSpawnCounter >= meteorSpawnInterval) {
        spawnMeteor();
        meteorSpawnCounter = 0;
    }

    for (uint8_t i = 0; i < MAX_METEORS; i++) {
        if (!meteors[i].active) continue;

        GameMeteor& m = meteors[i];
        m.x += m.vx;
        m.y += m.vy;

        if (m.y - m.radius > playHeight + 15 ||
            m.x < -m.radius - 20 ||
            m.x > playWidth + m.radius + 20) {
            m.active = false;
        }
    }
}

void MeteorShowerEffect::updateBullets() {
    if (bulletCooldown > 0) bulletCooldown--;

    if (handsRaised && bulletCooldown == 0 && !gameOver) {
        for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
            if (!playerBullets[i].active) {
                playerBullets[i].x = playerX;
                playerBullets[i].y = playerY - playerHeight / 2 - 2;
                playerBullets[i].active = true;
                bulletCooldown = bulletCooldownFrames;
                break;
            }
        }
    }

    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active) {
            playerBullets[i].y -= bulletSpeed;
            if (playerBullets[i].y < -5) {
                playerBullets[i].active = false;
            }
        }
    }
}

void MeteorShowerEffect::checkCollisions() {
    for (uint8_t b = 0; b < MAX_PLAYER_BULLETS; b++) {
        if (!playerBullets[b].active) continue;

        for (uint8_t m = 0; m < MAX_METEORS; m++) {
            if (!meteors[m].active) continue;

            float dx = playerBullets[b].x - meteors[m].x;
            float dy = playerBullets[b].y - meteors[m].y;
            float hitDist = meteors[m].radius + 2.0f;

            if (dx * dx + dy * dy < hitDist * hitDist) {
                meteors[m].active = false;
                playerBullets[b].active = false;
                score += 10 + (uint16_t)(meteors[m].radius * 2);
                break;
            }
        }
    }

    float playerHalfW = playerWidth / 2.0f;
    float playerHalfH = playerHeight / 2.0f;

    for (uint8_t i = 0; i < MAX_METEORS; i++) {
        if (!meteors[i].active) continue;

        float dx = meteors[i].x - playerX;
        float dy = meteors[i].y - playerY;
        float hitDist = meteors[i].radius + min(playerHalfW, playerHalfH) * 0.6f;

        if (dx * dx + dy * dy < hitDist * hitDist) {
            meteors[i].active = false;
            playerHitTimer = playerHitFlipFrames;
            lives--;
            if (lives <= 0) {
                gameOver = true;
            }
        }
    }
}

void MeteorShowerEffect::drawGradientBackground() {
    for (uint16_t gy = 0; gy < playHeight; gy++) {
        float gradientFactor = (float)gy / playHeight;

        uint8_t hue = 170 - (uint8_t)(gradientFactor * 25);
        uint8_t sat = 100 + (uint8_t)(gradientFactor * 75);
        uint8_t val = 15 + (uint8_t)(gradientFactor * 35);

        CRGB color;
        hsv2rgb_rainbow(CHSV(hue, sat, val), color);
        uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);

        int16_t mx = gy;
        m_matrix->background->fillRect(mx, 0, 1, playWidth, c565);
    }
}

void MeteorShowerEffect::drawStars() {
    for (uint8_t i = 0; i < MAX_STARS; i++) {
        float twinkle = stars[i].brightness + sin(frameCount * 0.1f + stars[i].x) * 15;
        uint8_t b = constrain((int)twinkle, 20, 80);
        drawGamePixel(stars[i].x, stars[i].y, CRGB(b, b, b));
    }
}

void MeteorShowerEffect::drawPlanets() {
    for (uint8_t i = 0; i < planetCount; i++) {
        MeteorPlanet& p = planets[i];
        int16_t x = (int16_t)p.pos.x;
        int16_t y = (int16_t)p.pos.y;
        int16_t r = (int16_t)(p.size / 2);

        CRGB glowColor;
        hsv2rgb_rainbow(CHSV(p.hue, 50, 50), glowColor);
        glowColor.nscale8(40);
        drawGameCircle(x, y, r + 1, glowColor);

        CRGB bodyColor;
        hsv2rgb_rainbow(CHSV(p.hue, 100, 80), bodyColor);
        bodyColor.nscale8(120);
        drawGameCircle(x, y, r, bodyColor);
    }
}

void MeteorShowerEffect::drawMeteors() {
    for (uint8_t i = 0; i < MAX_METEORS; i++) {
        if (!meteors[i].active) continue;

        GameMeteor& m = meteors[i];
        int16_t x = (int16_t)m.x;
        int16_t y = (int16_t)m.y;
        int16_t r = (int16_t)m.radius;

        float speed = sqrtf(m.vx * m.vx + m.vy * m.vy);
        float dirX = (speed > 0.001f) ? (m.vx / speed) : 0.0f;
        float dirY = (speed > 0.001f) ? (m.vy / speed) : 1.0f;

        // Trail behind the head, drawn first so the ball stays bright on top.
        CRGB tailColor;
        hsv2rgb_rainbow(CHSV(m.hue, 190, 200), tailColor);
        int16_t tailSteps = max(3, r + 2);
        float tailSpacing = max(1.2f, r * 0.55f);
        for (int16_t t = tailSteps; t >= 1; t--) {
            float fade = 1.0f - ((float)t / (float)(tailSteps + 1));
            int16_t tx = (int16_t)(x - dirX * tailSpacing * t);
            int16_t ty = (int16_t)(y - dirY * tailSpacing * t);
            int16_t tr = max(1, r - t / 2);
            CRGB step = tailColor;
            step.nscale8((uint8_t)(fade * fade * 180.0f + 30.0f));
            drawGameCircle(tx, ty, tr, step);
        }

        // Soft outer halo (behind the solid body).
        CRGB haloColor;
        hsv2rgb_rainbow(CHSV(m.hue, 160, 200), haloColor);
        haloColor.nscale8(55);
        drawGameCircle(x, y, r + 1, haloColor);

        // Solid bright ball — uniform fill, no dark core.
        CRGB bodyColor;
        hsv2rgb_rainbow(CHSV(m.hue, 200, 255), bodyColor);
        drawGameCircle(x, y, r, bodyColor);

        // Specular glint on the leading edge.
        if (r > 1) {
            CRGB specColor;
            hsv2rgb_rainbow(CHSV(m.hue + 6, 80, 255), specColor);
            int16_t sx = (int16_t)(x + dirX * r * 0.35f);
            int16_t sy = (int16_t)(y + dirY * r * 0.35f);
            drawGameCircle(sx, sy, max(1, r / 3), specColor);
        }
    }
}

void MeteorShowerEffect::drawBullets() {
    CRGB bulletColor;
    hsv2rgb_rainbow(CHSV(64, 230, 255), bulletColor);

    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active) {
            int16_t x = (int16_t)playerBullets[i].x;
            int16_t y = (int16_t)playerBullets[i].y;
            drawGameRect(x - bulletWidth / 2, y - bulletHeight / 2, bulletWidth, bulletHeight, bulletColor);
        }
    }
}

void MeteorShowerEffect::drawPlayer() {
    int16_t x = (int16_t)playerX;
    int16_t y = (int16_t)playerY;
    const uint8_t (*sprite)[20] = kSpacemanSpriteMove;
    if (playerHitTimer > 0) {
        sprite = kSpacemanSpriteHit;
    } else if (handsRaised) {
        sprite = kSpacemanSpriteBoost;
    }

    int16_t drawW = max<int16_t>(1, (int16_t)playerWidth);
    int16_t drawH = max<int16_t>(1, (int16_t)playerHeight);
    drawSpacemanSpriteScaled(sprite, x, y, drawW, drawH,
                             [this](int16_t gx, int16_t gy, const CRGB& c) { drawGamePixel(gx, gy, c); });
}

void MeteorShowerEffect::drawHUD() {
    uint8_t scoreDots = min(10, (int)(score / 50));
    for (uint8_t i = 0; i < scoreDots; i++) {
        drawGamePixel(1 + i * 2, 1, CRGB::White);
    }

    CRGB lifeColor;
    hsv2rgb_rainbow(CHSV(96, 200, 230), lifeColor);
    for (uint8_t i = 0; i < lives; i++) {
        drawGameRect(playWidth - 3 - i * 4, 1, 2, 2, lifeColor);
    }
}

void MeteorShowerEffect::drawGameOver() {
    m_matrix->background->dim(128);

    CRGB color;
    hsv2rgb_rainbow(CHSV(0, 255, 255), color);

    int16_t cx = playWidth / 2;
    int16_t cy = playHeight / 2;

    for (int8_t i = -4; i <= 4; i++) {
        drawGamePixel(cx + i, cy + i, color);
        drawGamePixel(cx + i, cy - i, color);
    }
}

void MeteorShowerEffect::update() {
    frameCount++;

    updateTofData();

    MeteorBlobResult blob = {0, 0, false};
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
        blob.size = tofInteractionData.blobSize;
        blob.valid = tofInteractionData.blobSize >= minBlobCells;
    }

    if (!gameOver) {
        gameplayFrames++;
        updateDifficultyRamp();
        updatePlayer(blob);
        updateBullets();
        updateMeteors();
        checkCollisions();
    }

    updatePlanets();

    drawGradientBackground();
    drawStars();
    drawPlanets();
    drawMeteors();
    drawBullets();
    drawPlayer();
    drawHUD();

    if (gameOver) {
        drawGameOver();
        if (frameCount % 90 == 0) {
            resetGame();
        }
    }

    if (tofGridReady && tofInteractionData.hasBlob &&
        tofInteractionData.distanceHint == TofDistanceHint::TooClose) {
        drawStepBackProximityWarning(playWidth, playHeight, m_matrix,
                                     [this](int16_t gx, int16_t gy, const CRGB& c) { drawGamePixel(gx, gy, c); });
    }

    if (blob.valid) {
        CRGB indicatorColor;
        hsv2rgb_rainbow(CHSV(40, 200, 230), indicatorColor);
        drawGameCircle((int16_t)blob.x, playHeight - 2, 1, indicatorColor);
    }
}
