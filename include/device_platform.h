#pragma once

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <esp_random.h>
#include <esp_wifi.h>
using ClockWebServer = WebServer;
static constexpr char DEVICE_FAMILY[]="esp32";
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
using ClockWebServer = ESP8266WebServer;
static constexpr char DEVICE_FAMILY[]="esp8266";
#endif

// Arduino-ESP32 chooses FLASH versus RAM at the first low-level radio init.
// Changing persistent() afterwards does not change an already selected store.
inline void configureWifiPersistence(){
#if defined(ESP32)
  WiFi.persistent(true);
#else
  WiFi.persistent(false);
#endif
}

enum class SavedWifiState:uint8_t {Missing,Available,Unreadable};
inline SavedWifiState savedWifiState(){
#if defined(ESP32)
  // STA.SSID() in core 3.x reports the connected AP, not saved credentials.
  wifi_config_t config={};
  if(esp_wifi_get_config(WIFI_IF_STA,&config)!=ESP_OK)return SavedWifiState::Unreadable;
  const bool present=config.sta.ssid[0]!=0;
  volatile uint8_t* bytes=reinterpret_cast<volatile uint8_t*>(&config);
  for(size_t i=0;i<sizeof config;++i)bytes[i]=0;
  return present?SavedWifiState::Available:SavedWifiState::Missing;
#else
  return WiFi.SSID().length()?SavedWifiState::Available:SavedWifiState::Missing;
#endif
}

inline uint32_t deviceIdentifier(){
#if defined(ESP32)
  return static_cast<uint32_t>(ESP.getEfuseMac()>>24)&0xffffff;
#else
  return ESP.getChipId();
#endif
}
inline void deviceRandomBytes(uint8_t* bytes,size_t count){
#if defined(ESP32)
  esp_fill_random(bytes,count);
#else
  ESP.random(bytes,count);
#endif
}
inline uint32_t deviceFlashSize(){
#if defined(ESP32)
  return ESP.getFlashChipSize();
#else
  return ESP.getFlashChipRealSize();
#endif
}
inline void collectClockHeaders(ClockWebServer& server){
  // HTTPUpdateServer replaces the collected headers during setup on ESP32.
  // Restore the combined set afterwards, retaining both reset and OTA guards.
#if defined(ESP32)
  const char* headers[]={"X-CSRF-Token","Origin","Host","X-Firmware-SHA256"};
  server.collectHeaders(headers,4);
#else
  server.collectHeaders("X-CSRF-Token","Origin","Host","X-Firmware-SHA256");
#endif
}
inline bool eraseDeviceWifi(){
  WiFi.setAutoReconnect(false);
#if defined(ESP32)
  if(!WiFi.mode(WIFI_STA)) return false;
  WiFi.disconnect(false,false); // A resumed reset may already be disconnected.
  // Restore only the Wi-Fi driver's NVS data, retaining the pending-reset
  // marker in the separate EEPROM namespace until the erase succeeds.
  return esp_wifi_restore()==ESP_OK;
#else
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);WiFi.disconnect(true,true);WiFi.persistent(false);
  return ESP.eraseConfig();
#endif
}
