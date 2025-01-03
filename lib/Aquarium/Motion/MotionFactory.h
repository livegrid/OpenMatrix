#ifndef MOTIONFACTORY_H
#define MOTIONFACTORY_H

#include "Motion.h"
#include "FishMotion.h"
#include "TurtleMotion.h"
#include "StarMotion.h"
#include "SnakeMotion.h"
#include "OctopusMotion.h"

class MotionFactory {
public:
    static std::unique_ptr<Motion> createMotion(const String& type, PVector pos, uint16_t xResolution, uint16_t yResolution) {
        if (type == "Fish") {
            return std::make_unique<FishMotion>(pos, xResolution, yResolution);
        }
        else if (type == "Turtle") {
            return std::make_unique<TurtleMotion>(pos, xResolution, yResolution);
        }
        else if (type == "Star") {
            return std::make_unique<StarMotion>(pos, xResolution, yResolution);
        }
        else if (type == "Snake") {
            return std::make_unique<SnakeMotion>(pos, xResolution, yResolution);
        }
        else if (type == "Octopus") {
            return std::make_unique<OctopusMotion>(pos, xResolution, yResolution);
        }
        else {
          return nullptr;
        }
    }
};

#endif // MOTIONFACTORY_H