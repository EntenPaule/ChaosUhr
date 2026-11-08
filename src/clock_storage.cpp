#include "clock_storage.h"
#if defined(ARDUINO)
ClockStorage clockStorage(EEPROM,LittleFS);
#endif
