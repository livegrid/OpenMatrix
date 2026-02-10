#include "SpaceInvadersEffect.h"
#include "../TOFSensor/TOFSensor.h"

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
    : Effect(matrix), tofSensor(sensor) {
    
    // Get matrix dimensions
    playWidth = m_matrix->getXResolution();
    playHeight = m_matrix->getYResolution();
    
    // Scale parameters based on matrix size (default designed for 64x64)
    float scaleX = (float)playWidth / 64.0f;
    float scaleY = (float)playHeight / 64.0f;
    
    // Alien configuration (scaled)
    alienRows = 4;
    alienCols = 5;
    alienSpacingX = (uint16_t)(11 * scaleX);
    alienSpacingY = (uint16_t)(10 * scaleY);
    alienStartY = (uint16_t)(8 * scaleY);
    alienMoveSpeed = max((uint16_t)1, (uint16_t)(1 * scaleX));
    alienDropAmount = (uint16_t)(4 * scaleY);
    alienMoveInterval = 25;  // frames between moves
    alienShootChance = 0.003f;
    alienBulletSpeed = 0.8f * scaleY;
    
    // Player configuration (scaled)
    playerWidth = (uint16_t)(6 * scaleX);
    playerHeight = (uint16_t)(4 * scaleY);
    playerYOffset = (uint16_t)(4 * scaleY);
    playerSmoothing = 0.25f;
    playerHitFlipFrames = 18;
    
    // Bullet configuration (scaled)
    bulletSpeed = 2.0f * scaleY;
    bulletWidth = max(1, (int)(2 * scaleX));
    bulletHeight = max(2, (int)(3 * scaleY));
    bulletCooldownFrames = 15;
    
    // ToF parameters
    tofGridReady = false;
    minDetectionDistance = 1000;
    maxDetectionDistance = 2000;
    tofRotation = 180;
    topRowActive = false;
    minBlobCells = 3;
    missingBlobRecentFrames = 20;
    lastBlobFrame = 0;
    lastBlobSize = 0;
    
    frameCount = 0;
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
}

void SpaceInvadersEffect::setDetectionRange(int16_t minDist, int16_t maxDist) {
    minDetectionDistance = minDist;
    maxDetectionDistance = maxDist;
}

void SpaceInvadersEffect::setTofRotation(uint16_t rotation) {
    tofRotation = rotation;
}

void SpaceInvadersEffect::rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY) {
    if (tofRotation == 90) {
        outX = 7 - y;
        outY = x;
        return;
    }
    if (tofRotation == 180) {
        outX = 7 - x;
        outY = 7 - y;
        return;
    }
    if (tofRotation == 270) {
        outX = y;
        outY = 7 - x;
        return;
    }
    outX = x;
    outY = y;
}

void SpaceInvadersEffect::updateTofData() {
    if (!tofSensor || !tofSensor->isActive()) {
        tofGridReady = false;
        return;
    }
    
    for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
        for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
            uint8_t rx, ry;
            rotateCoordinates(x, y, rx, ry);
            tofGrid[y][x] = tofSensor->getDistance(rx, ry);
        }
    }
    tofGridReady = true;
    updateTopRowActive();
}

void SpaceInvadersEffect::updateTopRowActive() {
    topRowActive = false;
    for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
        int16_t depth = tofGrid[0][x];
        if (depth > minDetectionDistance && depth < maxDetectionDistance) {
            topRowActive = true;
            return;
        }
    }
}

BlobResult SpaceInvadersEffect::findLargestBlob() {
    BlobResult result = {0, 0, false};
    
    if (!tofGridReady) return result;
    
    // Check if cell is active (in detection range)
    auto isActive = [this](uint8_t x, uint8_t y) -> bool {
        int16_t d = tofGrid[y][x];
        return d > minDetectionDistance && d < maxDetectionDistance;
    };
    
    bool visited[TOF_GRID_SIZE][TOF_GRID_SIZE] = {false};
    
    // BFS flood fill to find connected components
    uint8_t bestSize = 0;
    float bestCenterX = 0;
    
    for (uint8_t sy = 0; sy < TOF_GRID_SIZE; sy++) {
        for (uint8_t sx = 0; sx < TOF_GRID_SIZE; sx++) {
            if (visited[sy][sx] || !isActive(sx, sy)) continue;
            
            // Found a new blob, flood fill it
            uint8_t queue[64][2];  // x, y pairs
            uint8_t qHead = 0, qTail = 0;
            uint8_t count = 0;
            float sumX = 0;
            
            queue[qTail][0] = sx;
            queue[qTail][1] = sy;
            qTail++;
            visited[sy][sx] = true;
            
            while (qHead < qTail) {
                uint8_t cx = queue[qHead][0];
                uint8_t cy = queue[qHead][1];
                qHead++;
                
                count++;
                sumX += cx + 0.5f;
                
                // Check 4 neighbors
                const int8_t dx[] = {1, -1, 0, 0};
                const int8_t dy[] = {0, 0, 1, -1};
                
                for (uint8_t d = 0; d < 4; d++) {
                    int8_t nx = cx + dx[d];
                    int8_t ny = cy + dy[d];
                    
                    if (nx < 0 || nx >= TOF_GRID_SIZE || ny < 0 || ny >= TOF_GRID_SIZE) continue;
                    if (visited[ny][nx] || !isActive(nx, ny)) continue;
                    
                    visited[ny][nx] = true;
                    if (qTail < 64) {
                        queue[qTail][0] = nx;
                        queue[qTail][1] = ny;
                        qTail++;
                    }
                }
            }
            
            // Check if this blob is the largest
            if (count >= minBlobCells && count > bestSize) {
                bestSize = count;
                float cellWidth = (float)playWidth / TOF_GRID_SIZE;
                bestCenterX = (sumX / count) * cellWidth;
            }
        }
    }
    
    if (bestSize >= minBlobCells) {
        result.x = bestCenterX;
        result.size = bestSize;
        result.valid = true;
    }
    
    return result;
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
        playerTargetX = blob.x;
        lastBlobFrame = frameCount;
        lastBlobSize = blob.size;
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
    
    // Fire if top row is active and cooldown is done
    if (topRowActive && bulletCooldown == 0 && !gameOver && !gameWon) {
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
    // Deep space gradient
    for (uint16_t y = 0; y < playHeight; y++) {
        float gradientFactor = (float)y / playHeight;
        
        uint8_t hue = 170 - (uint8_t)(gradientFactor * 25);
        uint8_t sat = 100 + (uint8_t)(gradientFactor * 75);
        uint8_t val = 15 + (uint8_t)(gradientFactor * 35);
        
        CRGB color;
        hsv2rgb_rainbow(CHSV(hue, sat, val), color);
        
        for (uint16_t x = 0; x < playWidth; x++) {
            m_matrix->background->drawPixel(x, y, color);
        }
    }
}

void SpaceInvadersEffect::drawStars() {
    for (uint8_t i = 0; i < MAX_STARS; i++) {
        // Twinkle effect
        float twinkle = stars[i].brightness + sin(frameCount * 0.1f + stars[i].x) * 15;
        uint8_t b = constrain((int)twinkle, 20, 80);
        
        CRGB color = CRGB(b, b, b);
        m_matrix->background->drawPixel(stars[i].x, stars[i].y, color);
    }
}

void SpaceInvadersEffect::drawPlanets() {
    for (uint8_t i = 0; i < planetCount; i++) {
        GamePlanet& p = planets[i];
        int16_t x = (int16_t)p.pos.x;
        int16_t y = (int16_t)p.pos.y;
        int16_t r = (int16_t)(p.size / 2);
        
        // Planet glow
        CRGB glowColor;
        hsv2rgb_rainbow(CHSV(p.hue, 50, 50), glowColor);
        glowColor.nscale8(40);
        m_matrix->background->fillCircle(x, y, r + 1, m_matrix->background->color565(glowColor.r, glowColor.g, glowColor.b));
        
        // Planet body
        CRGB bodyColor;
        hsv2rgb_rainbow(CHSV(p.hue, 100, 80), bodyColor);
        bodyColor.nscale8(120);
        m_matrix->background->fillCircle(x, y, r, m_matrix->background->color565(bodyColor.r, bodyColor.g, bodyColor.b));
    }
}

void SpaceInvadersEffect::drawPixelAlien(Alien& alien) {
    const uint8_t* pattern = alien.animFrame == 0 ? 
                             alienPatternA[alien.type] : 
                             alienPatternB[alien.type];
    
    int16_t offsetX = (int16_t)alien.x - 4;  // Center the 8x8 pattern
    int16_t offsetY = (int16_t)alien.y - 4;
    
    CRGB color;
    hsv2rgb_rainbow(CHSV(alien.hue, 200, 220), color);
    
    for (uint8_t row = 0; row < 8; row++) {
        uint8_t rowBits = pattern[row];
        for (uint8_t col = 0; col < 8; col++) {
            if (rowBits & (0x80 >> col)) {  // Check bit from left to right
                int16_t px = offsetX + col;
                int16_t py = offsetY + row;
                
                if (px >= 0 && px < playWidth && py >= 0 && py < playHeight) {
                    m_matrix->background->drawPixel(px, py, color);
                }
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
    // Player bullets (yellow/green)
    CRGB playerBulletColor;
    hsv2rgb_rainbow(CHSV(64, 230, 255), playerBulletColor);
    
    for (uint8_t i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (playerBullets[i].active) {
            int16_t x = (int16_t)playerBullets[i].x;
            int16_t y = (int16_t)playerBullets[i].y;
            m_matrix->background->fillRect(x - bulletWidth/2, y - bulletHeight/2, 
                                           bulletWidth, bulletHeight,
                                           m_matrix->background->color565(playerBulletColor.r, playerBulletColor.g, playerBulletColor.b));
        }
    }
    
    // Alien bullets (red)
    CRGB alienBulletColor;
    hsv2rgb_rainbow(CHSV(0, 200, 255), alienBulletColor);
    
    for (uint8_t i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (alienBullets[i].active) {
            int16_t x = (int16_t)alienBullets[i].x;
            int16_t y = (int16_t)alienBullets[i].y;
            m_matrix->background->fillRect(x - 1, y - 1, 2, 3,
                                           m_matrix->background->color565(alienBulletColor.r, alienBulletColor.g, alienBulletColor.b));
        }
    }
}

void SpaceInvadersEffect::drawPlayer() {
    int16_t x = (int16_t)playerX;
    int16_t y = (int16_t)playerY;
    
    // Choose color based on state
    CRGB color;
    if (playerHitTimer > 0) {
        // Flash red when hit
        if ((playerHitTimer / 3) % 2 == 0) {
            hsv2rgb_rainbow(CHSV(0, 255, 255), color);
        } else {
            hsv2rgb_rainbow(CHSV(0, 255, 128), color);
        }
    } else if (topRowActive) {
        // Brighter when shooting
        hsv2rgb_rainbow(CHSV(96, 200, 255), color);
    } else {
        // Normal green
        hsv2rgb_rainbow(CHSV(96, 180, 200), color);
    }
    
    // Draw simple ship shape (triangle-ish)
    int16_t hw = playerWidth / 2;
    int16_t hh = playerHeight / 2;
    
    // Main body
    m_matrix->background->fillRect(x - hw, y - hh + 1, playerWidth, playerHeight - 1,
                                   m_matrix->background->color565(color.r, color.g, color.b));
    
    // Cockpit (top point)
    m_matrix->background->drawPixel(x, y - hh, color);
    m_matrix->background->drawPixel(x - 1, y - hh, color);
    m_matrix->background->drawPixel(x + 1, y - hh, color);
}

void SpaceInvadersEffect::drawHUD() {
    // Score in top-left (simplified - just draw colored pixels for score indicator)
    // Each 100 points = 1 dot
    uint8_t scoreDots = min(10, (int)(score / 50));
    CRGB scoreColor = CRGB::White;
    for (uint8_t i = 0; i < scoreDots; i++) {
        m_matrix->background->drawPixel(1 + i * 2, 1, scoreColor);
    }
    
    // Lives in top-right (green dots)
    CRGB lifeColor;
    hsv2rgb_rainbow(CHSV(96, 200, 230), lifeColor);
    for (uint8_t i = 0; i < lives; i++) {
        m_matrix->background->fillRect(playWidth - 3 - i * 4, 1, 2, 2,
                                       m_matrix->background->color565(lifeColor.r, lifeColor.g, lifeColor.b));
    }
}

void SpaceInvadersEffect::drawGameOver() {
    // Dim background
    m_matrix->background->dim(128);
    
    // Red "X" pattern in center
    CRGB color;
    hsv2rgb_rainbow(CHSV(0, 255, 255), color);
    
    int16_t cx = playWidth / 2;
    int16_t cy = playHeight / 2;
    
    for (int8_t i = -4; i <= 4; i++) {
        m_matrix->background->drawPixel(cx + i, cy + i, color);
        m_matrix->background->drawPixel(cx + i, cy - i, color);
    }
}

void SpaceInvadersEffect::drawWinScreen() {
    // Green checkmark or victory pattern
    CRGB color;
    hsv2rgb_rainbow(CHSV(96, 255, 255), color);
    
    int16_t cx = playWidth / 2;
    int16_t cy = playHeight / 2;
    
    // Simple checkmark
    m_matrix->background->drawPixel(cx - 3, cy, color);
    m_matrix->background->drawPixel(cx - 2, cy + 1, color);
    m_matrix->background->drawPixel(cx - 1, cy + 2, color);
    m_matrix->background->drawPixel(cx, cy + 1, color);
    m_matrix->background->drawPixel(cx + 1, cy, color);
    m_matrix->background->drawPixel(cx + 2, cy - 1, color);
    m_matrix->background->drawPixel(cx + 3, cy - 2, color);
}

void SpaceInvadersEffect::update() {
    frameCount++;
    
    // Update ToF sensor data
    updateTofData();
    
    // Find blob for player control
    BlobResult blob = findLargestBlob();
    
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
    
    // Draw blob indicator at bottom
    if (blob.valid) {
        CRGB indicatorColor;
        hsv2rgb_rainbow(CHSV(40, 200, 230), indicatorColor);
        m_matrix->background->fillCircle((int16_t)blob.x, playHeight - 2, 1,
                                         m_matrix->background->color565(indicatorColor.r, indicatorColor.g, indicatorColor.b));
    }
}
