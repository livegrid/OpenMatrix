#ifndef SQUIDMOTION_H
#define SQUIDMOTION_H

#include <math.h>
#include "Motion.h"

// Tip-led swim for squids. faceAngle is where the fin tip points; speed lives
// in canvas pixels per frame and is scaled onto the LED panel when integrating.
class SquidMotion : public Motion {
 private:
  float canvasSize;
  int chainLength;
  float segmentFactor;
  float cruiseSpeed;
  float jetSpeed;
  float faceAngle;
  float speed;

  float bloom = 0.0f;
  float bloomTarget = 0.0f;
  bool turning = false;
  float turnRemaining = 0.0f;
  bool jetting = false;
  unsigned long jetEnd = 0;
  unsigned long nextJet = 0;
  unsigned long reverseEnd = 0;
  unsigned long nextReverse = 0;
  unsigned long wanderNext = 0;

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

  float ledSpeed(float canvasPxPerFrame) const {
    return (canvasPxPerFrame / SQUID_CANVAS_SCALE) * (float)PHYSICS_SCALE;
  }

  float toCanvas(float physics) const {
    return (physics / (float)PHYSICS_SCALE) * SQUID_CANVAS_SCALE;
  }

  void updateBehavior(unsigned long now) {
    bloom = flerp(bloom, bloomTarget, 0.02f);

    if (turning && turnRemaining > 0.0f) {
      const float step = 0.0045f;
      const float d = step < turnRemaining ? step : turnRemaining;
      faceAngle += d;
      turnRemaining -= d;
      if (turnRemaining <= 0.0f) turning = false;
    }

    if (now > wanderNext && !jetting && !turning && bloomTarget < 0.1f) {
      faceAngle += (float)(random(0, 320) - 160) / 1000.0f;
      wanderNext = now + (unsigned long)random(9000, 18000);
    }

    if (bloomTarget < 0.1f && !turning && !jetting && now > nextReverse) {
      bloomTarget = 1.0f;
      turning = true;
      turnRemaining = PI * ((float)random(90, 116) / 100.0f);
      reverseEnd = now + (unsigned long)random(7000, 11000);
    }
    if (bloomTarget > 0.5f && reverseEnd > 0 && now + 2800UL > reverseEnd) {
      bloomTarget = 0.0f;
    }
    if (reverseEnd > 0 && bloomTarget == 0.0f && bloom < 0.05f && now > reverseEnd) {
      nextReverse = now + (unsigned long)random(20000, 45000);
      reverseEnd = 0;
    }

    if (!jetting && bloom < 0.2f && !turning && now > nextJet) {
      jetting = true;
      jetEnd = now + (unsigned long)random(700, 1100);
    } else if (jetting && now > jetEnd) {
      jetting = false;
      nextJet = now + (unsigned long)random(18000, 34000);
    }
  }

  void steerInsideTank(bool urgentTank) {
    const float mX = canvasSize * 0.85f;
    const float mY = canvasSize * 0.6f;
    const float look = canvasSize * 2.4f;
    const float back = (float)chainLength * segmentFactor * canvasSize;

    const float tipX = toCanvas(pos.x);
    const float tipY = toCanvas(pos.y);
    const float cF = cosf(faceAngle);
    const float sF = sinf(faceAngle);
    const float aheadX = tipX + cF * look;
    const float aheadY = tipY + sF * look;
    const float headX = tipX - cF * back;
    const float headY = tipY - sF * back;
    const float widthC = toCanvas((float)xResolution);
    const float heightC = toCanvas((float)yResolution);

    const float spotsX[3] = {tipX, aheadX, headX};
    const float spotsY[3] = {tipY, aheadY, headY};
    const float spotsW[3] = {1.0f, 0.7f, 0.85f};

    float px = 0.0f;
    float py = 0.0f;
    for (int i = 0; i < 3; i++) {
      if (spotsX[i] < mX) px += (mX - spotsX[i]) * spotsW[i];
      else if (spotsX[i] > widthC - mX) px -= (spotsX[i] - (widthC - mX)) * spotsW[i];
      if (spotsY[i] < mY) py += (mY - spotsY[i]) * spotsW[i];
      else if (spotsY[i] > heightC - mY) py -= (spotsY[i] - (heightC - mY)) * spotsW[i];
    }
    if (fabsf(px) < 0.01f && fabsf(py) < 0.01f) return;

    const float inward = atan2f(py, px);
    const float diff = wrapPi(inward - faceAngle);
    const float urgency = constrain(sqrtf(px * px + py * py) / 90.0f, 0.0f, 1.0f);
    float step = flerp(0.006f, 0.02f, urgency);
    if (urgentTank) step *= 2.5f;
    faceAngle += constrain(diff, -step, step);
  }

  void containTip(bool stayInside) {
    if (!stayInside) {
      // Allow squids to spawn outside the display and glide naturally across the rim.
      const float leash = 18.0f * (float)PHYSICS_SCALE;
      if (pos.x < -leash) pos.x = -leash;
      if (pos.y < -leash) pos.y = -leash;
      if (pos.x > (float)xResolution + leash) pos.x = (float)xResolution + leash;
      if (pos.y > (float)yResolution + leash) pos.y = (float)yResolution + leash;
      return;
    }
    const float marginX = 3.0f * (float)PHYSICS_SCALE;
    const float marginY = 3.0f * (float)PHYSICS_SCALE;
    if (pos.x < marginX) pos.x = marginX;
    if (pos.y < marginY) pos.y = marginY;
    if (pos.x > (float)xResolution - marginX) pos.x = (float)xResolution - marginX;
    if (pos.y > (float)yResolution - marginY) pos.y = (float)yResolution - marginY;
  }

 public:
  SquidMotion(PVector pos, uint16_t xResolution, uint16_t yResolution)
      : Motion(pos, xResolution, yResolution) {
    canvasSize = SQUID_CANVAS_SIZE * SQUID_NORMAL_DRAW_SCALE;
    chainLength = SQUID_CHAIN_LENGTH;
    segmentFactor = 0.55f;
    cruiseSpeed = SQUID_CRUISE_SPEED;
    jetSpeed = SQUID_JET_SPEED;

    const float wPx = (float)xResolution / (float)PHYSICS_SCALE;
    const float hPx = (float)yResolution / (float)PHYSICS_SCALE;
    const float lx = pos.x / (float)PHYSICS_SCALE;
    const float ly = pos.y / (float)PHYSICS_SCALE;
    if (lx < 0.0f || lx >= wPx || ly < 0.0f || ly >= hPx) {
      const float inward = atan2f((float)yResolution * 0.5f - pos.y,
                                  (float)xResolution * 0.5f - pos.x);
      faceAngle = wrapPi(inward + (float)(random(0, 1000) - 500) / 1000.0f);
    } else {
      faceAngle = (float)random(0, 6283) / 1000.0f;
    }

    speed = cruiseSpeed;
    const float ls = ledSpeed(speed);
    vel = PVector(cosf(faceAngle) * ls, sinf(faceAngle) * ls);
    acc = PVector(0, 0);
    angle = faceAngle;

    const unsigned long now = millis();
    nextJet = now + (unsigned long)random(8000, 16000);
    nextReverse = now + (unsigned long)random(18000, 35000);
    wanderNext = now + (unsigned long)random(4000, 8000);
  }

  void doMotion() override {}

  void update(float age = AGE_ADULT, long co2 = CO2_OK,
              bool stayInside = false) override {
    this->co2 = co2;
    if (age < AGE_EGG) {
      vel = PVector(0, 0);
      speed = 0.0f;
      return;
    }

    const unsigned long now = millis();
    const bool chasing = followingFood;
    if (!chasing) {
      updateBehavior(now);
    } else {
      jetting = false;
      const float target = atan2f(foodDirection.y - pos.y, foodDirection.x - pos.x);
      faceAngle = lerpAngle(faceAngle, target, 0.08f);
    }

    steerInsideTank(stayInside || co2 > CO2_BAD);

    float heading = speed > 0.05f ? atan2f(vel.y, vel.x) : faceAngle;
    const float cruise = chasing ? cruiseSpeed * 1.7f : cruiseSpeed;

    if (jetting) {
      speed = flerp(speed, jetSpeed, 0.16f);
      heading = lerpAngle(heading, faceAngle, 0.18f);
    } else if (speed > cruise) {
      const float excess = speed - cruise;
      const float drag = excess * excess * 0.0016f + excess * 0.0045f;
      speed = speed - drag;
      if (speed < cruise) speed = cruise;
      heading = lerpAngle(heading, faceAngle, 0.01f);
    } else {
      speed = flerp(speed, cruise, 0.045f);
      heading = lerpAngle(heading, faceAngle, 0.045f);
    }
    if (speed > jetSpeed) speed = jetSpeed;

    if (co2 >= CO2_REALBAD) {
      speed = 0.0f;
    } else if (co2 > CO2_BAD) {
      const float t = (float)(co2 - CO2_BAD) / (float)(CO2_REALBAD - CO2_BAD);
      speed *= 1.0f - constrain(t, 0.0f, 1.0f);
    }

    const float ls = ledSpeed(speed);
    vel.set(cosf(heading) * ls, sinf(heading) * ls);
    pos += vel;
    containTip(stayInside);

    faceAngle = wrapPi(faceAngle);
    angle = faceAngle;
    followingFood = false;
  }
};

#endif  // SQUIDMOTION_H
