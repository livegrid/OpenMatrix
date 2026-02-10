#pragma once

#include "Effect.h"

// #include "SimplexNoiseEffect.h"
// #include "CellularNoiseEffect.h"
#include "NoiseEffect.h"
#include "SnakeEffect.h"
#include "GameofLifeEffect.h"
#include "FlockEffect.h"
#include "LSystemEffect.h"
#include "MeteorShowerEffect.h"
#include "SpaceInvadersEffect.h"
#include "ConstellationEffect.h"

#include <vector>
#include <string>

// Forward declaration
class TOFSensor;

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
    
    // ToF sensor integration for interactive effects
    void setTofSensor(TOFSensor* sensor);

private:
    Matrix* m_matrix;
    TOFSensor* m_tofSensor = nullptr;
    std::vector<Effect*> m_effects;
    size_t m_currentEffect = 0;
    
    // Store pointers to TOF-enabled effects for sensor updates
    ConstellationEffect* m_constellation = nullptr;
    MeteorShowerEffect* m_meteorShower = nullptr;
    SpaceInvadersEffect* m_spaceInvaders = nullptr;
};