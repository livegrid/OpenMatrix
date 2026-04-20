#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Effect.h"

// Forward declarations - avoid pulling in every effect header here.
class TOFSensor;

/**
 * EffectManager holds a registry of effect *factories* and instantiates
 * exactly ONE effect at a time (the currently active one). Switching
 * destroys the previous instance before creating the next, keeping the
 * heap footprint to one effect at a time.
 *
 * Historically all effects were created eagerly at boot, which meant every
 * effect's internal buffers (particles, noise tables, sprite state, etc.)
 * were permanently resident even though only one ever draws.
 */
class EffectManager {
 public:
  // Registry entry: display name, factory that builds a fresh instance, and
  // an optional TOF-setter trampoline for effects that accept one. (Using
  // function pointers instead of dynamic_cast because the toolchain is
  // compiled with -fno-rtti.)
  struct Slot {
    const char* name;
    std::function<Effect*(Matrix*)> factory;
    std::function<void(Effect*, TOFSensor*)> applyTof;  // may be null
  };

  EffectManager(Matrix* matrix);
  ~EffectManager();

  void updateCurrentEffect();
  void setEffect(size_t number);
  void setEffect(const std::string& name);
  void nextEffect();
  void prevEffect();

  size_t getEffectCount() const { return m_slots.size(); }
  const char* getCurrentEffectName() const;
  uint8_t getCurrentEffect() const { return (uint8_t)m_currentIndex; }

  // Stores the sensor; re-applied whenever a new effect is instantiated.
  void setTofSensor(TOFSensor* sensor);

 private:
  void activate(size_t index);
  void destroyCurrent();
  void applyTofSensorToCurrent();

  Matrix* m_matrix;
  TOFSensor* m_tofSensor = nullptr;

  std::vector<Slot> m_slots;
  size_t m_currentIndex = 0;
  Effect* m_current = nullptr;
};
