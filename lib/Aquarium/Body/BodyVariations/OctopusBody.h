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

    void drawTentacle(uint8_t i, PVector bodyPos, float time, float velocityFactor, 
                       float backCenterX, float backCenterY, float spreadFactor) {
    // Calculate the angle for this tentacle
    float tentacleAngle = angle + PI + (i - (numTentacles - 1) / 2.0f) * spreadFactor;
    
    // Calculate the start position of the tentacle
    float tentacleStartX = backCenterX + cos(tentacleAngle) * (rad * size / 6);
    float tentacleStartY = backCenterY + sin(tentacleAngle) * (rad * size / 6);
    
    float currentX = tentacleStartX;
    float currentY = tentacleStartY;
    float segmentLength = (tentacleLength * size) / OCTOPUS_TENTACLE_SEGMENTS;

    // Pre-calculate base for sin: time * 2 + i * 0.5
    float sinBase = time * 2.0f + i * 0.5f;
    float movementScale = 0.2f * velocityFactor;

    for (uint8_t j = 0; j < OCTOPUS_TENTACLE_SEGMENTS; ++j) {
        float dx = currentX - tentacleSegments[i][j].x;
        float dy = currentY - tentacleSegments[i][j].y;
        float segmentAngle = atan2(dy, dx);

        // Add subtle movement to each segment
        float movementAngle = sin(sinBase + j * 0.3f) * movementScale;
        segmentAngle += movementAngle;

        tentacleSegments[i][j].x = currentX - cos(segmentAngle) * segmentLength;
        tentacleSegments[i][j].y = currentY - sin(segmentAngle) * segmentLength;

        CRGB segmentColor = colorPalette->colors[j + 1];
        
        matrix->foreground->drawLine(currentX, currentY, 
                                     tentacleSegments[i][j].x, tentacleSegments[i][j].y, 
                                     segmentColor);
        
        currentX = tentacleSegments[i][j].x;
        currentY = tentacleSegments[i][j].y;
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

        // Cache expensive calculations once for all tentacles
        // Wrap time to keep trig input small and consistent cost
        float time = fmodf(millis() / 1000.0f, TWO_PI);
        float velMagSq = vel.x * vel.x + vel.y * vel.y;
        float velocityFactor = min(sqrt(velMagSq) / 2.0f, 1.0f);
        
        // Calculate back center once
        float backOffsetX = cos(angle) * (rad * size / 6);
        float backOffsetY = sin(angle) * (rad * size / 6);
        float backCenterX = pos.x - backOffsetX;
        float backCenterY = pos.y - backOffsetY;
        float spreadFactor = (PI * 2.0f) / (numTentacles - 1);

        // Draw tentacles
        for (uint8_t i = 0; i < numTentacles; ++i) {
            drawTentacle(i, pos, time, velocityFactor, backCenterX, backCenterY, spreadFactor);
        }
    }
};

#endif