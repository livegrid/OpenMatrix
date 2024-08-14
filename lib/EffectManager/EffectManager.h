#pragma once

#include "Effect.h"

#include "NoiseEffect.h"
#include "SnakeEffect.h"
#include "GameofLifeEffect.h"
#include "FlockEffect.h"

#include <vector>
#include <string>

class EffectManager {
public:
    EffectManager(Matrix* matrix);
    ~EffectManager();

    void updateCurrentEffect();
    void setEffect(size_t number);
    void setEffect(const std::string& name);
    void nextEffect();
    void prevEffect();
    
    size_t getEffectCount() const;
    const char* getCurrentEffectName() const;
    uint8_t getCurrentEffect() const;

private:
    Matrix* m_matrix;
    std::vector<Effect*> m_effects;
    size_t m_currentEffect = 0;
};