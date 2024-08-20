#include "GameofLifeEffect.h"

GameofLifeEffect::GameofLifeEffect(Matrix* m) : Effect(m) {
    WORLD_WIDTH = m->getXResolution() / CELL_SIZE;
    WORLD_HEIGHT = m->getYResolution() / CELL_SIZE;
    world.resize(WORLD_WIDTH, std::vector<Cell>(WORLD_HEIGHT));
    reset();
}

void GameofLifeEffect::update() {
    uint32_t currentTime = millis();
    uint32_t elapsedTime = currentTime - lastUpdateTime;
    uint32_t adjustedInterval = baseUpdateInterval / speed;

    if (elapsedTime >= adjustedInterval) {
        updateWorld();
      lastUpdateTime = currentTime;
    }

    // Display current generation
    for (int i = 0; i < WORLD_WIDTH; i++) {
        for (int j = 0; j < WORLD_HEIGHT; j++) {
          CRGB color = baseColor;
          color.nscale8(world[i][j].brightness);
          m_matrix->background->drawPixel(i * CELL_SIZE, j * CELL_SIZE, color);
        }
    }
}

const char* GameofLifeEffect::getName() const {
    return "Game of Life";
}

void GameofLifeEffect::reset() {
    randomFillWorld();
    generation = 0;
}

void GameofLifeEffect::randomFillWorld() {
    for (int i = 0; i < WORLD_WIDTH; i++) {
        for (int j = 0; j < WORLD_HEIGHT; j++) {
            if (random(100) < density) {
                world[i][j] = {true, true, static_cast<uint8_t>(random(64)), 255};
            } else {
                world[i][j] = {false, false, 0, 0};
            }
        }
    }
}

int GameofLifeEffect::neighbours(int x, int y) {
    return (world[(x + 1) % WORLD_WIDTH][y].prev) +
           (world[x][(y + 1) % WORLD_HEIGHT].prev) +
           (world[(x + WORLD_WIDTH - 1) % WORLD_WIDTH][y].prev) +
           (world[x][(y + WORLD_HEIGHT - 1) % WORLD_HEIGHT].prev) +
           (world[(x + 1) % WORLD_WIDTH][(y + 1) % WORLD_HEIGHT].prev) +
           (world[(x + WORLD_WIDTH - 1) % WORLD_WIDTH][(y + 1) % WORLD_HEIGHT].prev) +
           (world[(x + WORLD_WIDTH - 1) % WORLD_WIDTH][(y + WORLD_HEIGHT - 1) % WORLD_HEIGHT].prev) +
           (world[(x + 1) % WORLD_WIDTH][(y + WORLD_HEIGHT - 1) % WORLD_HEIGHT].prev);
}

void GameofLifeEffect::updateWorld() {
    // Birth and death cycle
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y].brightness > 0 && !world[x][y].prev)
                world[x][y].brightness *= 0.9;
            
            int count = neighbours(x, y);
            if (count == 3 && !world[x][y].prev) {
                // A new cell is born
                world[x][y].alive = true;
                world[x][y].hue += 2;
                world[x][y].brightness = 255;
            } else if ((count < 2 || count > 3) && world[x][y].prev) {
                // Cell dies
                world[x][y].alive = false;
            }
        }
    }

    // Copy next generation into place
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            world[x][y].prev = world[x][y].alive;
        }
    }

    generation++;
    if (generation >= 256)
        generation = 0;
}