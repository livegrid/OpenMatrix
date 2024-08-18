#include "LSystemEffect.h"
#include <cmath>

LSystemEffect::LSystemEffect(Matrix* m) : Effect(m) {
    reset();
}

void LSystemEffect::update() {
    m_matrix->background->clear();

    if (millis() - lastUpdateMs > 30) {  // Update every 50ms
        lastUpdateMs = millis();

        if (growthPercent < 1) {
            growthPercent += growthRate / (currGeneration + growthPercent);
        } else {
            generate();
        }

        drawLSystem();
    }
}

const char* LSystemEffect::getName() const {
    return "LSystem";
}

void LSystemEffect::reset() {
    word = "X";
    maxGeneration = 7;
    currGeneration = 0;
    growthPercent = 1;
    growthRate = 0.2;
    lastUpdateMs = 0;
}

void LSystemEffect::generate() {
    if (currGeneration == maxGeneration) {
        currGeneration = 0;
        word = "X";
    } else {
        std::string next = "";
        for (char c : word) {
            next += applyRules(c);
        }
        word = next;
        currGeneration++;
    }
    growthPercent = 0;
}

std::string LSystemEffect::applyRules(char c) {
    switch (c) {
        case 'X':
            return random(100) < 50 ? "F[+X][-X]FX" : "F[-X]FX";
        case 'F':
            return random(100) < 85 ? "FF" : "F";
        default:
            return std::string(1, c);
    }
}

void LSystemEffect::drawLSystem() {
    int x = VIRTUAL_RES_X / 2;
    int y = VIRTUAL_RES_Y - 1;
    float angle = -M_PI / 2;  // Start growing upwards
    std::vector<std::pair<int, int>> stack;

    for (char c : word) {
        switch (c) {
            case 'F':
                {
                    int newX = x + round(cos(angle) * 2 * growthPercent);
                    int newY = y + round(sin(angle) * 2 * growthPercent);
                    m_matrix->background->drawLine(x, y, newX, newY, CRGB(0, 255, 0));
                    x = newX;
                    y = newY;
                }
                break;
            case '+':
                angle -= M_PI / 4 * growthPercent;
                break;
            case '-':
                angle += M_PI / 4 * growthPercent;
                break;
            case '[':
                stack.push_back({x, y});
                break;
            case ']':
                if (!stack.empty()) {
                    auto [savedX, savedY] = stack.back();
                    stack.pop_back();
                    x = savedX;
                    y = savedY;
                }
                break;
        }
    }
}