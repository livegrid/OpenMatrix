#ifndef MOTIONFACTORY_H
#define MOTIONFACTORY_H

#include "Motion.h"
#include "FishMotion.h"
#include "TurtleMotion.h"
#include "StarMotion.h"
#include "SnakeMotion.h"

class MotionFactory {
public:
    static std::unique_ptr<Motion> createMotion(const std::string& type, PVector pos, uint16_t xResolution, uint16_t yResolution, std::vector<std::shared_ptr<Attractor>>* attractors) {
        if (type == "Fish") {
            return std::make_unique<FishMotion>(pos, xResolution, yResolution, attractors);
        }
        else if (type == "Turtle") {
            return std::make_unique<TurtleMotion>(pos, xResolution, yResolution, attractors);
        }
        else if (type == "Star") {
            return std::make_unique<StarMotion>(pos, xResolution, yResolution, attractors);
        }
        else if (type == "Snake") {
            return std::make_unique<SnakeMotion>(pos, xResolution, yResolution, attractors);
        }
        else {
          return nullptr;
        }
    }
};

#endif // MOTIONFACTORY_H