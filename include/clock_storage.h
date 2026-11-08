#pragma once
#include <EEPROM.h>
#if defined(ARDUINO)
#include <LittleFS.h>
#include "journal_storage.h"
using ClockStorage=JournalStorage<EEPROMClass,decltype(LittleFS),fs::File>;
extern ClockStorage clockStorage;
#else
// Native adapter tests use their existing in-memory storage.
#define clockStorage EEPROM
#endif
