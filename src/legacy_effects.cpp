// SPDX-License-Identifier: GPL-3.0-only
// Adapted from ChaosUhr / FastLED DemoReel100. See THIRD_PARTY.md.
#include <Arduino.h>
#include "legacy_effects.h"
#include "legacy_animation.h"

bool renderLegacyEffect(uint8_t effect, uint32_t elapsedMs, uint8_t speed,
                        CRGB* leds, const Settings& settings) {
  if (!isLegacyEffect(effect)) return false;
  const uint16_t count = MATRIX_WIDTH*MATRIX_HEIGHT;
  const LegacyFrame frame = legacyFrameAt(effect,elapsedMs,speed);
  if (frame.pixels) {
    for (uint16_t i = 0; i < count; ++i)
      leds[legacyTargetIndex(i,settings)] = CRGB(legacyPixel(frame,i));
    return true;
  }

  const uint32_t elapsed = legacyElapsed(elapsedMs,speed);
  const uint8_t hue = elapsed/20;
  // FastLED's beat functions accept a timebase, allowing the same virtual
  // time/speed as the bitmap player without delaying the network loop.
  const uint32_t timebase = millis()-elapsed;
  switch (effect) {
    case CLASSIC_RAINBOW:
    case RAINBOW_GLITTER:
      for (uint16_t i = 0; i < count; ++i)
        leds[legacyTargetIndex(i,settings)] = CHSV(hue*2+i*7,240,255);
      if (effect == RAINBOW_GLITTER && random8() < 50)
        leds[legacyTargetIndex(random16(count),settings)] += CRGB::White;
      break;
    case CLASSIC_CONFETTI:
      fadeToBlackBy(leds,count,50);
      leds[legacyTargetIndex(random16(count),settings)] += CHSV(hue*2+random16(150),210,255);
      break;
    case SINELON:
      fadeToBlackBy(leds,count,10);
      leds[legacyTargetIndex(beatsin16(25,0,count-1,timebase),settings)] += CHSV(hue,255,210);
      break;
    case BPM: {
      const CRGBPalette16 palette = PartyColors_p;
      const uint8_t beat = beatsin8(45,64,255,timebase);
      for (uint16_t i = 0; i < count; ++i)
        leds[legacyTargetIndex(i,settings)] = ColorFromPalette(palette,hue+i*2,beat-hue+i*10);
      break;
    }
    case JUGGLE: {
      fadeToBlackBy(leds,count,60);
      uint8_t dotHue = 0;
      for (uint8_t i = 0; i < 9; ++i) {
        leds[legacyTargetIndex(beatsin16(i+7,0,count-1,timebase),settings)] |= CHSV(dotHue,210,255);
        dotHue += 28;
      }
      break;
    }
    default: return false;
  }
  return true;
}
