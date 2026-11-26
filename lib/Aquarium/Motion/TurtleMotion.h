#ifndef TURTLEMOTION_H
#define TURTLEMOTION_H

#include <math.h>
#include "Motion.h"

// Slow, serene sea-turtle swimming with stroke-coupled propulsion and smooth
// banking turns. Scales speeds by PHYSICS_SCALE so movement stays consistent
// across 78x78, 64x64, and 192x64 panels.
class TurtleMotion : public Motion {
 private:
  float targetAngle;
  float turnRate = 0.0f;
  float strokePhase;
  float strokeSpeed;
  float speedPx;
  unsigned long wanderNext = 0;

  static float flerp(float a, float b, float t) {
    return a + (b - a) * t;
  }

  static float wrapPi(float d) {
    while (d > PI) d -= TWO_PI;
    while (d < -PI) d += TWO_PI;
    return d;
  }

  bool steerInsideTank(bool stayInside) {
    const float wPx = (float)xResolution / (float)PHYSICS_SCALE;
    const float hPx = (float)yResolution / (float)PHYSICS_SCALE;
    const float lx = pos.x / (float)PHYSICS_SCALE;
    const float ly = pos.y / (float)PHYSICS_SCALE;
    const float pad = stayInside ? 12.0f : 7.0f;
    const float look = stayInside ? 12.0f : 9.0f;
    const float aheadX = lx + cosf(angle) * look;
    const float aheadY = ly + sinf(angle) * look;

    float px = 0.0f;
    float py = 0.0f;
    if (lx < pad) px += pad - lx;
    else if (lx > wPx - pad) px -= lx - (wPx - pad);
    if (ly < pad) py += pad - ly;
    else if (ly > hPx - pad) py -= ly - (hPx - pad);

    if (aheadX < pad) px += (pad - aheadX) * 0.7f;
    else if (aheadX > wPx - pad) px -= (aheadX - (wPx - pad)) * 0.7f;
    if (aheadY < pad) py += (pad - aheadY) * 0.7f;
    else if (aheadY > hPx - pad) py -= (aheadY - (hPx - pad)) * 0.7f;

    if (fabsf(px) < 0.05f && fabsf(py) < 0.05f) return false;

    const float inward = atan2f(py, px);
    const float diff = wrapPi(inward - angle);
    const float urgency =
        constrain(sqrtf(px * px + py * py) / (stayInside ? 14.0f : 20.0f), 0.0f, 1.0f);
    const float maxStep =
        stayInside ? flerp(0.016f, 0.052f, urgency) : flerp(0.008f, 0.028f, urgency);
    targetAngle = wrapPi(angle + constrain(diff, -maxStep * 2.5f, maxStep * 2.5f));
    return true;
  }

 public:
  TurtleMotion(PVector pos, uint16_t xResolution, uint16_t yResolution)
      : Motion(pos, xResolution, yResolution) {
    const float wPx = (float)xResolution / (float)PHYSICS_SCALE;
    const float hPx = (float)yResolution / (float)PHYSICS_SCALE;
    const float lx = pos.x / (float)PHYSICS_SCALE;
    const float ly = pos.y / (float)PHYSICS_SCALE;

    if (lx < 0.0f || lx >= wPx || ly < 0.0f || ly >= hPx) {
      const float inward = atan2f((float)yResolution * 0.5f - pos.y,
                                  (float)xResolution * 0.5f - pos.x);
      angle = wrapPi(inward + (float)(random(0, 1000) - 500) / 1000.0f);
    } else {
      angle = (float)random(0, 6283) / 1000.0f;
    }
    targetAngle = angle;

    // Slow, tranquil sea-turtle stroke cycle (~5.6 - 7.2s per stroke + glide at 30fps).
    strokePhase = (float)random(0, 6283) / 1000.0f;
    strokeSpeed = (TWO_PI / (float)random(5600, 7200)) * (1000.0f / 30.0f);
    speedPx = 0.14f;
    const float speedPhys = speedPx * (float)PHYSICS_SCALE;
    vel = PVector(cosf(angle) * speedPhys, sinf(angle) * speedPhys);
    acc = PVector(0, 0);

    wanderNext = millis() + (unsigned long)random(4000, 7500);
  }

  void doMotion() override {}

  float getStrokePhase() const override {
    return strokePhase;
  }

  float getTurnRate() const override {
    return turnRate;
  }

  void update(float age = AGE_ADULT, long co2 = CO2_OK,
              bool stayInside = false) override {
    this->co2 = co2;
    if (age < AGE_EGG) {
      vel = PVector(0, 0);
      turnRate = 0.0f;
      return;
    }

    float co2Scale = 1.0f;
    if (co2 >= CO2_REALBAD) {
      co2Scale = 0.0f;
    } else if (co2 > CO2_BAD) {
      co2Scale = 1.0f - constrain((float)(co2 - CO2_BAD) / (float)(CO2_REALBAD - CO2_BAD),
                                  0.0f, 1.0f);
    }

    const unsigned long now = millis();
    const bool chasing = followingFood;

    if (chasing) {
      targetAngle = atan2f(foodDirection.y - pos.y, foodDirection.x - pos.x);
    } else {
      const bool steeringWall = steerInsideTank(stayInside || co2 > CO2_BAD);
      if (!steeringWall && now > wanderNext) {
        targetAngle = wrapPi(angle + (float)(random(0, 1000) - 500) / 1000.0f);
        wanderNext = now + (unsigned long)random(4200, 7800);
      }
    }

    const float maxTurn =
        (chasing ? 0.048f : (stayInside ? 0.036f : 0.026f)) * (0.25f + 0.75f * co2Scale);
    const float diff = wrapPi(targetAngle - angle);
    const float step = constrain(diff * 0.11f, -maxTurn, maxTurn);
    angle = wrapPi(angle + step);
    turnRate = flerp(turnRate, step, 0.18f);

    // Advance flipper stroke; slightly brisker when chasing food.
    const float cadence = (chasing ? 1.45f : 1.0f) * (0.15f + 0.85f * co2Scale);
    strokePhase = fmodf(strokePhase + strokeSpeed * cadence, TWO_PI);

    // Asymmetric power-stroke thrust followed by a calm glide.
    const float cycle = strokePhase / TWO_PI;
    float thrust = 0.0f;
    if (cycle < 0.34f) {
      thrust = sinf((cycle / 0.34f) * PI);
    } else if (cycle < 0.72f) {
      thrust = 0.06f;
    } else {
      thrust = 0.01f;
    }

    // Speeds in LED pixels/frame (equivalent to 5.2..15.0 at PHYSICS_SCALE=50).
    const float glideSpeed = chasing ? 0.21f : 0.104f;
    const float burstSpeed = chasing ? 0.42f : 0.30f;
    const float targetSpeed = flerp(glideSpeed, burstSpeed, thrust) * co2Scale;
    const float accelRate = targetSpeed > speedPx ? 0.09f : 0.032f;
    speedPx = flerp(speedPx, targetSpeed, accelRate);

    const float speedPhys = speedPx * (float)PHYSICS_SCALE;
    vel.set(cosf(angle) * speedPhys, sinf(angle) * speedPhys);
    pos += vel;

    if (stayInside) {
      const float margin = 8.0f * (float)PHYSICS_SCALE;
      if (pos.x < margin) pos.x = margin;
      if (pos.y < margin) pos.y = margin;
      if (pos.x > (float)xResolution - margin) pos.x = (float)xResolution - margin;
      if (pos.y > (float)yResolution - margin) pos.y = (float)yResolution - margin;
    } else {
      const float leash = 16.0f * (float)PHYSICS_SCALE;
      if (pos.x < -leash) pos.x = -leash;
      if (pos.y < -leash) pos.y = -leash;
      if (pos.x > (float)xResolution + leash) pos.x = (float)xResolution + leash;
      if (pos.y > (float)yResolution + leash) pos.y = (float)yResolution + leash;
    }

    followingFood = false;
  }
};

#endif  // TURTLEMOTION_H
