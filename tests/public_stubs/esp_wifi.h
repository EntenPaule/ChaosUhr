#pragma once
#include "Arduino.h"
static constexpr int ESP_OK=0,ESP_FAIL=-1;
static constexpr int WIFI_IF_STA=0;
struct wifi_config_t {struct {uint8_t ssid[32],password[64];}sta;};
inline int esp_wifi_get_config(int,wifi_config_t* value){memset(value,0,sizeof *value);memcpy(value->sta.ssid,WiFi.savedSsid.data(),WiFi.savedSsid.size());return ESP_OK;}
inline int esp_wifi_restore(){return ESP.eraseConfig()?ESP_OK:ESP_FAIL;}
