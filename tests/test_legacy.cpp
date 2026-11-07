#include "legacy_animation.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

static void testClips() {
  const struct {uint8_t effect, count; uint16_t duration;} clips[] = {
    {DIG_DUG,2,500},{QBERT,2,1000},{BLUE_FACE,11,5300},{PURPLE_FACE,2,1400},
    {RED_FACE,15,1500},{BITMAP_FIRE,6,480},{DRINK,16,3200},{BOMB_JACK,2,500}
  };
  for (const auto& clip : clips) {
    assert(legacyFrameCount(clip.effect) == clip.count);
    uint32_t offset = 0;
    for (uint8_t i = 0; i < clip.count; ++i) {
      const auto frame = legacyFrame(clip.effect,i);
      assert(frame.pixels && frame.durationMs);
      for (uint16_t pixel = 0; pixel < 300; ++pixel)
        assert(legacyPixel(frame,pixel) <= 0xffffff);
      assert(legacyPixel(frame,300) == 0);
      for (uint32_t elapsed : {offset,offset+frame.durationMs-1}) {
        const auto selected = legacyFrameAt(clip.effect,elapsed,55);
        assert(selected.pixels == frame.pixels && selected.durationMs == frame.durationMs);
      }
      assert(legacyFrameAt(clip.effect,offset*4,1).pixels == frame.pixels);
      assert(legacyFrameAt(clip.effect,offset/2,100).pixels == frame.pixels);
      offset += frame.durationMs;
    }
    assert(offset == clip.duration);
    assert(legacyFrameAt(clip.effect,offset,55).pixels == legacyFrame(clip.effect,0).pixels);
    assert(!legacyFrame(clip.effect,clip.count).pixels);
    assert(legacyFrameAt(clip.effect,UINT32_MAX,100).pixels);
  }
  for (uint8_t effect : {uint8_t(0),uint8_t(16),uint8_t(23),uint8_t(31),uint8_t(255)}) {
    assert(!legacyFrameCount(effect));
    assert(!legacyFrameAt(effect,1000,55).pixels);
  }
  assert(legacyPixel({nullptr,0},0) == 0);
  assert(legacyElapsed(1000,1) == 250 && legacyElapsed(1000,55) == 1000);
  assert(legacyElapsed(1000,100) == 2000);
  assert(legacyElapsed(1000,0) == 250 && legacyElapsed(1000,255) == 2000);
  puts("PASS legacy clips: exact cycle times, frame boundaries, speed, looping, valid reads");
}

static void testMappingAndIds() {
  const Settings original = defaultSettings();
  for (uint16_t index = 0; index < 300; ++index)
    assert(legacyTargetIndex(index,original) == index); // Exact original LED order.
  for (uint8_t corner = 0; corner < 4; ++corner)
  for (uint8_t vertical = 0; vertical < 2; ++vertical)
  for (uint8_t zigzag = 0; zigzag < 2; ++zigzag) {
    Settings settings = original;
    settings.corner=corner; settings.vertical=vertical; settings.zigzag=zigzag;
    bool seen[300] = {};
    for (uint16_t index = 0; index < 300; ++index) {
      const uint16_t target = legacyTargetIndex(index,settings);
      assert(target < 300 && !seen[target]); seen[target]=true;
    }
    for (int y = 0; y < MATRIX_HEIGHT; ++y)
    for (int x = 0; x < MATRIX_WIDTH; ++x)
      assert(legacyTargetIndex(matrixIndex(x,y,original),settings) == matrixIndex(x,y,settings));
  }
  assert(EFFECT_COUNT == 30);
  for (uint8_t id = 1; id <= EFFECT_COUNT; ++id) {
    assert(effectName(id)[0]);
    assert(isLegacyEffect(id) == (id >= 17));
    Settings saved = original;
    saved.effect=id; sealSettings(saved);
    assert(validSettings(saved));
  }
  assert(!isLegacyEffect(0) && !isLegacyEffect(31));
  puts("PASS legacy mapping/catalog: original order, all 16 mappings, stable old IDs, all 30 selectable");
}

int main(int argc, char** argv) {
  if (argc == 2 && !strcmp(argv[1],"--dump")) {
    for (uint8_t effect = DIG_DUG; effect <= EFFECT_COUNT; ++effect)
    for (uint8_t frame = 0; frame < legacyFrameCount(effect); ++frame) {
      const auto selected = legacyFrame(effect,frame);
      printf("%u %u %u",effect,frame,selected.durationMs);
      for (uint16_t pixel = 0; pixel < 300; ++pixel) printf(" %06x",legacyPixel(selected,pixel));
      puts("");
    }
    return 0;
  }
  testClips();
  testMappingAndIds();
}
