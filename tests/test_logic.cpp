#include "display_logic.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

struct MemoryStorage {
  Settings buffer = defaultSettings();
  Settings flash = buffer;
  bool failCommit = false;
  unsigned commits = 0;
  void put(int address, const Settings& value) {
    assert(address == 0);
    buffer = value;
  }
  bool commit() {
    ++commits;
    if (failCommit) return false;
    flash = buffer;
    return true;
  }
};

static void testSettings() {
  const Settings defaults = defaultSettings();
  assert(validSettings(defaults));
  // Every byte, including the reserved byte and checksum bytes, is protected.
  for (size_t i = 0; i < sizeof(Settings); ++i) {
    Settings damaged = defaults;
    reinterpret_cast<unsigned char*>(&damaged)[i] ^= 1;
    assert(!validSettings(damaged));
  }
  struct Field { const char* name; unsigned minimum, maximum; };
  const Field fields[] = {
    {"brightness",1,100}, {"speed",1,100}, {"effect",1,TOTAL_EFFECT_COUNT}, {"mode",0,2},
    {"duration",5,120}, {"nightBrightness",1,100}, {"nightFrom",0,23},
    {"nightUntil",0,23}, {"nightOn",0,1}, {"corner",0,3},
    {"vertical",0,1}, {"zigzag",0,1}, {"autoOrder",0,1}
  };
  const char* invalid[] = {
    "", "-1", "+1", " 1", "1 ", "1x", "1.5", "1e1", "nan", "0x10",
    "65536", "4294967296", "9999999999999999999999999999999"
  };
  for (const auto& field : fields) {
    for (unsigned value = 0; value < 256; ++value) {
      Settings next = defaults;
      char text[4];
      snprintf(text, sizeof text, "%u", value);
      const bool accepted = applySetting(next, field.name, text);
      assert(accepted == (value >= field.minimum && value <= field.maximum));
      if (accepted) {
        sealSettings(next);
        assert(validSettings(next));
      } else assert(!memcmp(&next, &defaults, sizeof next));
    }
    for (const char* text : invalid) {
      Settings next = defaults;
      assert(!applySetting(next, field.name, text));
      assert(!memcmp(&next, &defaults, sizeof next));
    }
    Settings next = defaults;
    assert(!applySetting(next, field.name, nullptr));
  }
  Settings next = defaults;
  assert(!applySetting(next, "unknown", "1"));
  assert(!applySetting(next, nullptr, "1"));
  assert(applySetting(next, "effect", "16") && next.effect == 16);
  assert(applySetting(next, "effect", "30") && next.effect == BOMB_JACK);
  next.effect = TOTAL_EFFECT_COUNT+1;
  sealSettings(next);
  assert(!validSettings(next)); // CRC alone does not validate stored values.
  for(const char* text:{"1","2","65536","536870912","1073741823"}){
    next=defaults;assert(applySetting(next,"autoEffects",text));sealSettings(next);assert(validSettings(next));
  }
  for(const char* text:{"1073741824","2147483648","4294967295","4294967296","-1","1.5",""}){
    next=defaults;assert(!applySetting(next,"autoEffects",text));assert(!memcmp(&next,&defaults,sizeof next));
  }
  next=defaults;assert(applySetting(next,"autoEffects","0"));sealSettings(next);assert(!validSettings(next));
  assert(applySetting(next,"autoUploads","128"));sealSettings(next);assert(validSettings(next));
  assert(!applySetting(next,"autoUploads","256"));

  MemoryStorage storage;
  Settings current = defaults;
  bool persisted = false;
  assert(persistSettings(storage, current, current, persisted));
  assert(persisted && storage.commits == 1); // Defaults can be explicitly saved.
  assert(persistSettings(storage, current, current, persisted));
  assert(storage.commits == 1); // An identical save causes no flash operation.
  next = current;
  next.effect = 16;
  storage.failCommit = true;
  assert(!persistSettings(storage, current, next, persisted));
  assert(current.effect == 1 && storage.buffer.effect == 1 && !persisted);
  storage.failCommit = false;
  assert(persistSettings(storage, current, next, persisted));
  assert(current.effect == 16 && validSettings(current) && validSettings(storage.flash));
  const unsigned commits = storage.commits;
  next.effect = TOTAL_EFFECT_COUNT+1;
  assert(!persistSettings(storage, current, next, persisted));
  assert(current.effect == 16 && storage.commits == commits);
  puts("PASS settings: field boundaries, invalid input, old/new effect IDs, corrupt EEPROM, failed commits, unchanged saves");
}

static void testMapping() {
  Settings settings = defaultSettings();
  for (unsigned corner = 0; corner < 4; ++corner)
  for (unsigned vertical = 0; vertical < 2; ++vertical)
  for (unsigned zigzag = 0; zigzag < 2; ++zigzag) {
    settings.corner = corner; settings.vertical = vertical; settings.zigzag = zigzag;
    bool seen[300] = {};
    int xs[300], ys[300];
    for (int y = 0; y < MATRIX_HEIGHT; ++y)
    for (int x = 0; x < MATRIX_WIDTH; ++x) {
      const auto index = matrixIndex(x, y, settings);
      assert(index < 300 && !seen[index]);
      seen[index] = true; xs[index] = x; ys[index] = y;
    }
    assert(xs[0] == ((corner & 1) ? MATRIX_WIDTH-1 : 0));
    assert(ys[0] == ((corner & 2) ? MATRIX_HEIGHT-1 : 0));
    if (zigzag) for (unsigned i = 1; i < 300; ++i)
      assert(abs(xs[i]-xs[i-1]) + abs(ys[i]-ys[i-1]) == 1);
  }
  puts("PASS mapping: all 4800 coordinates, uniqueness, start corners, serpentine adjacency");
}

static void testNightAndScenes() {
  Settings settings = defaultSettings();
  for (unsigned enabled = 0; enabled < 2; ++enabled)
  for (unsigned start = 0; start < 24; ++start)
  for (unsigned end = 0; end < 24; ++end)
  for (unsigned hour = 0; hour < 24; ++hour) {
    settings.nightOn = enabled; settings.nightFrom = start; settings.nightUntil = end;
    const bool expected = enabled && ((hour+24-start)%24 < (end+24-start)%24);
    assert(isNightHour(hour, settings) == expected);
  }
  SceneState scene;
  settings.effect = 4;
  scene.reset(100, settings.effect);
  assert(scene.clock && !scene.due(5099, 0, 5) && scene.due(5100, 0, 5));
  assert(!scene.due(100000, 1, 5) && !scene.due(100000, 2, 5));
  scene.advance(5100, 16);
  assert(!scene.clock && scene.effect == 16 && scene.previousEffect == 16);
  assert(settings.effect == 4); // Automatic scenes preserve the user's selection.
  scene.advance(10100, 2);
  assert(scene.clock && scene.effect == 16);
  scene.reset(10200, settings.effect);
  assert(scene.clock && scene.effect == 4 && !scene.due(10200, 0, 5));
  scene.reset(UINT32_MAX-2000, 4);
  assert(!scene.due(2998, 0, 5) && scene.due(2999, 0, 5));
  puts("PASS night/scenes: 27648 night schedules, mode changes, selected effect, millis rollover");
}

int main() {
  testSettings();
  testMapping();
  testNightAndScenes();
}
