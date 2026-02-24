#include "SimplexNoiseEffect.h"

SimplexNoiseEffect::SimplexNoiseEffect(Matrix* m) : Effect(m) {
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
  noise.SetFrequency(.1f);
  int w = m->getXResolution();
  int h = m->getYResolution();
  m_bufferSize = w * h;
  m_noiseBuffer = new float[m_bufferSize];
}

SimplexNoiseEffect::~SimplexNoiseEffect() {
  delete[] m_noiseBuffer;
}

void SimplexNoiseEffect::setScale(uint8_t s) {
  scale = s;
}

void SimplexNoiseEffect::setSpeed(float s) {
  speed = s;
}

void SimplexNoiseEffect::update() {
  int w = m_matrix->getXResolution();
  int h = m_matrix->getYResolution();
  if (m_bufferSize < w * h) return;

  float timeZ = (float)(millis() * speed);
  noise.FillNoise2D(m_noiseBuffer, w, h, 0.0f, 0.0f, timeZ, (float)scale, (float)scale);

  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      float n = m_noiseBuffer[j + i * w];
      int noiseNow = (int)((n + 1.0f) * 127.5f);
      CRGB col = baseColor;
      int reducedNoise = noiseNow - 50;
      if (reducedNoise > 0)
        col.nscale8(noiseNow);
      else
        col.nscale8(0);
      m_matrix->background->drawPixel(i, j, col);
    }
  }
}

const char* SimplexNoiseEffect::getName() const {
  return "SimplexNoise";
}