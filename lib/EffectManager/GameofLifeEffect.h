#pragma once

#include "Effect.h"

class GameofLifeEffect : public Effect {
private:
    static constexpr int CELL_SIZE = 1;
    int WORLD_WIDTH;
    int WORLD_HEIGHT;
    float speed = 1;

    struct Cell {
        bool alive : 1;
        bool prev : 1;
        uint8_t hue : 6;
        uint8_t brightness;
    };

    std::vector<std::vector<Cell>> world;
    uint32_t lastUpdateTime = 0;
    uint16_t updateInterval = 100; // Update interval in milliseconds
    int generation = 0;
    unsigned int density = 50;

    void randomFillWorld();
    int neighbours(int x, int y);
    void updateWorld();

public:
    GameofLifeEffect(Matrix* m);

    void update() override;
    const char* getName() const override;
    void reset() override;
};