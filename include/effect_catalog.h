#pragma once

#include <stdint.h>

// Keep IDs 1..16 stable so existing EEPROM/API settings retain their meaning.
enum LegacyEffectId : uint8_t {
  DIG_DUG = 17, QBERT, BLUE_FACE, PURPLE_FACE, RED_FACE, BITMAP_FIRE,
  CLASSIC_RAINBOW, CLASSIC_CONFETTI, RAINBOW_GLITTER, SINELON, BPM, JUGGLE,
  DRINK, BOMB_JACK
};

static constexpr const char* EFFECT_NAMES[] = {
  "Matrix-Regen", "Regenbogenwellen", "Feuer", "Sternenhimmel", "Konfetti",
  "Plasma", "Blauer Regen", "Scanner", "Glitzer", "Farbringe", "Snake-Spur",
  "Meteoriten", "Polarlicht", "Schachbrett-Puls", "Farbbalken", "Blasen",
  "DigiDug", "Q*bert", "Blaues Gesicht", "Lila Gesicht", "Rotes Gesicht",
  "Feuer (Original)", "Regenbogen (Original)", "Konfetti (Original)",
  "Regenbogen mit Glitzer", "Sinelon", "BPM", "Juggle", "Drink", "BombJack"
};
static constexpr uint8_t EFFECT_COUNT = sizeof(EFFECT_NAMES) / sizeof(EFFECT_NAMES[0]);
static_assert(EFFECT_COUNT == BOMB_JACK, "Effect IDs and names must agree");
static constexpr uint8_t UPLOAD_SLOTS=8;
static constexpr uint8_t TOTAL_EFFECT_COUNT=EFFECT_COUNT+UPLOAD_SLOTS;

inline bool isLegacyEffect(uint8_t effect) {
  return effect >= DIG_DUG && effect <= EFFECT_COUNT;
}

inline const char* effectName(uint8_t effect) {
  return effect >= 1 && effect <= EFFECT_COUNT ? EFFECT_NAMES[effect-1] : "Uhr";
}
