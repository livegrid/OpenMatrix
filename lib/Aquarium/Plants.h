#ifndef PLANTS_H
#define PLANTS_H

#include <Arduino.h>
#include <Matrix.h>
#include <PVector.h>

struct Branch {
  PVector startPos;
  std::vector<PVector> nodes;
};

class Plants {
 private:
  Matrix* matrix;
  PVector pos;
  std::vector<Branch> branches;
  uint8_t numBranches;
  uint8_t branchSizeBase = 4;
  uint8_t numNodes;
  std::vector<float> phaseOffsets;

  float extend = 1;

 public:
  Plants(Matrix* m, uint8_t x, uint8_t y) : matrix(m) {
    pos = PVector(x, y);
    
    numBranches = random(6,12);
    for (uint8_t i = 0; i < numBranches; i++) {
      PVector branchStart = PVector::fromAngle(random((PI)*1000, (TWO_PI)*1000)/1000.0);
      branchStart *= branchSizeBase;
      
      Branch branch;
      branch.startPos = branchStart;
      branches.push_back(branch);

      // Initialize phase offset for each branch
      phaseOffsets.push_back(random(0, 500) / 100.0);
    }
    setupNodes();
    // addOriginToNodes();
  }

  void update(uint8_t humidity = 50) {
    unsigned long currentTime = millis();
    float sizeFactor = map(humidity, 0, 100, 0, 250);
    sizeFactor /= 100;

    for (uint8_t i = 0; i < branches.size(); i++) {
      matrix->foreground->drawLine(branches[i].nodes[0].x * sizeFactor + pos.x, branches[i].nodes[0].y * sizeFactor + pos.y, pos.x, pos.y, CRGB(0, 0, 0));
      // matrix->background->drawLine(branches[i].nodes[0].x * sizeFactor + pos.x, branches[i].nodes[0].y * sizeFactor + pos.y, pos.x, pos.y, CRGB(0, 0, 0));
      for (uint8_t j = 1; j < branches[i].nodes.size(); j++) {
        PVector node = branches[i].nodes[j];
        PVector prevNode = branches[i].nodes[j - 1];
        
        // Calculate sway based on node height and time
        float swayAmplitude = 0.8 * j; // Increase amplitude with node height
        float sway = sin(currentTime / 10000.0 + phaseOffsets[i]) * swayAmplitude;

        // Apply sway to x coordinates
        // matrix->background->drawLine(prevNode.x * sizeFactor + sway + pos.x, prevNode.y * sizeFactor + pos.y, node.x * sizeFactor + sway + pos.x, node.y * sizeFactor + pos.y, CRGB(0,0,1));
        matrix->foreground->drawLine(prevNode.x * sizeFactor + sway + pos.x, prevNode.y * sizeFactor + pos.y, node.x * sizeFactor + sway + pos.x, node.y * sizeFactor + pos.y, CRGB(0,0,1));
        
        // Flower at the end of the branch
        if(j == branches[i].nodes.size()-1){
          float glowFactor = (sin(currentTime / 10000.0 + phaseOffsets[i])) - 0.8; // Shift and scale the sine wave
          if(glowFactor > 0) {
            uint8_t glowIntensity = static_cast<uint8_t>(glowFactor * 1000); // Scale to color intensity
            // matrix->background->fillCircle(node.x * sizeFactor + sway + pos.x, node.y * sizeFactor + pos.y, 1, CRGB(glowIntensity, glowIntensity, 0));
            matrix->foreground->fillCircle(node.x * sizeFactor + sway + pos.x, node.y * sizeFactor + pos.y, 1, CRGB(glowIntensity, glowIntensity, 0));
          }
        }
      }
    }
  }

 private: 
  void setupNodes() {
    for (uint8_t i = 0; i < branches.size(); i++) {
      numNodes = random(4, 9);
      branches[i].nodes.push_back(branches[i].startPos);
      for (uint8_t j = 1; j < numNodes; j++) {
        PVector node = buildNode(branches[i].nodes[j-1]);
        branches[i].nodes.push_back(node);
      }
    }
  }

  PVector buildNode(PVector startVec) {
    float theta = startVec.heading();
    // Targeting 3*PI/2, adjust the increment calculation
    float target = 3 * PI / 2;
    float diff = target - theta;
    // Normalize the difference to be within -PI to PI for a smooth transition
    if (diff > PI) {
      diff -= TWO_PI;
    } else if (diff < -PI) {
      diff += TWO_PI;
    }
    // Apply a fraction of the difference to gently steer the branch towards 3*PI/2
    float increment = diff * 0.5;  // Adjust the 0.1 as needed to control the curvature

    PVector endPos = PVector::fromAngle(theta + increment);
    // endPos *= startVec.mag()*0.75;
    endPos *= branchSizeBase;
    endPos += startVec;
    return endPos;
  }

  void addOriginToNodes() {
    for (uint8_t i = 0; i < branches.size(); i++) {
      for (uint8_t j = 0; j < branches[i].nodes.size(); j++) {
        branches[i].nodes[j] += pos;
      }
    }
  }
};

#endif  // PLANTS_H
