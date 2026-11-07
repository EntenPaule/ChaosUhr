#pragma once

#include <FastLED.h>
#include "settings.h"

// Render one frame only. Brightness, power limiting and show() stay in main.
bool renderLegacyEffect(uint8_t effect, uint32_t elapsedMs, uint8_t speed,
                        CRGB* leds, const Settings& settings);
