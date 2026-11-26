#ifndef TURTLEBODY_H
#define TURTLEBODY_H

#include <Arduino.h>
#include <Body/Body.h>
#include <math.h>

// Sea turtle body rasterized in local carapace coordinates for rotation-invariant
// silhouettes on low-res LED matrices:
// - Broad-shouldered tapered shield shell with scalloped warm rim
// - Nested concentric inner shapes (smooth gradient OR alternating strips, no dark lines)
// - Concentric patterns emerge smoothly only after the turtle reaches mature size
// - Sweeping curved hydrofoil front flippers, compact hind paddles, rounded head (no eyes), and tail
class TurtleBody : public Body {
 private:
  struct Morph {
    uint8_t shellH;
    uint8_t ridgeH;
    uint8_t bandH;
    uint8_t rimH;
    uint8_t skinH;
    uint8_t shellS;
    uint8_t skinS;
  };

  struct Polyline {
    static constexpr int kSteps = 6;
    float x[kSteps + 1];
    float y[kSteps + 1];
    float minX, maxX, minY, maxY;
  };

  Morph morph;
  float shellL;
  float shellW;
  float flipperLen;
  bool gradientStyle;
  uint8_t ringCount;
  float hexFacet;
  float strokePhase = 0.0f;
  float turnRate = 0.0f;

  static float flerp(float a, float b, float t) {
    return a + (b - a) * t;
  }

  static uint8_t lerpHue(uint8_t h0, uint8_t h1, float t) {
    int16_t d = (int16_t)(((int16_t)h1 - (int16_t)h0 + 128) & 255) - 128;
    return (uint8_t)(((int16_t)h0 + (int16_t)(d * t) + 256) & 255);
  }

  static void buildQuadPolyline(Polyline& pl, float x0, float y0, float x1, float y1,
                                float x2, float y2, float pad) {
    pl.minX = 1e6f;
    pl.maxX = -1e6f;
    pl.minY = 1e6f;
    pl.maxY = -1e6f;
    for (int s = 0; s <= Polyline::kSteps; s++) {
      const float t = (float)s / (float)Polyline::kSteps;
      const float it = 1.0f - t;
      const float px = it * it * x0 + 2.0f * it * t * x1 + t * t * x2;
      const float py = it * it * y0 + 2.0f * it * t * y1 + t * t * y2;
      pl.x[s] = px;
      pl.y[s] = py;
      if (px < pl.minX) pl.minX = px;
      if (px > pl.maxX) pl.maxX = px;
      if (py < pl.minY) pl.minY = py;
      if (py > pl.maxY) pl.maxY = py;
    }
    pl.minX -= pad;
    pl.maxX += pad;
    pl.minY -= pad;
    pl.maxY += pad;
  }

  static bool distToPolyline(const Polyline& pl, float px, float py,
                             float& outDist, float& outT) {
    if (px < pl.minX || px > pl.maxX || py < pl.minY || py > pl.maxY) {
      return false;
    }
    float bestD2 = 1e9f;
    float bestT = 0.0f;
    for (int s = 0; s < Polyline::kSteps; s++) {
      const float ax = pl.x[s];
      const float ay = pl.y[s];
      const float abx = pl.x[s + 1] - ax;
      const float aby = pl.y[s + 1] - ay;
      const float ab2 = abx * abx + aby * aby;
      float h = ab2 > 1e-6f ? ((px - ax) * abx + (py - ay) * aby) / ab2 : 0.0f;
      if (h < 0.0f) h = 0.0f;
      else if (h > 1.0f) h = 1.0f;
      const float dx = px - (ax + abx * h);
      const float dy = py - (ay + aby * h);
      const float d2 = dx * dx + dy * dy;
      if (d2 < bestD2) {
        bestD2 = d2;
        bestT = ((float)s + h) / (float)Polyline::kSteps;
      }
    }
    outDist = sqrtf(bestD2);
    outT = bestT;
    return true;
  }

  float ageScale() const {
    if (size >= AGE_ADULT) return 1.0f;
    const float t = constrain((size - (float)AGE_EGG) / (float)(AGE_ADULT - AGE_EGG), 0.0f, 1.0f);
    return flerp(0.46f, 1.0f, t);
  }

  float ageValFactor() const {
    if (size >= AGE_ADULT) {
      return 1.0f - ((size - (float)AGE_ADULT) / (float)(AGE_DEAD - AGE_ADULT)) * 0.5f;
    }
    return 1.0f;
  }

  CRGB hsvColor(uint8_t h, float s, float v) const {
    const uint8_t sat = (uint8_t)constrain(s * health, 0.0f, 255.0f);
    const uint8_t val = (uint8_t)constrain(v * ageValFactor(), 22.0f, 255.0f);
    CRGB c;
    hsv2rgb_rainbow(CHSV(h, sat, val), c);
    // Keep non-black so pixels never punch through to transparent water.
    if (c.r == 0 && c.g == 0 && c.b == 0) return CRGB(2, 4, 3);
    return c;
  }

  void strokeKinematics(float phase, float& sweep, float& flex) const {
    float p = fmodf(phase, TWO_PI);
    if (p < 0.0f) p += TWO_PI;
    p /= TWO_PI;
    if (p < 0.34f) {
      const float t = p / 0.34f;
      const float ease = 0.5f - 0.5f * cosf(t * PI);
      sweep = 1.0f - 2.0f * ease;  // +1 forward -> -1 swept back
      flex = sinf(t * PI);
    } else if (p < 0.68f) {
      const float t = (p - 0.34f) / 0.34f;
      const float s = t * t * (3.0f - 2.0f * t);
      sweep = flerp(-1.0f, -0.62f, s);  // serene swept-back glide
      flex = flerp(0.14f, -0.08f, s);
    } else {
      const float t = (p - 0.68f) / 0.32f;
      const float ease = 0.5f - 0.5f * cosf(t * PI);
      sweep = flerp(-0.62f, 1.0f, ease);  // graceful recovery forward
      flex = -0.65f * sinf(t * PI);
    }
  }

  void syncPaletteFromMorph() {
    if (!colorPalette) return;
    std::vector<CHSV> saved;
    saved.reserve(5);
    saved.push_back(CHSV(morph.ridgeH, morph.shellS, 248));
    saved.push_back(CHSV(morph.shellH, morph.shellS, 240));
    saved.push_back(CHSV(morph.bandH, morph.shellS, 220));
    saved.push_back(CHSV(morph.rimH, morph.shellS, 245));
    saved.push_back(CHSV(morph.skinH, morph.skinS, 235));
    colorPalette->setColors(saved);
  }

 public:
  TurtleBody(Matrix* m, Head* head, Tail* tail, Fin* fin) : Body(m, head, tail, fin) {
    static const Morph kMorphs[4] = {
        {96, 66, 112, 56, 90, 162, 120},   // Green sea turtle
        {34, 54, 18, 58, 80, 154, 122},    // Hawksbill tortoiseshell
        {108, 86, 126, 64, 100, 152, 115}, // Reef jade
        {76, 50, 94, 44, 86, 160, 122},    // Sunlit olive
    };
    const Morph& base = kMorphs[random(0, 4)];
    const int8_t jitter = (int8_t)random(-4, 5);
    morph.shellH = (uint8_t)(base.shellH + jitter);
    morph.ridgeH = (uint8_t)(base.ridgeH + jitter);
    morph.bandH = (uint8_t)(base.bandH + jitter);
    morph.rimH = (uint8_t)(base.rimH + jitter);
    morph.skinH = (uint8_t)(base.skinH + jitter);
    morph.shellS = base.shellS;
    morph.skinS = base.skinS;

    const float scaleVar = (float)random(93, 108) / 100.0f;
    shellL = 7.0f * scaleVar;
    shellW = 5.6f * scaleVar;
    flipperLen = 8.6f * scaleVar;

    gradientStyle = (random(0, 2) == 0);
    ringCount = (uint8_t)random(3, 5);
    hexFacet = (random(0, 2) == 0) ? 0.055f : 0.0f;

    colorPalette = new ColorPalette(5, false);
    syncPaletteFromMorph();
    type = "Turtle";
  }

  void setColorPaletteHSV(const std::vector<CHSV>& palette) override {
    if (palette.size() >= 5) {
      morph.ridgeH = palette[0].hue;
      morph.shellH = palette[1].hue;
      morph.bandH = palette[2].hue;
      morph.rimH = palette[3].hue;
      morph.skinH = palette[4].hue;
      morph.shellS = palette[1].sat > 60 ? palette[1].sat : morph.shellS;
      morph.skinS = palette[4].sat > 50 ? palette[4].sat : morph.skinS;
      syncPaletteFromMorph();
    } else {
      // Older save files stored only 2-3 arbitrary colors; keep the sea-turtle morph.
      syncPaletteFromMorph();
    }
  }

  void update(PVector pos, PVector vel, float angle, float age = 0.8f,
              float health = 1.0f, float strokePhase = 0.0f,
              float turnRate = 0.0f) override {
    this->pos = pos;
    this->vel = vel;
    this->angle = angle;
    this->size = age;
    this->health = health;
    this->strokePhase = strokePhase;
    this->turnRate = turnRate;
    if (colorPalette && !colorPalette->colors.empty()) {
      colorPalette->colors[0] = hsvColor(morph.ridgeH, morph.shellS, 248.0f);
    }
  }

  void display() override {
    if (!matrix || !matrix->foreground) return;
    GFX_Layer* fg = matrix->foreground;

    const float g = ageScale();
    const float sL = shellL * g;
    const float sW = shellW * g;
    const float fLen = flipperLen * g;
    const float babyBoost = flerp(1.22f, 1.0f, constrain((g - 0.46f) / 0.54f, 0.0f, 1.0f));

    const float cx = pos.x;
    const float cy = pos.y;
    const float cosA = cosf(angle);
    const float sinA = sinf(angle);

    float sweep = 0.0f;
    float flex = 0.0f;
    strokeKinematics(strokePhase, sweep, flex);
    const float turnBias = constrain(turnRate * 8.0f, -0.35f, 0.35f);
    const float headYaw = constrain(turnRate * 4.5f, -0.32f, 0.32f);
    const float cosH = cosf(-headYaw);
    const float sinH = sinf(-headYaw);

    // Precompute curved hydrofoil front flipper polylines + tight AABBs.
    Polyline frontPl[2];
    const float frontPad = max(0.8f, g * babyBoost * 1.95f);
    for (int f = 0; f < 2; f++) {
      const float side = f == 0 ? -1.0f : 1.0f;
      const float sideSweep = constrain(sweep - side * turnBias * 0.32f, -1.08f, 1.08f);
      const float a = flerp(-0.76f, 0.62f, (sideSweep + 1.0f) * 0.5f);
      const float p0x = sL * 0.38f;
      const float p0y = side * sW * 0.64f;
      const float midA = a + 0.30f - flex * 0.14f;
      const float tipA = a - 0.24f - flex * 0.22f;
      const float p1x = p0x + sinf(midA) * fLen * 0.55f;
      const float p1y = p0y + side * cosf(midA) * fLen * 0.55f;
      const float p2x = p0x + sinf(tipA) * fLen;
      const float p2y = p0y + side * cosf(tipA) * fLen;
      buildQuadPolyline(frontPl[f], p0x, p0y, p1x, p1y, p2x, p2y, frontPad);
    }

    // Precompute compact hind paddle polylines.
    Polyline rearPl[2];
    const float rearKick = sinf(strokePhase - 1.15f) * 0.14f;
    const float rearPad = max(0.8f, g * 1.35f);
    for (int f = 0; f < 2; f++) {
      const float side = f == 0 ? -1.0f : 1.0f;
      const float rudder = -turnBias * 0.55f + side * rearKick;
      const float r0x = -sL * 0.66f;
      const float r0y = side * sW * 0.46f;
      const float r1x = -sL * 0.94f;
      const float r1y = side * sW * 0.78f + rudder * g * 1.1f;
      const float r2x = -sL * 1.18f;
      const float r2y = side * sW * 0.62f + rudder * g * 1.7f;
      buildQuadPolyline(rearPl[f], r0x, r0y, r1x, r1y, r2x, r2y, rearPad);
    }

    // Precompute small pointed tail polyline.
    Polyline tailPl;
    const float tailBend = -headYaw * 0.9f;
    buildQuadPolyline(tailPl, -sL * 0.88f, 0.0f, -sL * 1.04f, tailBend * g * 0.6f,
                      -sL * 1.18f, tailBend * g * 1.3f, max(0.8f, g * 0.95f));

    // Rounded head & distinct neck.
    const float neckU = sL * 0.80f;
    const float headLen = 2.45f * g * babyBoost;
    const float headWid = 1.92f * g * babyBoost;
    const float headCenterU = 3.05f * g * babyBoost;
    const float neckHalfW = max(0.65f, 1.05f * g * babyBoost);

    // Concentric shell patterns only emerge smoothly after reaching mature physical size.
    const float patternT = constrain((sL - 5.3f) / 1.0f, 0.0f, 1.0f);
    const float patternAmt = patternT * patternT * (3.0f - 2.0f * patternT);
    const uint8_t patternBlend = (uint8_t)(patternAmt * 255.0f);

    const int16_t boundR =
        (int16_t)ceilf(max(sL * 1.55f + headLen + 1.5f, sW + fLen + 2.5f));
    const int16_t viewW = matrix->getXResolution();
    const int16_t viewH = matrix->getYResolution();
    const int16_t minX = max((int16_t)0, (int16_t)floorf(cx - boundR));
    const int16_t maxX = min((int16_t)(viewW - 1), (int16_t)ceilf(cx + boundR));
    const int16_t minY = max((int16_t)0, (int16_t)floorf(cy - boundR));
    const int16_t maxY = min((int16_t)(viewH - 1), (int16_t)ceilf(cy + boundR));

    const Morph& m = morph;
    const float invSL = 1.0f / sL;

    for (int16_t py = minY; py <= maxY; py++) {
      const float dy = (float)py - cy;
      for (int16_t px = minX; px <= maxX; px++) {
        const float dx = (float)px - cx;
        // Turtle-local coordinates: u = forward, v = right.
        const float u = dx * cosA + dy * sinA;
        const float v = -dx * sinA + dy * cosA;

        // 1. Carapace shell (layered on top of flipper, neck, and tail roots).
        const float nx = u * invSL;
        if (fabsf(nx) <= 1.04f) {
          const float widthProfile = max(0.48f, 1.0f + 0.14f * nx - 0.16f * nx * nx);
          const float ny = v / (sW * widthProfile);
          const float r2 = nx * nx + ny * ny;
          if (r2 <= 1.0f) {
            const float r = sqrtf(r2);
            const float dome = max(0.0f, 1.0f - r2 * 0.52f);
            CRGB col;
            if (r > 0.80f) {
              const float ang = atan2f(ny, nx);
              const float scallop = 0.5f + 0.5f * cosf(ang * 12.0f);
              const float rimVal = flerp(222.0f, 248.0f, scallop * 0.45f + dome * 0.55f);
              col = hsvColor(m.rimH, (float)m.shellS * 0.72f, rimVal);
            } else if (patternBlend == 0) {
              const float val = flerp(205.0f, 244.0f, dome);
              col = hsvColor(m.shellH, (float)m.shellS * 0.92f, val);
            } else {
              const float ang = atan2f(ny, nx);
              const float facet = 1.0f + hexFacet * cosf(ang * 6.0f);
              const float ringU = constrain((r / 0.80f) * facet, 0.0f, 1.0f);
              const float baseVal = flerp(205.0f, 244.0f, dome);
              uint8_t patHue;
              float patSat;
              float patVal;

              if (gradientStyle) {
                const float wave =
                    0.5f + 0.5f * cosf(ringU * PI * (float)(ringCount * 2 - 1));
                if (ringU < 0.45f) {
                  const float t = ringU / 0.45f;
                  patHue = lerpHue(m.ridgeH, m.shellH, t);
                  patSat = flerp((float)m.shellS * 0.68f, (float)m.shellS, t);
                } else {
                  const float t = (ringU - 0.45f) / 0.55f;
                  patHue = lerpHue(m.shellH, m.bandH, t);
                  patSat = flerp((float)m.shellS, min(240.0f, (float)m.shellS * 1.15f), t);
                }
                patVal = flerp(255.0f, 174.0f, ringU * 0.88f) * flerp(0.93f, 1.06f, wave);
              } else {
                const int bandIdx =
                    min((int)ringCount - 1, (int)(ringU * (float)ringCount));
                const float bandLocal = ringU * (float)ringCount - (float)bandIdx;
                const float bandSoft = 1.0f - fabsf(bandLocal - 0.5f) * 0.14f;
                const bool isBrightBand = (((int)ringCount - 1 - bandIdx) % 2) == 1;
                if (isBrightBand) {
                  patHue = bandIdx == 0 ? m.ridgeH : lerpHue(m.ridgeH, m.shellH, 0.28f);
                  patSat = (float)m.shellS * 0.72f;
                  patVal = (bandIdx == 0 ? 255.0f : 244.0f) * bandSoft;
                } else {
                  patHue = bandIdx == (int)ringCount - 1 ? m.bandH : m.shellH;
                  patSat = min(240.0f, (float)m.shellS * 1.12f);
                  patVal = flerp(196.0f, 172.0f,
                                 (float)bandIdx / (float)max(1, (int)ringCount - 1)) *
                           bandSoft;
                }
              }

              const CRGB baseCol = hsvColor(m.shellH, (float)m.shellS * 0.92f, baseVal);
              const CRGB patCol = hsvColor(patHue, patSat, patVal);
              col = blend(baseCol, patCol, patternBlend);
            }
            fg->drawPixel(px, py, col);
            continue;
          }
        }

        // 2. Rounded head & distinct neck (no eyes).
        if (u > sL * 0.55f) {
          const float nu = u - neckU;
          const float hu = nu * cosH - v * sinH;
          const float hv = nu * sinH + v * cosH;

          const float headDx = (hu - headCenterU) / headLen;
          const float snoutTaper = headDx > 0.0f ? 1.0f - headDx * 0.16f : 1.0f;
          const float headDy = hv / (headWid * max(0.65f, snoutTaper));
          const float headR2 = headDx * headDx + headDy * headDy;

          const bool inNeck =
              hu >= -0.6f * g && hu <= headCenterU && fabsf(hv) <= neckHalfW;

          if (headR2 <= 1.0f || inNeck) {
            const float shade = headR2 <= 1.0f ? max(0.0f, 1.0f - headR2 * 0.45f) : 0.42f;
            const float val = flerp(210.0f, 250.0f, shade);
            fg->drawPixel(px, py, hsvColor(m.skinH, (float)m.skinS * 0.92f, val));
            continue;
          }
        }

        // 3. Sweeping curved hydrofoil front flippers.
        bool drawn = false;
        for (int f = 0; f < 2; f++) {
          float dist, t;
          if (!distToPolyline(frontPl[f], u, v, dist, t)) continue;
          const float bladeRad = max(
              0.72f, g * babyBoost * (1.34f * (1.0f - t) + 0.56f * t + 0.58f * sinf(t * PI)));
          if (dist <= bladeRad) {
            const float edgeT = dist / bladeRad;
            const float val =
                flerp(244.0f, 210.0f, t * 0.6f) * flerp(1.04f, 0.88f, edgeT * edgeT);
            fg->drawPixel(px, py, hsvColor(m.skinH, (float)m.skinS, val));
            drawn = true;
            break;
          }
        }
        if (drawn) continue;

        // 4. Compact hind paddle flippers & little pointed tail.
        if (u < -sL * 0.35f) {
          for (int f = 0; f < 2; f++) {
            float dist, t;
            if (!distToPolyline(rearPl[f], u, v, dist, t)) continue;
            const float padRad =
                max(0.72f, g * (0.76f + 0.52f * sinf(powf(t, 0.85f) * PI * 0.92f)));
            if (dist <= padRad) {
              const float val = flerp(224.0f, 194.0f, t);
              fg->drawPixel(px, py, hsvColor(m.skinH, (float)m.skinS * 1.02f, val));
              drawn = true;
              break;
            }
          }
          if (drawn) continue;

          float dist, t;
          if (distToPolyline(tailPl, u, v, dist, t)) {
            const float tailRad = max(0.72f, g * flerp(0.88f, 0.55f, t));
            if (dist <= tailRad) {
              fg->drawPixel(px, py, hsvColor(m.skinH, (float)m.skinS, flerp(214.0f, 182.0f, t)));
            }
          }
        }
      }
    }
  }
};

#endif  // TURTLEBODY_H
