#pragma once

#include <Arduino.h>
#include <string.h>
#include "AquariumSettings.h"

/** True when the accelerometer reports portrait (90° or 270°). */
inline bool aquariumIsPortraitRotation(uint8_t rotation) {
  return rotation == 1 || rotation == 3;
}

/** Visual layout size for plant/coral placement (not the GFX buffer size). */
inline void aquariumLayoutDimensions(uint8_t matrixW, uint8_t matrixH, uint8_t rotation,
                                     uint8_t& layoutW, uint8_t& layoutH) {
  if (aquariumIsPortraitRotation(rotation)) {
    layoutW = min(matrixW, matrixH);
    layoutH = max(matrixW, matrixH);
  } else {
    layoutW = matrixW;
    layoutH = matrixH;
  }
}

/**
 * Map visual tank coords → GFX buffer (192×64). Matrix rotation stays at 0 in aquarium mode.
 * Supports all four mounting orientations (0–3).
 */
inline void aquariumVisualToBuffer(int16_t vx, int16_t vy, uint8_t rotation,
                                   uint8_t matrixW, uint8_t matrixH,
                                   int16_t& bx, int16_t& by) {
  switch (rotation & 3) {
    case 0:
      bx = vx;
      by = vy;
      break;
    case 1:
      bx = vy;
      by = vx;
      break;
    case 2:
      bx = (int16_t)matrixW - 1 - vx;
      by = (int16_t)matrixH - 1 - vy;
      break;
    case 3:
      // 180° from case 1 in visual space (other portrait mounting)
      bx = (int16_t)max(matrixW, matrixH) - 1 - vy;
      by = (int16_t)min(matrixW, matrixH) - 1 - vx;
      break;
    default:
      bx = vx;
      by = vy;
      break;
  }
}

/** Portrait uses one third as many ground plants/seagrass (minimum 1). */
inline uint8_t aquariumPortraitScaledCount(uint8_t landscapeCount, uint8_t rotation) {
  if (!aquariumIsPortraitRotation(rotation) || landscapeCount == 0) return landscapeCount;
  uint8_t scaled = landscapeCount / 3;
  return scaled > 0 ? scaled : 1;
}

// ---- Fountain point attractors ----
// Discrete points (not full-height lines): personal spawn anchor, waist center, top center.

inline float fountainDepth(float screenY, float height) {
  if (height < 1.0f) height = 1.0f;
  return constrain((height - screenY) / height, 0.0f, 1.0f);
}

inline void fountainPointPull(float sx, float sy, float ax, float ay, float strength,
                              float radius, float& outFx, float& outFy) {
  float dx = ax - sx;
  float dy = ay - sy;
  float dSq = dx * dx + dy * dy;
  if (dSq < 1.0f) return;
  float d = sqrtf(dSq);
  if (d > radius) return;
  float t = 1.0f - d / radius;
  outFx += (dx / d) * strength * t;
  outFy += (dy / d) * strength * t * 0.20f;
}

inline void fountainPointPush(float sx, float sy, float ax, float ay, float strength,
                              float radius, float& outFx, float& outFy) {
  float dx = sx - ax;
  float dy = sy - ay;
  float dSq = dx * dx + dy * dy;
  if (dSq < 1.0f) return;
  float d = sqrtf(dSq);
  if (d > radius) return;
  float t = 1.0f - d / radius;
  outFx += (dx / d) * strength * t;
  outFy += (dy / d) * strength * t * 0.15f;
}

inline void fountainPointSteering(float sx, float sy, float anchorX, float anchorY,
                                  float centerX, float height, float& outFx, float& outFy) {
  outFx = 0.0f;
  outFy = 0.0f;
  float depth = fountainDepth(sy, height);

  if (depth < 0.38f) {
    float w = 1.0f - depth / 0.38f;
    fountainPointPull(sx, sy, anchorX, anchorY, FOUNTAIN_ANCHOR_GAIN * w,
                      FOUNTAIN_POINT_RADIUS, outFx, outFy);
  }

  if (depth > 0.32f && depth < 0.68f) {
    float waistY = height * 0.5f;
    float w = 1.0f - fabsf(depth - 0.5f) / 0.18f;
    w = constrain(w, 0.0f, 1.0f);
    fountainPointPull(sx, sy, centerX, waistY, FOUNTAIN_WAIST_GAIN * w, FOUNTAIN_WAIST_RADIUS,
                      outFx, outFy);
  }

  if (depth > 0.62f) {
    float topY = height * 0.12f;
    float w = (depth - 0.62f) / 0.38f;
    fountainPointPush(sx, sy, centerX, topY, FOUNTAIN_TOP_GAIN * w, FOUNTAIN_POINT_RADIUS,
                      outFx, outFy);
  }
}

inline void fountainPointSteeringBoid(float sx, float sy, float anchorX, float anchorY,
                                      float centerX, float height, float& outFx, float& outFy) {
  outFx = 0.0f;
  outFy = 0.0f;
  float depth = fountainDepth(sy, height);
  float gain = FOUNTAIN_BOID_POINT_GAIN;

  if (depth < 0.38f) {
    float w = 1.0f - depth / 0.38f;
    fountainPointPull(sx, sy, anchorX, anchorY, gain * w * 2.5f, FOUNTAIN_POINT_RADIUS, outFx,
                      outFy);
  }
  if (depth > 0.32f && depth < 0.68f) {
    float w = constrain(1.0f - fabsf(depth - 0.5f) / 0.18f, 0.0f, 1.0f);
    fountainPointPull(sx, sy, centerX, height * 0.5f, gain * w * 1.8f, FOUNTAIN_WAIST_RADIUS,
                      outFx, outFy);
  }
  if (depth > 0.62f) {
    float w = (depth - 0.62f) / 0.38f;
    fountainPointPush(sx, sy, centerX, height * 0.12f, gain * w * 1.6f, FOUNTAIN_POINT_RADIUS,
                      outFx, outFy);
  }
}

// Spread spawns across width (even lanes + jitter) and stagger depth below the canvas.
inline void fountainSpawnPosition(int slotIndex, int slotCount, float width, float height,
                                  float& outX, float& outY, bool respawnJitter = false) {
  uint32_t hsh = (uint32_t)(slotIndex + 1) * 2654435761u;
  if (respawnJitter) {
    hsh ^= (uint32_t)millis();
    hsh ^= (uint32_t)random(0, 65535);
  }

  float fracX = (float)(hsh % 10000u) / 10000.0f;
  float fracY = (float)((hsh >> 13) % 10000u) / 10000.0f;
  float margin = FOUNTAIN_SPAWN_X_MARGIN_PX;
  float usableW = width - 2.0f * margin;
  if (usableW < 1.0f) usableW = 1.0f;

  if (slotCount > 1) {
    float lane = ((float)slotIndex + fracX * 0.75f) / (float)slotCount;
    outX = margin + lane * usableW;
  } else {
    outX = margin + fracX * usableW;
  }

  outY = height + FOUNTAIN_RESPAWN_MARGIN_PX + fracY * FOUNTAIN_SPAWN_Y_SPREAD_PX;
}

// ---- Jump performance (grid activity) ----
// Sum |Δdepth| across the 8x8 map vs the previous frame; EMA-smooth into 0..1 activity.

struct JumpGridActivityState {
  int16_t prevDepth[8][8] = {};
  bool havePrev = false;
  float smoothed = 0.0f;
};

inline float stepJumpGridActivity(const int16_t depthMap[8][8], int16_t minD, int16_t maxD,
                                  JumpGridActivityState& st) {
  float sum = 0.0f;
  if (st.havePrev) {
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        int16_t d = depthMap[y][x];
        int16_t p = st.prevDepth[y][x];
        bool dOk = (d > minD && d < maxD);
        bool pOk = (p > minD && p < maxD);
        if (!dOk && !pOk) continue;
        if (dOk && pOk) {
          sum += (float)abs((int)d - (int)p);
        } else {
          // Cell entered/left band — treat as activity.
          sum += (float)((maxD - minD) / 4);
        }
      }
    }
  }

  memcpy(st.prevDepth, depthMap, sizeof(st.prevDepth));
  st.havePrev = true;

  float raw = constrain(sum / JUMP_GRID_CHANGE_NORM_MM, 0.0f, 1.0f);
  st.smoothed += (raw - st.smoothed) * JUMP_GRID_CHANGE_SMOOTH;
  return st.smoothed;
}

/** Stable random home inside the margin buffer (screen coords). */
inline void jumpHomePosition(int slotIndex, float width, float height, float& outX, float& outY) {
  uint32_t hsh = (uint32_t)(slotIndex + 1) * 2654435761u;
  float fx = (float)(hsh % 10000u) / 10000.0f;
  float fy = (float)((hsh >> 13) % 10000u) / 10000.0f;
  float mx = width * JUMP_MARGIN_X_FRAC;
  float my = height * JUMP_MARGIN_Y_FRAC;
  float usableW = width - 2.0f * mx;
  float usableH = height - 2.0f * my;
  if (usableW < 1.0f) usableW = 1.0f;
  if (usableH < 1.0f) usableH = 1.0f;
  outX = mx + fx * usableW;
  outY = my + fy * usableH;
}
