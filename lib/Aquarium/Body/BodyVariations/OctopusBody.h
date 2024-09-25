#ifndef OCTOPUSBODY_H
#define OCTOPUSBODY_H

#include <Arduino.h>
#include <Body/Body.h>

class OctopusBody : public Body {
protected:
    uint8_t rad, tentacleLength;
    std::vector<std::vector<PVector>> tentacleSegments;
    uint8_t numTentacles;

public:
    OctopusBody(Matrix* m, Head* head, Tail* tail, Fin* fin) : Body(m, head, tail, fin) {
        rad = random(OCTOPUS_SIZE);
        numTentacles = random(OCTOPUS_MIN_TENTACLES, OCTOPUS_MAX_TENTACLES + 1);
        tentacleLength = random(OCTOPUS_TENTACLE_LENGTH);

        for (int i = 0; i < numTentacles; ++i) {
            std::vector<PVector> tentacle;
            for (int j = 0; j < OCTOPUS_TENTACLE_SEGMENTS; ++j) {
                tentacle.push_back(PVector(0, 0));
            }
            tentacleSegments.push_back(tentacle);
        }

        colorPalette = new ColorPalette(OCTOPUS_TENTACLE_SEGMENTS + 1); // +1 for body color
        type = "Octopus";
    }

    void drawTentacle(uint8_t i, PVector bodyPos) {
    float spreadAngle = PI * 2; // We'll keep this as a reasonable base spread
    
    // Calculate the back center of the octopus
    float backOffsetX = cos(angle) * (rad * size / 6);
    float backOffsetY = sin(angle) * (rad * size / 6);
    PVector backCenter(
        bodyPos.x - backOffsetX,
        bodyPos.y - backOffsetY
    );
    
    // Calculate the angle for this tentacle
    float tentacleAngle = angle + PI + (i - (numTentacles - 1) / 2.0) * (spreadAngle / (numTentacles - 1));
    // Calculate the start position of the tentacle
    PVector tentacleStart(
        backCenter.x + cos(tentacleAngle) * (rad * size / 6),
        backCenter.y + sin(tentacleAngle) * (rad * size / 6)
    );
    
    PVector current = tentacleStart;
    float segmentLength = (tentacleLength * size) / OCTOPUS_TENTACLE_SEGMENTS;

    float time = millis() / 1000.0; // Time factor for movement
    float velocityFactor = min(vel.mag() / 2.0, 1.0); // Velocity influence on movement

    for (uint8_t j = 0; j < OCTOPUS_TENTACLE_SEGMENTS; ++j) {
        PVector dv = current - tentacleSegments[i][j];
        float segmentAngle = dv.heading();

        // Add subtle movement to each segment
        float movementAngle = sin(time * 2 + i * 0.5 + j * 0.3) * 0.2 * velocityFactor;
        segmentAngle += movementAngle;

        tentacleSegments[i][j].x = current.x - cos(segmentAngle) * segmentLength;
        tentacleSegments[i][j].y = current.y - sin(segmentAngle) * segmentLength;

        CRGB segmentColor = colorPalette->colors[j + 1];
        
        matrix->foreground->drawLine(current.x, current.y, 
                                     tentacleSegments[i][j].x, tentacleSegments[i][j].y, 
                                     segmentColor);
        
        current = tentacleSegments[i][j];
    }
}
    void display() override {
        // Scale the radius less aggressively
        uint8_t rad2draw = rad * size / 4;
        if(rad2draw < 1) rad2draw = 1;
        
        // Keep the length scaling as before
        uint8_t length2draw = rad * size  / 2;
        if(length2draw < 1) length2draw = 1;

        // Draw the body (horizontally stretched ellipse)
        matrix->foreground->drawCircleArray(pos.x, pos.y, rad2draw, length2draw, angle, colorPalette->colors[0]);

        // Draw tentacles
        for (uint8_t i = 0; i < numTentacles; ++i) {
            drawTentacle(i, pos);
        }
    }
};

#endif