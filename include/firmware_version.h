#pragma once
#define CHAOSUHR_VERSION "2.0.11-beta.4"
#if defined(ESP32) && defined(CHAOSUHR_ESP32_FOUNDATIONS)
#undef CHAOSUHR_VERSION
#define CHAOSUHR_VERSION "2.0.11-beta.4.2"
#endif
