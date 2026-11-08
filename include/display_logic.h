#pragma once

#include "settings.h"

static constexpr uint8_t MATRIX_WIDTH = 20, MATRIX_HEIGHT = 15;

// Caller supplies coordinates inside the matrix bounds.
inline uint16_t matrixIndex(int x, int y, const Settings& settings) {
  if (settings.corner & 1) x = MATRIX_WIDTH - 1 - x;
  if (settings.corner & 2) y = MATRIX_HEIGHT - 1 - y;
  if (!settings.vertical) {
    if (settings.zigzag && (y & 1)) x = MATRIX_WIDTH - 1 - x;
    return static_cast<uint16_t>(y) * MATRIX_WIDTH + x;
  }
  if (settings.zigzag && (x & 1)) y = MATRIX_HEIGHT - 1 - y;
  return static_cast<uint16_t>(x) * MATRIX_HEIGHT + y;
}

inline bool isNightHour(uint8_t hour, const Settings& settings) {
  if (!settings.nightOn) return false;
  return settings.nightFrom <= settings.nightUntil ?
    (hour >= settings.nightFrom && hour < settings.nightUntil) :
    (hour >= settings.nightFrom || hour < settings.nightUntil);
}

struct SceneState {
  bool clock = true;
  uint8_t effect = 1, previousEffect = 0, page = 0;
  uint32_t startedAt = 0;

  void reset(uint32_t now, uint8_t selectedEffect) {
    clock = true;
    page = 0;
    effect = selectedEffect;
    startedAt = now;
  }

  bool due(uint32_t now, uint8_t mode, uint16_t duration) const {
    return mode == 0 && static_cast<uint32_t>(now - startedAt) >= duration * 1000UL;
  }

  void advance(uint32_t now, uint8_t nextEffect) {
    clock = !clock;
    startedAt = now;
    if (!clock) previousEffect = effect = nextEffect;
  }
};
