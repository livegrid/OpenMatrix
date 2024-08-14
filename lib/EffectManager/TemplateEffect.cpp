#include "TemplateEffect.h"

TemplateEffect::TemplateEffect(Matrix* m) : Effect(m) {}

// Implement public methods here

void TemplateEffect::update() {
    uint16_t currentTimeSpeedInt = millis() * speed;
    for(int i = 0; i < m_matrix->getXResolution(); i++) {
        for(int j = 0; j < m_matrix->getYResolution(); j++) {
            CRGB col = CRGB(255, 0, 0);
            m_matrix->background->drawPixel(i, j, col);
        }
    }
}

const char* TemplateEffect::getName() const {
    return "Template";
}