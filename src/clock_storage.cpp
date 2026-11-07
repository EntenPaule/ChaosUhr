#include "clock_storage.h"
#if defined(ARDUINO) && defined(ESP8266)
ClockStorage clockStorage(EEPROM,LittleFS);
#endif
