#pragma once
#include "Arduino.h"
static constexpr int ESP_OK=0,ESP_FAIL=-1;
inline int esp_wifi_restore(){return ESP.eraseConfig()?ESP_OK:ESP_FAIL;}
