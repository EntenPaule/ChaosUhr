#pragma once

#include <stdint.h>
#include "display_logic.h"

struct LegacyFrame {
  const uint32_t* pixels;
  uint16_t durationMs;
};

// Timings and RGB888 frames come from ChaosUhr commit 51aef401db22ef0fba3e5e334f141bf794d7505a.
uint8_t legacyFrameCount(uint8_t effect);
LegacyFrame legacyFrame(uint8_t effect, uint8_t frame);
LegacyFrame legacyFrameAt(uint8_t effect, uint32_t elapsedMs, uint8_t speed);
uint32_t legacyPixel(const LegacyFrame& frame, uint16_t sourceIndex);

// Default speed 55 preserves original bitmap timings; range is 0.25x..2x.
inline uint32_t legacyElapsed(uint32_t elapsedMs, uint8_t speed) {
  if (speed < 1) speed = 1;
  if (speed > 100) speed = 100;
  const uint16_t scale = speed <= 55 ? 250 + (speed-1)*750/54 : 1000 + (speed-55)*1000/45;
  return static_cast<uint32_t>(static_cast<uint64_t>(elapsedMs)*scale/1000);
}

// Original bitmap arrays are already in physical LED order (top-right,
// horizontal serpentine), not row-major images. Recover their logical position
// before applying the user's current wiring configuration.
inline uint16_t legacyTargetIndex(uint16_t sourceIndex, const Settings& settings) {
  const int y = sourceIndex / MATRIX_WIDTH;
  const int offset = sourceIndex % MATRIX_WIDTH;
  const int x = (y & 1) ? offset : MATRIX_WIDTH-1-offset;
  return matrixIndex(x, y, settings);
}
