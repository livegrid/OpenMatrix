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
    struct Rule {
        std::string rule;
        float prob;
    };

    const float len = 1.0f;

    std::vector<Rule> xRules = {
        {"F[+X][-X]FX", 0.5},
        {"F[-X]FX", 0.05},
        {"F[+X]FX", 0.05},
        {"F[++X][-X]FX", 0.1},
        {"F[+X][--X]FX", 0.1},
        {"F[+X][-X]FXA", 0.1},
        {"F[+X][-X]FXB", 0.1}
    };

    std::vector<Rule> fRules = {
        {"FF", 0.85},
        {"FFF", 0.05},
        {"F", 0.1}
    };

    std::string word;
    int maxGeneration;
    int currGeneration;
    float growthPercent;
    float growthRate;
    float scale;
    int fullGrowthDelay;
    static const int FULL_GROWTH_DELAY_MAX = 600; // Adjust as needed

    void generate();
    void drawLSystem();
    std::string chooseOne(const std::vector<Rule>& rules);
    std::string applyRules(char c);
};

#endif // LSYSTEM_EFFECT_H