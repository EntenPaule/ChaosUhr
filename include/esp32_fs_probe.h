#pragma once
#if defined(ESP32) && defined(ARDUINO)
#include <esp_partition.h>
// Read-only evidence, never infer corruption from a mount error alone.
inline const esp_partition_t* clockFsPartition(){
  const auto* p=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"spiffs");
  return p && p->address==0x3d0000 && p->size==0x30000 && ESP.getFlashChipSize()>=0x400000?p:nullptr;
}
inline const char* clockFsEvidence(){
  const auto* p=clockFsPartition();if(!p)return "layout-mismatch";
  uint8_t buffer[256];
  for(size_t offset=0;offset<p->size;offset+=sizeof buffer){
    if(esp_partition_read(p,offset,buffer,sizeof buffer)!=ESP_OK)return "read-error";
    for(auto value:buffer)if(value!=0xff)return "nonempty-mount-failed";
    yield();
  }
  return "erased-uninitialized";
}
#endif
