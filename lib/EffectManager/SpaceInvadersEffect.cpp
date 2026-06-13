#include "SpaceInvadersEffect.h"
#include "SpacemanSprites.h"
#include "StepBackSprite.h"
#include "../TOFSensor/TOFSensor.h"
#include <math.h>

namespace {
/** Digital orientation tweak for this effect vs global TOF frame (after physical rotation in TOFSensor). */
constexpr int16_t kTofEffectRotationDeg = 0;  // 0 / 90 / 180 / 270
}  // namespace

// Static random seed
static uint32_t gameNoiseSeed = 54321;

float SpaceInvadersEffect::randomFloat() {
    gameNoiseSeed = gameNoiseSeed * 1103515245 + 12345;
    return (float)(gameNoiseSeed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

// Alien pixel patterns (8x8) - each bit represents a pixel
// Pattern A (frame 0) for each alien type
const uint8_t SpaceInvadersEffect::alienPatternA[3][8] = {
    // Type 0: Squid
    {0b00011000, 0b00111100, 0b01111110, 0b11011011, 0b11111111, 0b00100100, 0b01011010, 0b10100101},
    // Type 1: Crab  
    {0b00100100, 0b00011000, 0b00111100, 0b01100110, 0b11111111, 0b10111101, 0b10100101, 0b00011000},
    // Type 2: Octopus
    {0b00011000, 0b00111100, 0b01111110, 0b11011011, 0b11111111, 0b01100110, 0b01000010, 0b10000001}
};

// Pattern B (frame 1) for animation
const uint8_t SpaceInvadersEffect::alienPatternB[3][8] = {
    // Type 0: Squid - legs in
    {0b00011000, 0b00111100, 0b01111110, 0b11011011, 0b11111111, 0b00100100, 0b01011010, 0b01000010},
    // Type 1: Crab - claws down
    {0b00100100, 0b10011001, 0b10111101, 0b11100111, 0b11111111, 0b00111100, 0b01000010, 0b00100100},
    // Type 2: Octopus - tentacles out
    {0b00011000, 0b00111100, 0b01111110, 0b11011011, 0b11111111, 0b00100100, 0b01011010, 0b01000010}
};

// Alien colors by row (FastLED hue values: 0=red, 32=orange, 96=green, 160=blue)
const uint8_t SpaceInvadersEffect::alienHues[4] = {0, 24, 96, 192};

SpaceInvadersEffect::SpaceInvadersEffect(Matrix* matrix, TOFSensor* sensor)
    : Effect(matrix), tofSensor(sensor), tofInteraction(nullptr), tofInteractionData{} {
    
    // Matrix is landscape; game plays in portrait (rotated 90°)
    matrixWidth = m_matrix->getXResolution();
    matrixHeight = m_matrix->getYResolution();
    playWidth = matrixHeight;    // narrow dimension = player left/right
    playHeight = matrixWidth;    // wide dimension = alien descent
    
    // Scale relative to the JS reference portrait dimensions (64×192)
    float scaleX = (float)playWidth / 64.0f;
    float scaleY = (float)playHeight / 192.0f;
    
    // Alien configuration (tuned for portrait play area)
    alienRows = 4;
    alienCols = 5;
    alienSpacingX = (uint16_t)(11 * scaleX);
    alienSpacingY = (uint16_t)(12 * scaleY);
    alienStartY = (uint16_t)(22 * scaleY);
    alienMoveSpeed = max((uint16_t)1, (uint16_t)(1 * scaleX));
    alienDropAmount = (uint16_t)(6 * scaleY);
    alienMoveInterval = 30;
    alienShootChance = 0.001f;
    alienBulletSpeed = 2.0f * scaleY;
    
    // Player configuration (scaled)
    playerWidth = (uint16_t)(10 * scaleX);
    playerHeight = (uint16_t)(14 * scaleY);
    playerYOffset = (uint16_t)(7 * scaleY);
    playerSmoothing = 0.28f;
    playerHitFlipFrames = 18;
    
    // Bullet configuration (scaled)
    bulletSpeed = 4.0f * scaleY;
    bulletWidth = max(1, (int)(2 * scaleX));
    bulletHeight = max(2, (int)(4 * scaleY));
    bulletCooldownFrames = 18;
    
    // ToF parameters
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
    
    reset();
}

void SpaceInvadersEffect::reset() {
    frameCount = 0;
    initStars();
    initPlanets();
    resetGame();
    m_matrix->background->fillScreen(0);
}

const char* SpaceInvadersEffect::getName() const {
    return "SpaceInvaders";
}

void SpaceInvadersEffect::setTofSensor(TOFSensor* sensor) {
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

void SpaceInvadersEffect::setDetectionRange(int16_t minDist, int16_t maxDist) {
    minDetectionDistance = minDist;
    maxDetectionDistance = maxDist;
    if (tofInteraction) {
        tofInteraction->setDistanceRange(minDist, maxDist);
    }
}

// Portrait game coordinate (gx, gy) → landscape matrix coordinate.
// Vertically flipped to match the working orientation on hardware.
void SpaceInvadersEffect::drawGamePixel(int16_t gx, int16_t gy, const CRGB& color) {
    if (gx < 0 || gx >= (int16_t)playWidth || gy < 0 || gy >= (int16_t)playHeight) return;
    int16_t mx = gy;
    int16_t my = gx;
    m_matrix->background->drawPixel(mx, my, color);
}

void SpaceInvadersEffect::drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color) {
    // Game rect (gx, gy, gw, gh) → rotated matrix rect
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillRect(mx, my, gh, gw, c565);
}

void SpaceInvadersEffect::drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color) {
    int16_t mx = gy;
    int16_t my = gx;
    uint16_t c565 = m_matrix->background->color565(color.r, color.g, color.b);
    m_matrix->background->fillCircle(mx, my, r, c565);
}

void SpaceInvadersEffect::updateTofData() {
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

void SpaceInvadersEffect::resetGame() {
    resetPlayer();
    resetAliens();
    
    // Clear bullets
    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
        playerBullets[i].active = false;
    }
    for (uint8_t i = 0; i < MAX_ALIEN_BULLETS; i++) {
        alienBullets[i].active = false;
    }
    
    bulletCooldown = 0;
    score = 0;
    lives = 3;
    gameOver = false;
    gameWon = false;
    level = 1;
}

void SpaceInvadersEffect::resetPlayer() {
    playerX = playWidth / 2.0f;
    playerTargetX = playerX;
    filteredBlobX = playerX;
    playerY = playHeight - playerYOffset - playerHeight / 2.0f;
    playerHitTimer = 0;
}

void SpaceInvadersEffect::resetAliens() {
    alienCount = 0;
    alienDirection = 1;
    alienMoveCounter = 0;
    
    // Clear alien bullets
    for (uint8_t i = 0; i < MAX_ALIEN_BULLETS; i++) {
        alienBullets[i].active = false;
    }
    
    // Calculate total width and starting X
    float totalWidth = (alienCols - 1) * alienSpacingX;
    float startX = (playWidth - totalWidth) / 2.0f;
    
    for (uint8_t row = 0; row < alienRows && alienCount < MAX_ALIENS; row++) {
        for (uint8_t col = 0; col < alienCols && alienCount < MAX_ALIENS; col++) {
            Alien& a = aliens[alienCount];
            a.x = startX + col * alienSpacingX;
            a.y = alienStartY + row * alienSpacingY;
            a.alive = true;
            a.type = row % 3;
            a.hue = alienHues[row % 4];
            a.animFrame = 0;
            alienCount++;
        }
    }
}

void SpaceInvadersEffect::nextLevel() {
    level++;
    resetAliens();
    
    // Clear bullets
    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
        playerBullets[i].active = false;
    }
    for (uint8_t i = 0; i < MAX_ALIEN_BULLETS; i++) {
        alienBullets[i].active = false;
    }
    
    // Increase difficulty
    if (alienMoveInterval > 10) {
        alienMoveInterval -= 2;
    }
    alienShootChance += 0.001f;
}

void SpaceInvadersEffect::initStars() {
    for (uint8_t i = 0; i < MAX_STARS; i++) {
        stars[i].x = (uint16_t)(randomFloat() * playWidth);
        stars[i].y = (uint16_t)(randomFloat() * playHeight);
        stars[i].brightness = 30 + (uint8_t)(randomFloat() * 40);
    }
}

void SpaceInvadersEffect::initPlanets() {
    planetCount = 0;
    planetSpawnCounter = 0;
    
    if (randomFloat() > 0.5f) {
        spawnPlanet();
    }
}

void SpaceInvadersEffect::spawnPlanet() {
    if (planetCount >= MAX_PLANETS) return;
    
    GamePlanet& p = planets[planetCount];
    bool fromTop = randomFloat() > 0.5f;
    
    if (fromTop) {
        p.pos.x = playWidth * 0.2f + randomFloat() * playWidth * 0.6f;
        p.pos.y = -10;
        p.vel.x = (randomFloat() - 0.5f) * 0.1f;
        p.vel.y = 0.05f + randomFloat() * 0.1f;
    } else {
        p.pos.x = randomFloat() > 0.5f ? -10 : playWidth + 10;
        p.pos.y = playHeight * 0.2f + randomFloat() * playHeight * 0.6f;
        p.vel.x = p.pos.x < 0 ? (0.05f + randomFloat() * 0.1f) : (-0.05f - randomFloat() * 0.1f);
        p.vel.y = (randomFloat() - 0.5f) * 0.05f;
    }
    
    p.size = 6 + randomFloat() * 8;
    p.hue = 120 + (uint8_t)(randomFloat() * 80);
    p.hasRings = randomFloat() > 0.6f;
    
    planetCount++;
}

void SpaceInvadersEffect::updatePlanets() {
    planetSpawnCounter++;
    if (planetSpawnCounter >= 150 && planetCount < MAX_PLANETS) {
        spawnPlanet();
        planetSpawnCounter = 0;
    }
    
    for (int i = planetCount - 1; i >= 0; i--) {
        planets[i].pos.x += planets[i].vel.x;
        planets[i].pos.y += planets[i].vel.y;
        
        // Remove off-screen planets
        if (planets[i].pos.y > playHeight + 20 || 
            planets[i].pos.x < -20 || 
            planets[i].pos.x > playWidth + 20 ||
            planets[i].pos.y < -20) {
            // Remove by swapping with last
            if (i < planetCount - 1) {
                planets[i] = planets[planetCount - 1];
            }
            planetCount--;
        }
    }
}

void SpaceInvadersEffect::updatePlayer(BlobResult& blob) {
    if (blob.valid) {
        // Temporal filtering on blob center reduces jitter from ToF noise spikes.
        float smoothing = 0.35f;
        filteredBlobX += (blob.x - filteredBlobX) * smoothing;

        // Ignore micro-jitter around current filtered point.
        if (fabsf(blob.x - filteredBlobX) < 0.18f) {
            filteredBlobX = (filteredBlobX * 0.7f) + (blob.x * 0.3f);
        }

        playerTargetX = filteredBlobX;
        lastBlobFrame = frameCount;
    } else if (frameCount - lastBlobFrame > missingBlobRecentFrames) {
        // Return to center if no blob detected for a while
        playerTargetX = playWidth / 2.0f;
    }
    
    // Smooth movement towards target
    playerX += (playerTargetX - playerX) * playerSmoothing;
    
    // Clamp to screen bounds
    float halfWidth = playerWidth / 2.0f;
    if (playerX < halfWidth) playerX = halfWidth;
    if (playerX > playWidth - halfWidth) playerX = playWidth - halfWidth;
    
    // Update hit timer
    if (playerHitTimer > 0) {
        playerHitTimer--;
    }
}

void SpaceInvadersEffect::fireBullet() {
    // Find inactive bullet slot
    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (!playerBullets[i].active) {
            playerBullets[i].x = playerX;
            playerBullets[i].y = playerY - playerHeight / 2 - 2;
            playerBullets[i].active = true;
            playerBullets[i].isPlayerBullet = true;
            bulletCooldown = bulletCooldownFrames;
            return;
        }
    }
}

void SpaceInvadersEffect::updateBullets() {
    if (bulletCooldown > 0) bulletCooldown--;
    
    // Fire when hand-raise gesture is stably detected.
    if (handsRaised && bulletCooldown == 0 && !gameOver && !gameWon) {
        fireBullet();
    }
    
    // Update player bullets (move up)
    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active) {
            playerBullets[i].y -= bulletSpeed;
            if (playerBullets[i].y < -5) {
                playerBullets[i].active = false;
            }
        }
    }
}

void SpaceInvadersEffect::updateAliens() {
    alienMoveCounter++;
    
    if (alienMoveCounter >= alienMoveInterval) {
        alienMoveCounter = 0;
        
        // Find boundaries of alive aliens
        float minX = playWidth;
        float maxX = 0;
        bool anyAlive = false;
        
        for (uint8_t i = 0; i < alienCount; i++) {
            if (!aliens[i].alive) continue;
            anyAlive = true;
            if (aliens[i].x < minX) minX = aliens[i].x;
            if (aliens[i].x > maxX) maxX = aliens[i].x;
        }
        
        if (!anyAlive) return;
        
        // Check if hit edge
        bool hitEdge = (alienDirection > 0 && maxX > playWidth - 6) ||
                       (alienDirection < 0 && minX < 6);
        
        if (hitEdge) {
            // Reverse direction and drop down
            alienDirection *= -1;
            for (uint8_t i = 0; i < alienCount; i++) {
                if (aliens[i].alive) {
                    aliens[i].y += alienDropAmount;
                }
            }
        } else {
            // Move horizontally
            for (uint8_t i = 0; i < alienCount; i++) {
                if (aliens[i].alive) {
                    aliens[i].x += alienDirection * alienMoveSpeed * 2;
                }
            }
        }
        
        // Toggle animation frame
        for (uint8_t i = 0; i < alienCount; i++) {
            aliens[i].animFrame = (aliens[i].animFrame + 1) % 2;
        }
    }
    
    // Alien shooting
    for (uint8_t i = 0; i < alienCount; i++) {
        if (!aliens[i].alive) continue;
        
        if (randomFloat() < alienShootChance) {
            // Find inactive bullet slot
            for (uint8_t j = 0; j < MAX_ALIEN_BULLETS; j++) {
                if (!alienBullets[j].active) {
                    alienBullets[j].x = aliens[i].x;
                    alienBullets[j].y = aliens[i].y + 4;
                    alienBullets[j].active = true;
                    alienBullets[j].isPlayerBullet = false;
                    break;
                }
            }
        }
    }
    
    // Check if aliens reached bottom
    for (uint8_t i = 0; i < alienCount; i++) {
        if (aliens[i].alive && aliens[i].y > playHeight - 15) {
            gameOver = true;
        }
    }
}

void SpaceInvadersEffect::updateAlienBullets() {
    for (uint8_t i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (alienBullets[i].active) {
            alienBullets[i].y += alienBulletSpeed;
            if (alienBullets[i].y > playHeight + 5) {
                alienBullets[i].active = false;
            }
        }
    }
}

void SpaceInvadersEffect::checkCollisions() {
    // Player bullets vs aliens
    for (uint8_t b = 0; b < MAX_PLAYER_BULLETS; b++) {
        if (!playerBullets[b].active) continue;
        
        for (uint8_t a = 0; a < alienCount; a++) {
            if (!aliens[a].alive) continue;
            
            float dx = playerBullets[b].x - aliens[a].x;
            float dy = playerBullets[b].y - aliens[a].y;
            float hitDist = 5;  // Collision radius
            
            if (abs(dx) < hitDist && abs(dy) < hitDist) {
                aliens[a].alive = false;
                playerBullets[b].active = false;
                
                // Score based on row (higher rows = more points)
                uint8_t row = a / alienCols;
                score += 10 * (alienRows - row);
                break;
            }
        }
    }
    
    // Alien bullets vs player
    float playerHalfW = playerWidth / 2.0f;
    float playerHalfH = playerHeight / 2.0f;
    
    for (uint8_t i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (!alienBullets[i].active) continue;
        
        float dx = alienBullets[i].x - playerX;
        float dy = alienBullets[i].y - playerY;
        
        if (abs(dx) < playerHalfW + 2 && abs(dy) < playerHalfH + 2) {
            alienBullets[i].active = false;
            playerHitTimer = playerHitFlipFrames;
            lives--;
            if (lives <= 0) {
                gameOver = true;
            }
        }
    }
    
    // Aliens vs player (direct collision)
    for (uint8_t i = 0; i < alienCount; i++) {
        if (!aliens[i].alive) continue;
        
        float dx = aliens[i].x - playerX;
        float dy = aliens[i].y - playerY;
        float hitDist = playerHalfW + 4;
        
        if (dx * dx + dy * dy < hitDist * hitDist) {
            gameOver = true;
        }
    }
}

void SpaceInvadersEffect::checkWinCondition() {
    for (uint8_t i = 0; i < alienCount; i++) {
        if (aliens[i].alive) return;
    }
    gameWon = true;
}

void SpaceInvadersEffect::drawGradientBackground() {
    // Deep space gradient - each game row has uniform color.
    // After 90° rotation, each game row becomes a matrix column.
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

void SpaceInvadersEffect::drawStars() {
    for (uint8_t i = 0; i < MAX_STARS; i++) {
        float twinkle = stars[i].brightness + sin(frameCount * 0.1f + stars[i].x) * 15;
        uint8_t b = constrain((int)twinkle, 20, 80);
        
        CRGB color = CRGB(b, b, b);
        drawGamePixel(stars[i].x, stars[i].y, color);
    }
}

void SpaceInvadersEffect::drawPlanets() {
    for (uint8_t i = 0; i < planetCount; i++) {
        GamePlanet& p = planets[i];
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

void SpaceInvadersEffect::drawPixelAlien(Alien& alien) {
    const uint8_t* pattern = alien.animFrame == 0 ? 
                             alienPatternA[alien.type] : 
                             alienPatternB[alien.type];
    
    int16_t offsetX = (int16_t)alien.x - 4;
    int16_t offsetY = (int16_t)alien.y - 4;
    
    CRGB color;
    hsv2rgb_rainbow(CHSV(alien.hue, 200, 220), color);
    
    for (uint8_t row = 0; row < 8; row++) {
        uint8_t rowBits = pattern[row];
        for (uint8_t col = 0; col < 8; col++) {
            if (rowBits & (0x80 >> col)) {
                drawGamePixel(offsetX + col, offsetY + row, color);
            }
        }
    }
}

void SpaceInvadersEffect::drawAliens() {
    for (uint8_t i = 0; i < alienCount; i++) {
        if (aliens[i].alive) {
            drawPixelAlien(aliens[i]);
        }
    }
}

void SpaceInvadersEffect::drawBullets() {
    CRGB playerBulletColor;
    hsv2rgb_rainbow(CHSV(64, 230, 255), playerBulletColor);
    
    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active) {
            int16_t x = (int16_t)playerBullets[i].x;
            int16_t y = (int16_t)playerBullets[i].y;
            drawGameRect(x - bulletWidth/2, y - bulletHeight/2, bulletWidth, bulletHeight, playerBulletColor);
        }
    }
    
    CRGB alienBulletColor;
    hsv2rgb_rainbow(CHSV(0, 200, 255), alienBulletColor);
    
    for (uint8_t i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (alienBullets[i].active) {
            int16_t x = (int16_t)alienBullets[i].x;
            int16_t y = (int16_t)alienBullets[i].y;
            drawGameRect(x - 1, y - 1, 2, 3, alienBulletColor);
        }
    }
}

void SpaceInvadersEffect::drawPlayer() {
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

void SpaceInvadersEffect::drawHUD() {
    uint8_t scoreDots = min(10, (int)(score / 50));
    CRGB scoreColor = CRGB::White;
    for (uint8_t i = 0; i < scoreDots; i++) {
        drawGamePixel(1 + i * 2, 1, scoreColor);
    }
    
    CRGB lifeColor;
    hsv2rgb_rainbow(CHSV(96, 200, 230), lifeColor);
    for (uint8_t i = 0; i < lives; i++) {
        drawGameRect(playWidth - 3 - i * 4, 1, 2, 2, lifeColor);
    }
}

void SpaceInvadersEffect::drawGameOver() {
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

void SpaceInvadersEffect::drawWinScreen() {
    CRGB color;
    hsv2rgb_rainbow(CHSV(96, 255, 255), color);
    
    int16_t cx = playWidth / 2;
    int16_t cy = playHeight / 2;
    
    drawGamePixel(cx - 3, cy, color);
    drawGamePixel(cx - 2, cy + 1, color);
    drawGamePixel(cx - 1, cy + 2, color);
    drawGamePixel(cx, cy + 1, color);
    drawGamePixel(cx + 1, cy, color);
    drawGamePixel(cx + 2, cy - 1, color);
    drawGamePixel(cx + 3, cy - 2, color);
}

void SpaceInvadersEffect::update() {
    frameCount++;
    
    // Update ToF sensor data
    updateTofData();

    BlobResult blob = {0, 0, false};
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
    
    // Game logic
    if (!gameOver && !gameWon) {
        updatePlayer(blob);
        updateBullets();
        updateAliens();
        updateAlienBullets();
        checkCollisions();
        checkWinCondition();
    }
    
    // Background
    updatePlanets();
    
    // Drawing
    drawGradientBackground();
    drawStars();
    drawPlanets();
    drawAliens();
    drawBullets();
    drawPlayer();
    drawHUD();
    
    if (gameOver) {
        drawGameOver();
        // Auto-restart after delay
        if (frameCount % 90 == 0) {
            resetGame();
        }
    } else if (gameWon) {
        drawWinScreen();
        // Auto-advance to next level after delay
        if (frameCount % 60 == 0) {
            nextLevel();
            gameWon = false;
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
