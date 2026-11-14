#pragma once
#include "brightness_control.h"
#include "chaos_errors.h"
// Deliberately separate from EEPROM/security journal and MqttConfig v1.
template<class FS,class File>class BrightnessStorage {
 public:
 explicit BrightnessStorage(FS& fs):fs_(fs){}
 bool load(BrightnessPolicy& policy){
  error_=ErrorCode::None;
  if(!fs_.begin())return fail(ErrorCode::FS_MOUNT);
  if(!fs_.exists(path())){policy={};return true;}
  uint8_t bytes[12];if(!read(path(),bytes))return fail(ErrorCode::FS_BRIGHTNESS_CORRUPT);
  policy={bytes[5]!=0,bytes[6]!=0};return true;
 }
 bool save(BrightnessPolicy& current,const BrightnessPolicy& next){
  error_=ErrorCode::None;
  if(!fs_.begin())return fail(ErrorCode::FS_MOUNT);
  if(current.nightOff==next.nightOff && current.mqtt==next.mqtt)return true;
  uint8_t bytes[12]={'C','U','B','P',1,static_cast<uint8_t>(next.nightOff),static_cast<uint8_t>(next.mqtt),0,0,0,0,0};
  uint32_t crc=checksum(bytes);for(unsigned i=0;i<4;++i)bytes[8+i]=crc>>(i*8);
  File file=fs_.open(temp(),"w");if(!file)return fail(ErrorCode::FS_OPEN_WRITE);
  const bool written=file.write(bytes,sizeof bytes)==sizeof bytes;file.flush();file.close();
  uint8_t checked[12];
  if(!written){cleanup();return fail(ErrorCode::FS_WRITE);}
  if(!read(temp(),checked) || memcmp(bytes,checked,sizeof bytes)){cleanup();return fail(ErrorCode::FS_VERIFY_READ);}
  if(!fs_.rename(temp(),path())){cleanup();return fail(ErrorCode::FS_RENAME);}
  current=next;return true;
 }
 ErrorCode errorCode()const{return error_;}
 static const char* path(){return "/brightness-policy.bin";}
 private:
 FS& fs_;ErrorCode error_=ErrorCode::None;
 static const char* temp(){return "/brightness-policy.pending";}
 static uint32_t checksum(const uint8_t* bytes){uint32_t value=2166136261u;for(unsigned i=0;i<8;++i)value=(value^bytes[i])*16777619u;return value;}
 bool read(const char* name,uint8_t* bytes){
  File file=fs_.open(name,"r");if(!file)return false;
  const bool ok=file.size()==12 && file.read(bytes,12)==12;file.close();if(!ok)return false;
  if(memcmp(bytes,"CUBP",4)||bytes[4]!=1||bytes[5]>1||bytes[6]>1||bytes[7])return false;
  const uint32_t expected=checksum(bytes);for(unsigned i=0;i<4;++i)if(bytes[8+i]!=static_cast<uint8_t>(expected>>(i*8)))return false;
  return true;
 }
 void cleanup(){if(fs_.exists(temp())&&!fs_.remove(temp()))recordError(ErrorCode::FS_REMOVE);}
 bool fail(ErrorCode error){error_=error;return false;}
};
