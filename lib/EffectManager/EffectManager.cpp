#include "EffectManager.h"
#include "LSystemEffect.h"  // Add this line

EffectManager::EffectManager(Matrix* matrix) : m_matrix(matrix) {
    // m_effects.push_back(new SimplexNoiseEffect(matrix));
    // m_effects.push_back(new CellularNoiseEffect(matrix));
    m_effects.push_back(new NoiseEffect(matrix));
    m_effects.push_back(new SnakeEffect(matrix));
    m_effects.push_back(new FlockEffect(matrix));
    m_effects.push_back(new GameofLifeEffect(matrix));
    m_effects.push_back(new LsystemEffect(matrix));
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

void EffectManager::setEffect(size_t number) {
  m_matrix->background->clear();
  if (number < m_effects.size()) {
    m_currentEffect = number;
    m_effects[m_currentEffect]->reset();
  }
}

void EffectManager::nextEffect() {
    if (m_currentEffect == m_effects.size() - 1) {
        m_currentEffect = 0;
    } else {
        m_currentEffect++;
    }
    m_effects[m_currentEffect]->reset();
}

void EffectManager::prevEffect() {
    if (m_currentEffect == 0) {
        m_currentEffect = m_effects.size() - 1;
    } else {
        m_currentEffect--;
    }
    m_effects[m_currentEffect]->reset();
}

void EffectManager::setEffect(const std::string& name) {
  m_matrix->background->clear();
  for (size_t i = 0; i < m_effects.size(); ++i) {
    if (name == m_effects[i]->getName()) {
        m_currentEffect = i;
        m_effects[m_currentEffect]->reset();
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

uint8_t EffectManager::getCurrentEffect() const {
    return m_currentEffect;
}