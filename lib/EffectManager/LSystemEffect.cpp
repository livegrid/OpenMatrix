#include "LSystemEffect.h"

#include <cmath>

LSystemEffect::LSystemEffect(Matrix* m) : Effect(m) {
  reset();
}

void LSystemEffect::update() {
  m_matrix->background->clear();

  if (scale < 1.0f) {
    scale += growthRate;
    scale = std::min(scale, 1.0f);
    fullGrowthDelay = 0;
  } else {
    fullGrowthDelay++;
    if (fullGrowthDelay >= FULL_GROWTH_DELAY_MAX) {
      reset();
    }
  }

  drawLSystem();
}

const char* LSystemEffect::getName() const {
  return "LSystem";
}

void LSystemEffect::reset() {
  word = "X";
  maxGeneration = 5;
  currGeneration = 0;
  growthPercent = 1;
  growthRate = 0.001;
  scale = 0.0f;

  for (int i = 0; i < maxGeneration; i++) {
    generate();
  }
}

void LSystemEffect::generate() {
  if (currGeneration == maxGeneration) {
    currGeneration = 0;
    word = "X";
  } else {
    std::string next = "";
    for (char c : word) {
      next += applyRules(c);
    }
    word = next;
    currGeneration++;
  }
  growthPercent = 0;
}

std::string LSystemEffect::chooseOne(const std::vector<Rule>& ruleSet) {
  float n = static_cast<float>(random(100)) / 100.0f;
  float t = 0;
  for (const auto& rule : ruleSet) {
    t += rule.prob;
    if (t > n) {
      return rule.rule;
    }
  }
  return "";
}

std::string LSystemEffect::applyRules(char c) {
  switch (c) {
    case 'X':
      return chooseOne(xRules);
    case 'F':
      return chooseOne(fRules);
    default:
      return std::string(1, c);
  }
}

void LSystemEffect::drawLSystem() {
  int x = m_matrix->getXResolution() / 2;
  int y = m_matrix->getYResolution() - 1;
  float angle = -M_PI / 2;  // Start growing upwards
  std::vector<std::tuple<int, int, float>> stack;

  const float subPixelScale = 4.0f;
  float subX = x * subPixelScale;
  float subY = y * subPixelScale;

  for (char c : word) {
    switch (c) {
      case 'F': {
        float newSubX = subX + cos(angle) * len * subPixelScale * scale;
        float newSubY = subY + sin(angle) * len * subPixelScale * scale;
        m_matrix->background->drawLine(subX / subPixelScale, subY / subPixelScale, 
                                       newSubX / subPixelScale, newSubY / subPixelScale, 
                                       CRGB(158, 169, 63));
        subX = newSubX;
        subY = newSubY;
      } break;
      case '+':
        angle -= M_PI / 4 * scale;
        break;
      case '-':
        angle += M_PI / 4 * scale;
        break;
      case '[':
        stack.push_back({subX, subY, angle});
        break;
      case ']':
        if (!stack.empty()) {
          std::tie(subX, subY, angle) = stack.back();
          stack.pop_back();
        }
        break;
      case 'A':
        m_matrix->background->fillCircle(subX / subPixelScale, subY / subPixelScale, 
                                         len * scale, CRGB(229, 206, 220));
        break;
      case 'B':
        m_matrix->background->fillCircle(subX / subPixelScale, subY / subPixelScale, 
                                         len * scale, CRGB(252, 161, 125));
        break;
    }
  }
}