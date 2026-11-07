#pragma once
#include <EEPROM.h>
#if defined(ARDUINO) && defined(ESP8266)
#include <LittleFS.h>
#include "journal_storage.h"
using ClockStorage=JournalStorage<EEPROMClass,fs::FS,fs::File>;
extern ClockStorage clockStorage;
#else
// Native adapter tests and the deferred ESP32 port use their existing storage.
#define clockStorage EEPROM
#endif
