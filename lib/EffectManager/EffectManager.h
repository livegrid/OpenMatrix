#pragma once

#include "Effect.h"
#include "NoiseEffect.h"
#include <vector>
#include <string>

class EffectManager {
public:
    EffectManager(Matrix* matrix);
    ~EffectManager();

    void updateCurrentEffect();
    void setEffectByNumber(size_t number);
    void setEffectByName(const std::string& name);
    
    size_t getEffectCount() const;
    const char* getCurrentEffectName() const;

private:
    Matrix* m_matrix;
    std::vector<Effect*> m_effects;
    size_t m_currentEffect = 0;
};
