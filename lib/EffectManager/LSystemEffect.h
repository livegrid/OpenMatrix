#ifndef LSYSTEM_EFFECT_H
#define LSYSTEM_EFFECT_H

#include "Effect.h"
#include <vector>
#include <string>

class LSystemEffect : public Effect {
public:
    LSystemEffect(Matrix* m);
    void update() override;
    const char* getName() const override;
    void reset();

private:
    std::string word;
    int maxGeneration;
    int currGeneration;
    float growthPercent;
    float growthRate;
    unsigned long lastUpdateMs;

    void generate();
    void drawLSystem();
    std::string applyRules(char c);
};

#endif // LSYSTEM_EFFECT_H