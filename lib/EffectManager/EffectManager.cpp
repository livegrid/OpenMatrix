#include "EffectManager.h"

EffectManager::EffectManager(Matrix* matrix) : m_matrix(matrix) {
    m_effects.push_back(new NoiseEffect(matrix));
    // Add other effects here as you create them
}

EffectManager::~EffectManager() {
    for (auto effect : m_effects) {
        delete effect;
    }
}

void EffectManager::updateCurrentEffect() {
    if (m_currentEffect < m_effects.size()) {
        m_effects[m_currentEffect]->update();
    }
}

void EffectManager::setEffectByNumber(size_t number) {
    if (number < m_effects.size()) {
        m_currentEffect = number;
    }
}

void EffectManager::setEffectByName(const std::string& name) {
    for (size_t i = 0; i < m_effects.size(); ++i) {
        if (name == m_effects[i]->getName()) {
            m_currentEffect = i;
            return;
        }
    }
}

size_t EffectManager::getEffectCount() const {
    return m_effects.size();
}

const char* EffectManager::getCurrentEffectName() const {
    if (m_currentEffect < m_effects.size()) {
        return m_effects[m_currentEffect]->getName();
    }
    return "None";
}
