#include <cstring>
#include <cassert>
#include "esp32_fs_probe.h"
ProbeESP ESP;
esp_partition_t partition{0x3d0000,0x30000};
bool present=true,readError=false;size_t dirty=0x30000,reads=0;
void yield(){}
const esp_partition_t* esp_partition_find_first(int type,int subtype,const char* label){
 assert(type==1 && subtype==0x82 && !strcmp(label,"spiffs"));return present?&partition:nullptr;
}
int esp_partition_read(const esp_partition_t* p,size_t offset,void* data,size_t size){
 assert(p==&partition && offset+size<=p->size);++reads;
 if(readError)return -1;
 memset(data,255,size);if(dirty>=offset && dirty<offset+size)static_cast<uint8_t*>(data)[dirty-offset]=0;return 0;
}
int main(){
 assert(!strcmp(clockFsEvidence(),"erased-uninitialized") && reads==768);
 dirty=partition.size-1;assert(!strcmp(clockFsEvidence(),"nonempty-mount-failed"));
 readError=true;assert(!strcmp(clockFsEvidence(),"read-error"));readError=false;
 partition.address=0x9000;assert(!clockFsPartition());partition.address=0x3d0000;
 ESP.flash=0x200000;assert(!clockFsPartition());ESP.flash=0x800000;assert(clockFsPartition());
 present=false;assert(!strcmp(clockFsEvidence(),"layout-mismatch"));
}
