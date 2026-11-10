// The pinned Arduino prebuilt IDF enables flash core dumps, but our unchanged
// 4 MiB layout has no coredump partition. Interpose ONLY dump initialization and
// dump writing; panic handling, reset, UART backtraces and watchdogs stay intact.
// Do not silently change this with a framework upgrade: verify symbols/map again.
#if defined(ESP32) && defined(CHAOSUHR_ESP32_FOUNDATIONS)
#include <esp_attr.h>
#include <esp_core_dump.h>
#include <esp_arduino_version.h>
#if ESP_ARDUINO_VERSION_MAJOR != 3 || ESP_ARDUINO_VERSION_MINOR != 3 || ESP_ARDUINO_VERSION_PATCH != 12
#error Revalidate core dump interposition for the new framework before building
#endif
extern "C" void IRAM_ATTR __wrap_esp_core_dump_init(void) {}
extern "C" void IRAM_ATTR __wrap_esp_core_dump_write(panic_info_t*) {}
#endif
