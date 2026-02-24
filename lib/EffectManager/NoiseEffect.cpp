#include "NoiseEffect.h"

NoiseEffect::NoiseEffect(Matrix* m) : Effect(m) {
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFrequency(0.01f);
    int w = m->getXResolution();
    int h = m->getYResolution();
    m_bufferSize = w * h;
    m_noiseBuffer = new float[m_bufferSize];
}

NoiseEffect::~NoiseEffect() {
    delete[] m_noiseBuffer;
}

void NoiseEffect::setScale(uint8_t s) {
    scale = s;
}

void NoiseEffect::setSpeed(float s) {
    speed = s;
}

void NoiseEffect::reset() {}

void NoiseEffect::update() {
    int w = m_matrix->getXResolution();
    int h = m_matrix->getYResolution();
    if (m_bufferSize < w * h) return;

    float timeZ = (float)(millis() * speed);
    noise.FillNoise2D(m_noiseBuffer, w, h, (float)x, (float)y, timeZ, (float)scale, (float)scale);

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            float n = m_noiseBuffer[i + j * w];
            uint8_t noiseNow = (uint8_t)((n + 1.0f) * 127.5f);
            CRGB col = baseColor;
            col.nscale8(noiseNow / 2);
            m_matrix->background->drawPixel(i, j, col);
        }
    }
}

const char* NoiseEffect::getName() const {
    return "Noise";
}
