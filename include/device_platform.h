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
