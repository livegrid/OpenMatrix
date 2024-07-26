#ifndef EFFECT_MANAGER_H
#define EFFECT_MANAGER_H

#include "Effect.h"
#include "NoiseEffect.h"
#include <vector>
#include <string>

class EffectManager {
public:
    EffectManager(Matrix* matrix) : m_matrix(matrix) {
        m_effects.push_back(new NoiseEffect(matrix));
        // Add other effects here as you create them
    }

    ~EffectManager() {
        for (auto effect : m_effects) {
            delete effect;
        }
    }

    void updateCurrentEffect() {
        if (m_currentEffect < m_effects.size()) {
            m_effects[m_currentEffect]->update();
        }
    }

    void setEffectByNumber(size_t number) {
        if (number < m_effects.size()) {
            m_currentEffect = number;
        }
    }

    void setEffectByName(const std::string& name) {
        for (size_t i = 0; i < m_effects.size(); ++i) {
            if (name == m_effects[i]->getName()) {
                m_currentEffect = i;
                return;
            }
        }
    }

    size_t getEffectCount() const {
        return m_effects.size();
    }

    const char* getCurrentEffectName() const {
        if (m_currentEffect < m_effects.size()) {
            return m_effects[m_currentEffect]->getName();
        }
        return "None";
    }

private:
    Matrix* m_matrix;
    std::vector<Effect*> m_effects;
    size_t m_currentEffect = 0;
};

#endif // EFFECT_MANAGER_H