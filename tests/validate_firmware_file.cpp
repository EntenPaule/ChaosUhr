// Offline acceptance/rejection tests of the production validator on real files.
#include "firmware_upload.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

static std::vector<uint8_t> readFile(const char* path) {
  std::ifstream file(path, std::ios::binary);
  assert(file);
  return {std::istreambuf_iterator<char>(file), {}};
}

static bool feedFile(FirmwareValidator& validator, const std::vector<uint8_t>& bytes) {
  for (size_t at = 0; at < bytes.size(); at += 977)
    if (!validator.feed(bytes.data() + at, std::min<size_t>(977, bytes.size() - at))) return false;
  return true;
}

int main(int argc, char** argv) {
  assert(argc == 4); // matching OTA, other chip's OTA, full ESP32 factory image
  const auto data = readFile(argv[1]);
  assert(data.size() > 4096);
  Sha256Stream sha;
  sha.add(data.data(), data.size());
  uint8_t digest[32];
  assert(sha.digest(digest));
  char hex[65];
  for (size_t i = 0; i < 32; ++i) std::snprintf(hex + 2 * i, 3, "%02x", digest[i]);
  FirmwareValidator valid;
  assert(valid.expectSha256(hex));
  assert(feedFile(valid, data));
  assert(valid.complete() && valid.size() == data.size());

  auto changed = data;
  changed[changed.size() / 2] ^= 1;
  FirmwareValidator bad;
  assert(bad.expectSha256(hex));
  assert(!feedFile(bad, changed) || !bad.complete());

  auto shortened = data;
  shortened.pop_back();
  FirmwareValidator truncated;
  assert(!feedFile(truncated, shortened) || !truncated.complete());

  FirmwareValidator otherChip;
  assert(!feedFile(otherChip, readFile(argv[2])) || !otherChip.complete());
  FirmwareValidator factory;
  assert(!feedFile(factory, readFile(argv[3])) || !factory.complete());

  hex[0] = hex[0] == '0' ? '1' : '0';
  FirmwareValidator wrongSha;
  assert(wrongSha.expectSha256(hex));
  assert(feedFile(wrongSha, data) && !wrongSha.complete());
  std::cout << "PASS real OTA file: accepted; mutation, truncation, other chip, factory and wrong SHA rejected: "
            << FIRMWARE_PRODUCT << '\n';
}
