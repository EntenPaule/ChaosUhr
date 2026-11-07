// SPDX-License-Identifier: GPL-3.0-only
// Frame sequences/timings adapted from Micha1986's ChaosBlinkiBuntiUhr.
#include "legacy_animation.h"

#ifdef ARDUINO
#include <pgmspace.h>
#else
#define PROGMEM
#endif
#include "legacy_assets.h"

namespace {
using namespace legacyAssets;

static const LegacyFrame dug[] PROGMEM = {{DigDug01,250},{DigDug02,250}};
static const LegacyFrame qbert[] PROGMEM = {{Qbert01,500},{Qbert02,500}};
static const LegacyFrame blue[] PROGMEM = {
  {Face001,2000},{Face002,1000},{Face003,1000},{Face001,1000},
  {Face004,40},{Face005,40},{Face007,40},{Face008,60},
  {Face007,40},{Face005,40},{Face004,40}
};
static const LegacyFrame purple[] PROGMEM = {{Face202,700},{Face203,700}};
static const LegacyFrame red[] PROGMEM = {
  {Face301,100},{Face302,100},{Face303,100},{Face304,100},{Face305,100},
  {Face306,100},{Face307,100},{Face308,100},{Face309,100},{Face310,100},
  {Face311,100},{Face312,100},{Face313,100},{Face314,100},{Face315,100}
};
static const LegacyFrame fire[] PROGMEM = {
  {Feuer1,80},{Feuer2,80},{Feuer3,80},{Feuer4,80},{Feuer5,80},{Feuer6,80}
};
static const LegacyFrame drink[] PROGMEM = {
  {Drink01,200},{Drink02,200},{Drink03,200},{Drink04,200},
  {Drink05,200},{Drink06,200},{Drink07,200},{Drink08,200},
  {Drink09,200},{Drink010,200},{Drink011,200},{Drink012,200},
  {Drink013,200},{Drink014,200},{Drink015,200},{Drink016,200}
};
// Upstream provides these two pictures but does not schedule BombJack.
static const LegacyFrame bombJack[] PROGMEM = {{BombJack01,250},{BombJack02,250}};

struct Clip { const LegacyFrame* frames; uint8_t count; };
template <size_t N> constexpr Clip clip(const LegacyFrame (&frames)[N]) {
  return {frames, static_cast<uint8_t>(N)};
}
Clip findClip(uint8_t effect) {
  switch (effect) {
    case DIG_DUG: return clip(dug);
    case QBERT: return clip(qbert);
    case BLUE_FACE: return clip(blue);
    case PURPLE_FACE: return clip(purple);
    case RED_FACE: return clip(red);
    case BITMAP_FIRE: return clip(fire);
    case DRINK: return clip(drink);
    case BOMB_JACK: return clip(bombJack);
    default: return {nullptr,0};
  }
}
}

uint8_t legacyFrameCount(uint8_t effect) { return findClip(effect).count; }

LegacyFrame legacyFrame(uint8_t effect, uint8_t frame) {
  const Clip selected = findClip(effect);
  if (frame >= selected.count) return {nullptr,0};
#ifdef ARDUINO
  LegacyFrame result;
  memcpy_P(&result, selected.frames+frame, sizeof result);
  return result;
#else
  return selected.frames[frame];
#endif
}

LegacyFrame legacyFrameAt(uint8_t effect, uint32_t elapsedMs, uint8_t speed) {
  const uint8_t count = legacyFrameCount(effect);
  uint32_t cycleMs = 0;
  for (uint8_t i = 0; i < count; ++i) cycleMs += legacyFrame(effect,i).durationMs;
  if (!cycleMs) return {nullptr,0};
  uint32_t phase = legacyElapsed(elapsedMs,speed) % cycleMs;
  for (uint8_t i = 0; i < count; ++i) {
    const LegacyFrame frame = legacyFrame(effect,i);
    if (phase < frame.durationMs) return frame;
    phase -= frame.durationMs;
  }
  return {nullptr,0};
}

uint32_t legacyPixel(const LegacyFrame& frame, uint16_t sourceIndex) {
  if (!frame.pixels || sourceIndex >= MATRIX_WIDTH*MATRIX_HEIGHT) return 0;
#ifdef ARDUINO
  return pgm_read_dword(frame.pixels+sourceIndex);
#else
  return frame.pixels[sourceIndex];
#endif
}
