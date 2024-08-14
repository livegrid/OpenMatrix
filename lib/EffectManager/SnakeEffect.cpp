#include "SnakeEffect.h"

SnakeEffect::SnakeEffect(Matrix* m) : Effect(m) {}

void SnakeEffect::update() {
    uint32_t currentTime = millis();
    uint32_t elapsedTime = currentTime - lastUpdateTime;
    uint32_t adjustedInterval = baseUpdateInterval / speed;

    if (elapsedTime >= adjustedInterval) {
      m_matrix->background->dim(10);
      moveSnakes();
      drawSnakes();
      lastUpdateTime = currentTime;
    }
}

const char* SnakeEffect::getName() const {
    return "Snake";
}


void SnakeEffect::reset() {
    for (auto& snake : snakes) {
        snake.reset(m_matrix->getXResolution(), m_matrix->getYResolution());
    }
}

void SnakeEffect::moveSnakes() {
    for (auto& snake : snakes) {
        snake.move(m_matrix->getXResolution(), m_matrix->getYResolution());
        if (random(10) > 7) {
            snake.newDirection();
        }
    }
}

void SnakeEffect::drawSnakes() {
    for (auto& snake : snakes) {
        snake.draw(m_matrix, baseColor);
    }
}

// int SnakeEffect::random(int max) {
//     return std::rand() % max;
// }

// Snake implementation
void SnakeEffect::Snake::reset(int maxX, int maxY) {
    direction = Direction::UP;
    for (auto& pixel : pixels) {
        pixel.x = random(maxX);
        pixel.y = random(maxY);
    }
}

void SnakeEffect::Snake::move(int maxX, int maxY) {
    for (int i = SNAKE_LENGTH - 1; i > 0; i--) {
        pixels[i] = pixels[i - 1];
    }

    switch (direction) {
        case Direction::UP:    pixels[0].y = (pixels[0].y + 1) % maxY; break;
        case Direction::DOWN:  pixels[0].y = (pixels[0].y - 1 + maxY) % maxY; break;
        case Direction::LEFT:  pixels[0].x = (pixels[0].x - 1 + maxX) % maxX; break;
        case Direction::RIGHT: pixels[0].x = (pixels[0].x + 1) % maxX; break;
    }
}

void SnakeEffect::Snake::newDirection() {
    direction = static_cast<Direction>(random(4));
}

void SnakeEffect::Snake::draw(Matrix* matrix, const CRGB& color) {
  for (int i = 0; i < SNAKE_LENGTH; i++) {
    CRGB pixelColor = color;
    pixelColor.nscale8(255 - i * (255 / SNAKE_LENGTH));  // Fade out towards the tail
    matrix->background->drawPixel(pixels[i].x, pixels[i].y, pixelColor);
  }
}