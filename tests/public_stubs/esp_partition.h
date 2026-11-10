#pragma once
#include <cstddef>
#include <cstdint>
struct esp_partition_t{uint32_t address,size;};
constexpr int ESP_PARTITION_TYPE_DATA=1,ESP_PARTITION_SUBTYPE_DATA_SPIFFS=0x82,ESP_OK=0;
const esp_partition_t* esp_partition_find_first(int,int,const char*);
int esp_partition_read(const esp_partition_t*,size_t,void*,size_t);
struct ProbeESP{uint32_t flash=0x400000;uint32_t getFlashChipSize(){return flash;}};
extern ProbeESP ESP;
void yield();
