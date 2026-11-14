#pragma once

#include <Arduino.h>

// Quarter-turn mapping for a non-square panel.
// The HUB75 driver rotation swaps width and height and then clips, so a
// 192x64 panel keeps its buffer at rotation 0 and these functions move
// upright (visual) coordinates into that buffer.
// Landscape cases 0 and 2 match the mount. Portrait cases 1 and 3 are
// turned 180 degrees from the 3-display-test mapping so the floor stays down.

inline bool panelIsPortrait(uint8_t rotation) {
  return rotation == 1 || rotation == 3;
}

inline void panelVisualSize(uint8_t bufferW, uint8_t bufferH, uint8_t rotation,
                            uint8_t& visualW, uint8_t& visualH) {
  if (panelIsPortrait(rotation)) {
    visualW = bufferW < bufferH ? bufferW : bufferH;
    visualH = bufferW > bufferH ? bufferW : bufferH;
  } else {
    visualW = bufferW;
    visualH = bufferH;
  }
}

inline void panelVisualToBuffer(int16_t vx, int16_t vy, uint8_t rotation,
                                uint8_t bufferW, uint8_t bufferH,
                                int16_t& bx, int16_t& by) {
  const int16_t longEdge = bufferW > bufferH ? bufferW : bufferH;
  const int16_t shortEdge = bufferW < bufferH ? bufferW : bufferH;
  switch (rotation & 3) {
    case 0:
      bx = vx;
      by = vy;
      break;
    case 1:
      bx = longEdge - 1 - vy;
      by = shortEdge - 1 - vx;
      break;
    case 2:
      bx = (int16_t)bufferW - 1 - vx;
      by = (int16_t)bufferH - 1 - vy;
      break;
    case 3:
      bx = vy;
      by = vx;
      break;
    default:
      bx = vx;
      by = vy;
      break;
  }
}

inline void panelBufferToVisual(int16_t bx, int16_t by, uint8_t rotation,
                                uint8_t bufferW, uint8_t bufferH,
                                int16_t& vx, int16_t& vy) {
  const int16_t longEdge = bufferW > bufferH ? bufferW : bufferH;
  const int16_t shortEdge = bufferW < bufferH ? bufferW : bufferH;
  switch (rotation & 3) {
    case 0:
      vx = bx;
      vy = by;
      break;
    case 1:
      vx = shortEdge - 1 - by;
      vy = longEdge - 1 - bx;
      break;
    case 2:
      vx = (int16_t)bufferW - 1 - bx;
      vy = (int16_t)bufferH - 1 - by;
      break;
    case 3:
      vx = by;
      vy = bx;
      break;
    default:
      vx = bx;
      vy = by;
      break;
  }
}

// Portrait floor is the short edge, so ground plants and seagrass are cut to a third.
inline uint8_t panelScaledCount(uint8_t count, bool narrowFloor) {
  if (!narrowFloor || count == 0) return count;
  uint8_t scaled = count / 3;
  return scaled > 0 ? scaled : 1;
}
