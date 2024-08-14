#pragma once

#include "Effect.h"

class SnakeEffect : public Effect {
private:
  uint32_t lastUpdateTime = 0;
  float speed = 1;

public:
  SnakeEffect(Matrix* m);

  // Declare public methods here
  void update() override;

  const char* getName() const override;


    static constexpr int SNAKE_COUNT = 6;
    static constexpr int SNAKE_LENGTH = 16;

    enum class Direction { UP, DOWN, LEFT, RIGHT };

    struct Pixel {
        int x;
        int y;
    };

    class Snake {
    public:
        void reset(int maxX, int maxY);
        void move(int maxX, int maxY);
        void newDirection();
        void draw(Matrix* , const CRGB&);

    private:
        std::array<Pixel, SNAKE_LENGTH> pixels;
        Direction direction;
    };

    std::array<Snake, SNAKE_COUNT> snakes;
    uint8_t initialHue;

    void reset();
    void moveSnakes();
    void drawSnakes();

    // Helper function to get a random number
    static int random(int max) {
        return rand() % max;
    }
};