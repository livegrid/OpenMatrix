#include "EffectManager.h"

#include "../TOFSensor/TOFSensor.h"

// Full effect headers are only needed in the .cpp so the manager's header
// stays light.
#include "AsteroidHopperEffect.h"
#include "ConstellationEffect.h"
#include "GravityFlapEffect.h"
#include "MeteorShowerEffect.h"
#include "NoiseEffect.h"
#include "SpaceDriftEffect.h"
#include "SpaceInvadersEffect.h"

// Helper macros keep the registry table readable.
#define EFFECT_FACTORY(Cls) \
  [](Matrix* m) -> Effect* { return new Cls(m); }
#define EFFECT_TOF(Cls)                                                   \
  [](Effect* e, TOFSensor* s) { static_cast<Cls*>(e)->setTofSensor(s); }
#define EFFECT_TOF_RANGE(Cls)                                                             \
  [](Effect* e, int16_t minDist, int16_t maxDist) { static_cast<Cls*>(e)->setDetectionRange(minDist, maxDist); }

EffectManager::EffectManager(Matrix* matrix) : m_matrix(matrix) {
  // Registry order == selectable order in UI. Index 0 = default (Constellation).
  // Asteroid Hopper and Space Drift are the last selectable slots after Noise.
  // Effects that don't accept a TOFSensor get null applyTof entries.
  m_slots = {
      {"Constellation",   EFFECT_FACTORY(ConstellationEffect),   EFFECT_TOF(ConstellationEffect), EFFECT_TOF_RANGE(ConstellationEffect)},
      {"Meteor Shower",   EFFECT_FACTORY(MeteorShowerEffect),    EFFECT_TOF(MeteorShowerEffect), EFFECT_TOF_RANGE(MeteorShowerEffect)},
      {"Space Invaders",  EFFECT_FACTORY(SpaceInvadersEffect),   EFFECT_TOF(SpaceInvadersEffect), EFFECT_TOF_RANGE(SpaceInvadersEffect)},
      {"Gravity Flap",    EFFECT_FACTORY(GravityFlapEffect),     EFFECT_TOF(GravityFlapEffect), EFFECT_TOF_RANGE(GravityFlapEffect)},
      {"Noise",           EFFECT_FACTORY(NoiseEffect),           nullptr, nullptr},
      {"Asteroid Hopper", EFFECT_FACTORY(AsteroidHopperEffect),  EFFECT_TOF(AsteroidHopperEffect), EFFECT_TOF_RANGE(AsteroidHopperEffect)},
      // {"Space Drift",     EFFECT_FACTORY(SpaceDriftEffect),      EFFECT_TOF(SpaceDriftEffect), EFFECT_TOF_RANGE(SpaceDriftEffect)},
  };
  // No effect is instantiated yet - activate() runs on the first setEffect() call.
}

EffectManager::~EffectManager() { destroyCurrent(); }

void EffectManager::destroyCurrent() {
  if (m_current) {
    delete m_current;
    m_current = nullptr;
  }
}

void EffectManager::applyTofSensorToCurrent() {
  if (m_current == nullptr || m_tofSensor == nullptr) return;
  if (m_currentIndex >= m_slots.size()) return;
  const auto& applier = m_slots[m_currentIndex].applyTof;
  if (applier) applier(m_current, m_tofSensor);
}

void EffectManager::applyTofDetectionRangeToCurrent() {
  if (m_current == nullptr || !m_hasTofDetectionRange) return;
  if (m_currentIndex >= m_slots.size()) return;
  const auto& applier = m_slots[m_currentIndex].applyTofRange;
  if (applier) applier(m_current, m_tofMinDistance, m_tofMaxDistance);
}

void EffectManager::activate(size_t index) {
  if (index >= m_slots.size()) return;
  destroyCurrent();
  m_currentIndex = index;
  if (m_matrix && m_matrix->background) m_matrix->background->clear();
  log_i("EffectManager: activating %s", m_slots[index].name);
  m_current = m_slots[index].factory(m_matrix);
  applyTofSensorToCurrent();
  applyTofDetectionRangeToCurrent();
  if (m_current) m_current->reset();
}

void EffectManager::updateCurrentEffect() {
  if (m_current) m_current->update();
}

void EffectManager::setEffect(size_t number) {
  activate(number);
}

void EffectManager::setEffect(const std::string& name) {
  for (size_t i = 0; i < m_slots.size(); ++i) {
    if (name == m_slots[i].name) {
      activate(i);
      return;
    }
  }
}

void EffectManager::nextEffect() {
  size_t next = (m_currentIndex + 1) % m_slots.size();
  activate(next);
}

void EffectManager::prevEffect() {
  size_t prev = (m_currentIndex == 0) ? (m_slots.size() - 1) : (m_currentIndex - 1);
  activate(prev);
}

const char* EffectManager::getCurrentEffectName() const {
  if (m_current && m_currentIndex < m_slots.size()) {
    return m_slots[m_currentIndex].name;
  }
  return "None";
}

void EffectManager::setTofSensor(TOFSensor* sensor) {
  m_tofSensor = sensor;
  applyTofSensorToCurrent();
}

void EffectManager::setTofDetectionRange(int16_t minDist, int16_t maxDist) {
  m_tofMinDistance = minDist;
  m_tofMaxDistance = maxDist;
  m_hasTofDetectionRange = true;
  applyTofDetectionRangeToCurrent();
}
