#ifndef SQUIDBODY_H
#define SQUIDBODY_H

#include <Arduino.h>
#include <Body/Body.h>
#include <math.h>

// Tip-led squid body drawn with GFX fillCircle / drawLine.
// The fin tip is the lead point; the mantle chain trails it and never flips.
// Arms hang off the head at the rear of the chain; the two outer hunting arms are longer.
class SquidBody : public Body {
 private:
  struct Particle {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float life = 0.0f;
    bool alive = false;
  };

  struct TentacleRoot {
    float ax;
    float ay;
    float normal;
    float hang;
    float u;
  };

  float bodySize;
  float segmentLength;
  float jointRadius;
  float headRadius;
  float tentacleSegLen;
  float huntSegLen;
  float fan;
  float armDrape;
  float huntDrape;
  float maxBend;

  PVector joints[SQUID_CHAIN_LENGTH];
  PVector tentacles[SQUID_TENTACLES][SQUID_HUNT_SEGMENTS];
  uint8_t tentacleSegCounts[SQUID_TENTACLES];
  float tentaclePhase[SQUID_TENTACLES];
  int8_t tentacleHueShift[SQUID_TENTACLES];

  float huntReach[2] = {0.0f, 0.0f};
  float huntGoal[2] = {0.0f, 0.0f};
  unsigned long huntNext[2] = {0, 0};

  Particle particles[SQUID_MAX_PARTICLES];
  bool placed = false;
  bool jetting = false;
  bool wasJetting = false;

  static constexpr float kDegToRad = PI / 180.0f;

  static float flerp(float a, float b, float t) {
    return a + (b - a) * t;
  }

  static float wrapPi(float d) {
    while (d > PI) d -= TWO_PI;
    while (d < -PI) d += TWO_PI;
    return d;
  }

  static float lerpAngle(float a, float b, float t) {
    return a + wrapPi(b - a) * t;
  }

  static int16_t toPx(float v) {
    return (int16_t)(v >= 0.0f ? v + 0.5f : v - 0.5f);
  }

  static int16_t toRad(float v) {
    if (v < 0.5f) return 0;
    if (v > 12.0f) return 12;
    return (int16_t)(v + 0.5f);
  }

  static CRGB hsvRgb(uint8_t h, uint8_t s, uint8_t v) {
    CRGB c;
    hsv2rgb_rainbow(CHSV(h, s, v), c);
    return c;
  }

  static bool isEdge(int t) {
    return t == 0 || t == SQUID_TENTACLES - 1;
  }

  float ageScale() const {
    if (size >= AGE_ADULT) return 1.0f;
    return constrain(size / (float)AGE_ADULT, 0.35f, 1.0f);
  }

  float mantleRadius(int i, float g) const {
    const float jr = jointRadius * g;
    const int headStart = SQUID_CHAIN_LENGTH - SQUID_HEAD_JOINTS;
    if (i < headStart) {
      const float u = constrain((float)(i + 1) / (float)(headStart + 1), 0.0f, 1.0f);
      const float thin = powf(u, 1.35f);
      const float full = powf(sinf(u * HALF_PI), 0.58f);
      const float b = constrain((u - 0.08f) / 0.42f, 0.0f, 1.0f);
      const float blendT = b * b * (3.0f - 2.0f * b);
      const float shape = flerp(thin, full, blendT);
      return jr * flerp(0.04f, 1.0f, shape);
    }
    const float u = (float)(i - headStart) / (float)max(1, SQUID_HEAD_JOINTS - 1);
    const float dome = powf(max(0.0f, 1.0f - u * u), 0.72f);
    return jr * flerp(0.42f, 1.08f, dome);
  }

  float spineHeading(int i) const {
    const float cx = i < 0 ? pos.x : joints[i].x;
    const float cy = i < 0 ? pos.y : joints[i].y;
    const float nx = i <= 0 ? pos.x : joints[i - 1].x;
    const float ny = i <= 0 ? pos.y : joints[i - 1].y;
    return atan2f(ny - cy, nx - cx);
  }

  void dragJoint(int i, float xin, float yin, float preferred, float step) {
    float ang = atan2f(joints[i].y - yin, joints[i].x - xin);
    const float delta = constrain(wrapPi(ang - preferred), -maxBend, maxBend);
    ang = preferred + delta;
    joints[i].x = xin + cosf(ang) * step;
    joints[i].y = yin + sinf(ang) * step;
  }

  void placeStraight() {
    const float step = segmentLength * ageScale();
    const float back = angle + PI;
    const float cb = cosf(back);
    const float sb = sinf(back);
    for (int i = 0; i < SQUID_CHAIN_LENGTH; i++) {
      joints[i].x = pos.x + cb * step * (float)(i + 1);
      joints[i].y = pos.y + sb * step * (float)(i + 1);
    }
  }

  void dragMantle() {
    const float step = segmentLength * ageScale();
    dragJoint(0, pos.x, pos.y, angle + PI, step);
    for (int i = 0; i < SQUID_CHAIN_LENGTH - 1; i++) {
      const float leadX = i == 0 ? pos.x : joints[i - 1].x;
      const float leadY = i == 0 ? pos.y : joints[i - 1].y;
      const float spine = atan2f(joints[i].y - leadY, joints[i].x - leadX);
      dragJoint(i + 1, joints[i].x, joints[i].y, spine, step);
    }
  }

  TentacleRoot tentacleRoot(int t, float g) const {
    const float u = SQUID_TENTACLES <= 1 ? 0.5f : (float)t / (float)(SQUID_TENTACLES - 1);
    const int ring = t % 2;
    int jointIdx = SQUID_CHAIN_LENGTH - 1 - ring;
    if (jointIdx < 0) jointIdx = 0;
    const float heading = spineHeading(jointIdx);
    const float r = mantleRadius(jointIdx, g);
    const float theta = flerp(-fan, fan, u);
    const float normal = heading + PI + theta;
    const float hang = heading + PI;
    const float attachR = r * (ring == 0 ? 0.92f : 0.78f);
    const float ax = joints[jointIdx].x + cosf(normal) * attachR;
    const float ay = joints[jointIdx].y + sinf(normal) * attachR;
    return {ax, ay, normal, hang, u};
  }

  void updateTentacles(bool snap, unsigned long now) {
    const float g = ageScale();
    const float time = fmodf((float)now * 0.00065f, TWO_PI * 100.0f);
    const float timeFine = fmodf((float)now * 0.001f, TWO_PI * 100.0f);

    for (int t = 0; t < SQUID_TENTACLES; t++) {
      const TentacleRoot root = tentacleRoot(t, g);
      const bool hunt = isEdge(t);
      const float step = (hunt ? huntSegLen : tentacleSegLen) * g;
      const float side = root.u - 0.5f;
      const float phase = tentaclePhase[t];
      const float drape = lerpAngle(root.normal, root.hang, hunt ? huntDrape : armDrape);
      const int segs = tentacleSegCounts[t];

      float reach = 0.0f;
      if (hunt) {
        const int gi = t == 0 ? 0 : 1;
        if (now > huntNext[gi]) {
          const float outward = t == 0 ? -1.0f : 1.0f;
          const long roll = random(0, 100);
          if (roll < 55) huntGoal[gi] = outward * ((float)random(90, 176) / 100.0f);
          else if (roll < 85) huntGoal[gi] = outward * ((float)random(35, 96) / 100.0f);
          else huntGoal[gi] = -outward * ((float)random(25, 96) / 100.0f);
          huntNext[gi] = now + (unsigned long)random(1100, 2400);
        }
        huntReach[gi] = flerp(huntReach[gi], huntGoal[gi], 0.045f);
        const float curious = sinf(time * 2.4f + phase) * 0.55f +
                              sinf(timeFine * 1.3f + phase * 1.7f) * 0.32f;
        reach = huntReach[gi] + curious;
      }

      const float lag = jetting ? 0.45f : 1.0f;
      float leadX = root.ax;
      float leadY = root.ay;
      bool hasPrev = false;
      float prevAng = 0.0f;

      for (int i = 0; i < segs; i++) {
        const float along = (float)i / (float)max(1, segs - 1);
        const float flare = side * (hunt ? 5.0f : 8.0f) * kDegToRad * sinf(along * PI);
        const float waveAmp = (hunt ? 22.0f : 12.0f) * kDegToRad;
        const float wave =
            sinf(time * (hunt ? 1.25f : 0.95f) - along * (hunt ? 3.8f : 3.2f) + phase) *
            waveAmp * (0.2f + along);
        const float bend = sinf(phase * 1.7f + (float)i * 0.65f + time * 2.4f) *
                           (hunt ? 14.0f : 4.0f) * kDegToRad * along;
        const float play = hunt
                               ? sinf(timeFine * 0.95f + phase + along * 3.1f) *
                                     (26.0f * kDegToRad) * along * along
                               : 0.0f;
        const float hook = hunt
                               ? sinf(time * 0.7f + phase * 1.4f + along * 2.2f) *
                                     (34.0f * kDegToRad) * along * along
                               : side * (7.0f * kDegToRad) * along * along;
        const float preferred =
            drape + reach * (0.12f + along * 0.9f) + flare + wave + bend + play + hook;

        float ang;
        if (snap) {
          ang = preferred;
        } else {
          ang = atan2f(tentacles[t][i].y - leadY, tentacles[t][i].x - leadX);
          const float follow =
              hunt ? flerp(0.09f, 0.045f, along) * (jetting ? 0.6f : 1.0f)
                   : flerp(0.07f, 0.016f, along) * lag;
          ang = lerpAngle(ang, preferred, follow);
        }

        if (hasPrev) {
          const float limit = (hunt ? 48.0f : 32.0f) * kDegToRad;
          const float delta = constrain(wrapPi(ang - prevAng), -limit, limit);
          ang = prevAng + delta;
        }

        leadX += cosf(ang) * step;
        leadY += sinf(ang) * step;
        tentacles[t][i].x = leadX;
        tentacles[t][i].y = leadY;
        prevAng = ang;
        hasPrev = true;
      }
    }
  }

  void spawnParticles(int count) {
    const int headIdx = SQUID_CHAIN_LENGTH - 1;
    const float exhaust = angle + PI;
    const float g = ageScale();
    const float hr = headRadius * g;
    const float fx = joints[headIdx].x + cosf(exhaust) * hr * 0.55f;
    const float fy = joints[headIdx].y + sinf(exhaust) * hr * 0.55f;

    for (int n = 0; n < count; n++) {
      int slot = -1;
      for (int i = 0; i < SQUID_MAX_PARTICLES; i++) {
        if (!particles[i].alive) {
          slot = i;
          break;
        }
      }
      if (slot < 0) return;

      const float spray = exhaust + (float)random(-55, 56) / 100.0f;
      const float spd = ((float)random(16, 43) / 10.0f) / SQUID_CANVAS_SCALE;
      Particle& p = particles[slot];
      p.x = fx + (float)random(-2, 3) * g;
      p.y = fy + (float)random(-2, 3) * g;
      p.vx = cosf(spray) * spd;
      p.vy = sinf(spray) * spd;
      p.life = 1.0f;
      p.alive = true;
    }
  }

  void updateParticles() {
    for (int i = 0; i < SQUID_MAX_PARTICLES; i++) {
      Particle& p = particles[i];
      if (!p.alive) continue;
      p.x += p.vx;
      p.y += p.vy;
      p.vx *= 0.94f;
      p.vy *= 0.94f;
      p.life -= (float)random(12, 29) / 1000.0f;
      if (p.life <= 0.0f) p.alive = false;
    }
  }

 public:
  SquidBody(Matrix* m, Head* head, Tail* tail, Fin* fin) : Body(m, head, tail, fin) {
    bodySize = (SQUID_CANVAS_SIZE / SQUID_CANVAS_SCALE) * SQUID_NORMAL_DRAW_SCALE;
    segmentLength = bodySize * 0.55f;
    jointRadius = bodySize * 0.42f;
    headRadius = bodySize * 0.30f;
    tentacleSegLen = bodySize * 0.42f;
    huntSegLen = bodySize * 0.58f;
    fan = 120.0f * kDegToRad;
    armDrape = 0.32f;
    huntDrape = 0.16f;
    maxBend = 22.0f * kDegToRad;

    for (int i = 0; i < SQUID_CHAIN_LENGTH; i++) {
      joints[i] = PVector(0, 0);
    }
    for (int t = 0; t < SQUID_TENTACLES; t++) {
      tentacleSegCounts[t] = isEdge(t) ? SQUID_HUNT_SEGMENTS : SQUID_ARM_SEGMENTS;
      tentaclePhase[t] = (float)random(0, 6283) / 1000.0f;
      tentacleHueShift[t] = (int8_t)random(-11, 12);
      for (int s = 0; s < SQUID_HUNT_SEGMENTS; s++) {
        tentacles[t][s] = PVector(0, 0);
      }
    }

    const unsigned long now = millis();
    huntNext[0] = now + (unsigned long)random(600, 1800);
    huntNext[1] = now + (unsigned long)random(1200, 2600);

    colorPalette = new ColorPalette(1, false);
    type = "Squid";
  }

  void update(PVector pos, PVector vel, float angle, float age = 0.8f,
              float health = 1.0f, float strokePhase = 0.0f,
              float turnRate = 0.0f) override {
    Body::update(pos, vel, angle, age, health, strokePhase, turnRate);
    if (age < AGE_EGG) return;

    const unsigned long now = millis();
    const float speedLed = vel.mag() / (float)PHYSICS_SCALE;
    jetting = speedLed > (SQUID_JET_SPEED / SQUID_CANVAS_SCALE) * 0.55f;

    if (!placed) {
      placeStraight();
      updateTentacles(true, now);
      placed = true;
    } else {
      dragMantle();
      updateTentacles(false, now);
    }
    updateParticles();
    if (jetting && !wasJetting) spawnParticles(3);
    else if (jetting) spawnParticles(1);
    wasJetting = jetting;
  }

  void display() override {
    if (!placed || !matrix || !matrix->foreground || !colorPalette ||
        colorPalette->colorsHSV.empty()) {
      return;
    }

    GFX_Layer* fg = matrix->foreground;
    const float g = ageScale();
    const uint8_t mantle = colorPalette->colorsHSV[0].hue;
    const uint8_t headHue = (uint8_t)(mantle + 51);
    const uint8_t armHue = (uint8_t)(mantle + 24);
    const uint8_t clubHue = (uint8_t)(mantle + 106);
    const uint8_t sat = colorPalette->colorsHSV[0].sat;
    const uint8_t val = colorPalette->colorsHSV[0].val;
    const int headStart = SQUID_CHAIN_LENGTH - SQUID_HEAD_JOINTS;

    // 1. Mantle & head crown.
    for (int i = SQUID_CHAIN_LENGTH - 1; i >= 0; i--) {
      const int16_t rad = toRad(mantleRadius(i, g));
      if (rad < 1) continue;
      const float t = (float)(i + 1) / (float)SQUID_CHAIN_LENGTH;
      const uint8_t hue = i >= headStart ? headHue : (uint8_t)(mantle + (uint8_t)(t * 11.0f));
      fg->fillCircle(toPx(joints[i].x), toPx(joints[i].y), rad, hsvRgb(hue, sat, val));
    }

    const int16_t tipRad = toRad(mantleRadius(-1, g));
    if (tipRad >= 1) {
      fg->fillCircle(toPx(pos.x), toPx(pos.y), tipRad, hsvRgb(mantle, sat, val));
    }

    // 2. Trailing arms & longer outer hunting tentacles.
    for (int t = 0; t < SQUID_TENTACLES; t++) {
      const TentacleRoot root = tentacleRoot(t, g);
      const bool hunt = isEdge(t);
      const int segs = tentacleSegCounts[t];
      float prevX = root.ax;
      float prevY = root.ay;
      for (int i = 0; i < segs; i++) {
        const float along = segs <= 1 ? 1.0f : (float)i / (float)(segs - 1);
        const uint8_t hue = (uint8_t)(armHue + tentacleHueShift[t] + (hunt ? 14 : 0) +
                                      (int)(along * (hunt ? 18.0f : 8.0f)));
        const uint8_t v = (uint8_t)(val * (1.0f - along * 0.15f));
        fg->drawLine(toPx(prevX), toPx(prevY), toPx(tentacles[t][i].x),
                     toPx(tentacles[t][i].y), hsvRgb(hue, sat, v));
        prevX = tentacles[t][i].x;
        prevY = tentacles[t][i].y;
      }
    }

    // 3. Hunting tentacle clubs.
    for (int t = 0; t < SQUID_TENTACLES; t++) {
      if (!isEdge(t)) continue;
      const int last = tentacleSegCounts[t] - 1;
      if (last < 0) continue;
      const PVector& tip = tentacles[t][last];
      const int16_t clubRad = toRad(bodySize * g * 0.14f);
      if (clubRad < 1) continue;
      fg->fillCircle(toPx(tip.x), toPx(tip.y), clubRad, hsvRgb(clubHue, sat, val));
      const int16_t coreRad = toRad(bodySize * g * 0.055f);
      if (coreRad >= 1) {
        fg->fillCircle(toPx(tip.x), toPx(tip.y), coreRad,
                       hsvRgb((uint8_t)(clubHue + 21), sat / 3, val));
      }
    }

    // 4. Jet propulsion particles.
    for (int i = 0; i < SQUID_MAX_PARTICLES; i++) {
      const Particle& p = particles[i];
      if (!p.alive || p.life <= 0.05f) continue;
      const uint8_t pv = (uint8_t)(val * p.life);
      fg->fillCircle(toPx(p.x), toPx(p.y), 1, hsvRgb(armHue, sat, pv));
    }
  }
};

#endif  // SQUIDBODY_H
