#pragma once
#include "Arduino.h"
inline void esp_fill_random(void* buffer,size_t length){
  ESP.random(static_cast<uint8_t*>(buffer),length);
}
