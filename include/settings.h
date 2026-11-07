#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "effect_catalog.h"

static constexpr uint32_t SETTINGS_MAGIC = 0xCA052026;
static_assert(EFFECT_COUNT>0 && EFFECT_COUNT<32,"Automatic selection needs a 32-bit mask");
static constexpr uint32_t ALL_AUTO_EFFECTS=(uint32_t{1}<<EFFECT_COUNT)-1;
static constexpr uint8_t AUTO_SETTINGS_VERSION=1;

// Preserve the original EEPROM field order and magic for existing settings.
struct Settings {
  uint32_t magic;
  uint8_t brightness;
  uint8_t speed;
  uint8_t effect;
  uint8_t mode;  // 0 auto, 1 clock, 2 effect
  uint8_t nightBrightness;
  uint8_t nightFrom;
  uint8_t nightUntil;
  uint8_t nightOn;
  uint16_t duration;
  uint8_t corner;  // 0 TL, 1 TR, 2 BL, 3 BR
  uint8_t vertical;
  uint8_t zigzag;
  uint8_t reserved;  // Former padding byte; keep copies/checksums deterministic.
  uint16_t crc;
  // Extension in previously unused bytes 20..27. Admin data starts at 32.
  uint32_t autoEffects;
  uint8_t autoOrder;  // 0 shuffled rounds, 1 ascending effect IDs
  uint8_t autoVersion;
  uint16_t autoCrc;
  uint8_t autoUploads;
  uint8_t uploadVersion;
  uint16_t uploadCrc;
};
static_assert(sizeof(Settings)==32 && offsetof(Settings,crc)==18 &&
              offsetof(Settings,autoEffects)==20 && offsetof(Settings,autoCrc)==26 && offsetof(Settings,uploadCrc)==30,
              "Keep the legacy 20-byte prefix and stay below admin offset 32");

inline uint16_t checksum(const Settings& settings) {
  const auto* bytes = reinterpret_cast<const uint8_t*>(&settings);
  uint16_t result = 0xBEEF;
  for (size_t i = 0; i < offsetof(Settings, crc); ++i)
    result = static_cast<uint16_t>((result << 5) | (result >> 11)) ^ bytes[i];
  return result;
}
inline uint16_t autoChecksum(const Settings& settings){
  const auto* bytes=reinterpret_cast<const uint8_t*>(&settings);
  uint16_t result=0xA707;
  for(size_t i=0;i<offsetof(Settings,autoCrc);++i)
    result=static_cast<uint16_t>((result<<5)|(result>>11))^bytes[i];
  return result;
}
inline void sealSettings(Settings& settings){
  settings.crc=checksum(settings);
  settings.autoVersion=AUTO_SETTINGS_VERSION;
  settings.autoCrc=autoChecksum(settings);
  settings.uploadVersion=1;
  uint16_t crc=0xC701;
  const auto* bytes=reinterpret_cast<const uint8_t*>(&settings);
  for(size_t i=0;i<offsetof(Settings,uploadCrc);++i)crc=static_cast<uint16_t>((crc<<5)|(crc>>11))^bytes[i];
  settings.uploadCrc=crc;
}
inline bool validUploadSettings(const Settings& settings){
  Settings copy=settings;sealSettings(copy);
  return settings.uploadVersion==1 && settings.uploadCrc==copy.uploadCrc;
}
inline void defaultAutomaticSettings(Settings& settings){
  settings.autoEffects=ALL_AUTO_EFFECTS;
  settings.autoOrder=0;
  settings.autoUploads=0;
  sealSettings(settings);
}

inline Settings defaultSettings() {
  Settings settings;
  memset(&settings, 0, sizeof(settings));
  settings.magic = SETTINGS_MAGIC;
  settings.brightness = 25;
  settings.speed = 55;
  settings.effect = 1;
  settings.nightOn = 1;
  settings.nightBrightness = 6;
  settings.nightFrom = 23;
  settings.nightUntil = 6;
  settings.duration = 15;
  settings.corner = 1;
  settings.zigzag = 1;
  defaultAutomaticSettings(settings);
  return settings;
}

inline bool validLegacySettings(const Settings& settings) {
  return settings.magic == SETTINGS_MAGIC && checksum(settings) == settings.crc &&
         settings.brightness >= 1 && settings.brightness <= 100 &&
         settings.speed >= 1 && settings.speed <= 100 &&
         settings.effect >= 1 && settings.effect <= TOTAL_EFFECT_COUNT &&
         settings.mode <= 2 && settings.duration >= 5 && settings.duration <= 120 &&
         settings.nightBrightness >= 1 && settings.nightBrightness <= 100 &&
         settings.nightFrom <= 23 && settings.nightUntil <= 23 &&
         settings.nightOn <= 1 && settings.corner <= 3 &&
         settings.vertical <= 1 && settings.zigzag <= 1;
}
inline bool validAutomaticSettings(const Settings& settings){
  return settings.autoVersion==AUTO_SETTINGS_VERSION && settings.autoOrder<=1 &&
         !(settings.autoEffects&~ALL_AUTO_EFFECTS) &&
         settings.autoCrc==autoChecksum(settings);
}
inline bool validSettings(const Settings& settings){
  return validLegacySettings(settings) && validAutomaticSettings(settings) && validUploadSettings(settings) &&
         (settings.autoEffects || settings.autoUploads);
}
// No write on boot. A later explicit save persists a migrated extension.
inline bool normalizeLoadedSettings(Settings& settings){
  if(!validLegacySettings(settings)){settings=defaultSettings();return false;}
  if(!validAutomaticSettings(settings)){defaultAutomaticSettings(settings);return false;}
  if(!validUploadSettings(settings)){
    settings.autoUploads=0;if(!settings.autoEffects)settings.autoEffects=ALL_AUTO_EFFECTS;
    sealSettings(settings);return false;
  }
  if(!settings.autoEffects && !settings.autoUploads){defaultAutomaticSettings(settings);return false;}
  return true;
}

// Reject empty, signed, fractional, overflowing and partially numeric input.
template <typename T>
inline bool parseSetting(const char* text, uint32_t minimum, uint32_t maximum, T& target) {
  if (!text || !*text) return false;
  uint32_t value = 0;
  for (const char* cursor = text; *cursor; ++cursor) {
    if (*cursor < '0' || *cursor > '9') return false;
    const uint64_t next = static_cast<uint64_t>(value) * 10 + (*cursor - '0');
    if (next > maximum) return false;
    value = static_cast<uint32_t>(next);
  }
  if (value < minimum) return false;
  target = static_cast<T>(value);
  return true;
}

inline bool applySetting(Settings& settings, const char* key, const char* text) {
  if (!key) return false;
  if (!strcmp(key, "brightness")) return parseSetting(text, 1, 100, settings.brightness);
  if (!strcmp(key, "speed")) return parseSetting(text, 1, 100, settings.speed);
  if (!strcmp(key, "effect")) return parseSetting(text, 1, TOTAL_EFFECT_COUNT, settings.effect);
  if (!strcmp(key, "mode")) return parseSetting(text, 0, 2, settings.mode);
  if (!strcmp(key, "duration")) return parseSetting(text, 5, 120, settings.duration);
  if (!strcmp(key, "nightBrightness")) return parseSetting(text, 1, 100, settings.nightBrightness);
  if (!strcmp(key, "nightFrom")) return parseSetting(text, 0, 23, settings.nightFrom);
  if (!strcmp(key, "nightUntil")) return parseSetting(text, 0, 23, settings.nightUntil);
  if (!strcmp(key, "nightOn")) return parseSetting(text, 0, 1, settings.nightOn);
  if (!strcmp(key, "corner")) return parseSetting(text, 0, 3, settings.corner);
  if (!strcmp(key, "vertical")) return parseSetting(text, 0, 1, settings.vertical);
  if (!strcmp(key, "zigzag")) return parseSetting(text, 0, 1, settings.zigzag);
  if (!strcmp(key, "autoEffects")) return parseSetting(text,0,ALL_AUTO_EFFECTS,settings.autoEffects);
  if (!strcmp(key, "autoUploads")) return parseSetting(text,0,255,settings.autoUploads);
  if (!strcmp(key, "autoOrder")) return parseSetting(text,0,1,settings.autoOrder);
  return false;
}
inline bool forgetUploadedAnimation(Settings& settings,uint8_t id){
  if(id<=EFFECT_COUNT || id>TOTAL_EFFECT_COUNT)return false;
  settings.autoUploads&=~(1u<<(id-EFFECT_COUNT-1));
  if(!settings.autoEffects && !settings.autoUploads){settings.autoEffects=1;settings.mode=1;}
  if(settings.effect==id)settings.effect=1;
  return true;
}

// Only publish the new runtime configuration after a successful flash commit.
// EEPROM.put is buffered; restore that buffer as well when commit fails.
template <typename Storage>
inline bool persistSettings(Storage& storage, Settings& current, Settings next, bool& persisted) {
  sealSettings(next);
  if (!validSettings(next)) return false;
  if (persisted && !memcmp(&current, &next, sizeof(Settings))) return true;
  storage.put(0, next);
  if (!storage.commit()) {
    storage.put(0, current);
    persisted = false;
    return false;
  }
  current = next;
  persisted = true;
  return true;
}
