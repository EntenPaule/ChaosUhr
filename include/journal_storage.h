#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "chaos_errors.h"

// A persistent, atomic LittleFS mirror of all 128 EEPROM bytes. It contains no
// plaintext after a completed credentials migration. Never autoformat the FS.
template<typename Storage,typename Filesystem,typename File>class JournalStorage {
 public:
  JournalStorage(Storage& storage,Filesystem& fs):storage_(storage),fs_(fs){}
  void begin(size_t size){storage_.begin(size);if(size!=128){fault_=true;recordError(ErrorCode::SYS_STORAGE_SIZE);return;}recover();}
  size_t length(){return storage_.length();}
  uint8_t read(size_t at){return storage_.read(at);}
  void write(size_t at,uint8_t value){storage_.write(at,value);}
  template<typename T>void get(size_t at,T& value){storage_.get(at,value);}
  template<typename T>void put(size_t at,const T& value){storage_.put(at,value);}
  bool durable(){return !fault_ && fs_.begin();}
  bool fault()const{return fault_;}
  bool commit(){
    if(fault_){recordError(ErrorCode::FS_JOURNAL_CORRUPT);return false;}
    // Uninitialized legacy devices retain their existing login until their user
    // explicitly initializes storage. Settings can still be saved as before.
    if(!fs_.begin()){recordError(ErrorCode::FS_MOUNT);const bool ok=storage_.commit();if(!ok)recordError(ErrorCode::FS_EEPROM_WRITE);return ok;}
    Record next={};next.magic=0x384a5543;next.version=1;
    for(size_t i=0;i<128;++i)next.image[i]=storage_.read(i);
    next.checksum=checksum(next);
    File file=fs_.open(tempPath(),"w");
    if(!file){recordError(ErrorCode::FS_OPEN_WRITE);return false;}
    const bool written=file.write(reinterpret_cast<const uint8_t*>(&next),sizeof next)==sizeof next;
    file.flush();file.close();Record checked={};
    ErrorCode failure=ErrorCode::None;
    if(!written)failure=ErrorCode::FS_WRITE;
    else if(!load(tempPath(),checked))failure=ErrorCode::FS_VERIFY_READ;
    else if(memcmp(&checked,&next,sizeof next))failure=ErrorCode::FS_VERIFY_CONTENT;
    else if(!fs_.rename(tempPath(),path()))failure=ErrorCode::FS_RENAME;
    if(failure!=ErrorCode::None){if(!fs_.remove(tempPath()))recordError(ErrorCode::FS_REMOVE);recordError(failure);return false;}
    // The validated journal is the commit point. EEPROM is a mirror: even if its
    // write fails, acknowledge this durable image and recover it on next boot.
    syncPending_=!storage_.commit();if(syncPending_)recordError(ErrorCode::FS_EEPROM_MIRROR);
    return true;
  }
  bool recover(){
    if(!fs_.begin()){recordError(ErrorCode::FS_MOUNT);return true;}
    if(!fs_.exists(path())){fs_.remove(tempPath());return true;}
    Record saved={};if(!load(path(),saved)){fault_=true;recordError(ErrorCode::FS_JOURNAL_CORRUPT);return false;}
    bool different=false;for(size_t i=0;i<128;++i)if(storage_.read(i)!=saved.image[i])different=true;
    if(different){for(size_t i=0;i<128;++i)storage_.write(i,saved.image[i]);if(!storage_.commit()){fault_=true;recordError(ErrorCode::FS_EEPROM_WRITE);return false;}}
    fs_.remove(tempPath());fault_=false;syncPending_=false;return true;
  }
  // Preserve the durable reset marker while clearing EEPROM. Only then may the
  // caller format LittleFS. A reboot before formatting recovers the marker.
  bool clearForReset(){
    if(fault_){recordError(ErrorCode::FS_JOURNAL_CORRUPT);return false;}
    // This path is reached only after an explicit, persistently marked reset.
    // If the old FS was unreadable, establish the marker journal before erasing
    // EEPROM; a failed format must leave the EEPROM reset marker intact.
    if(!fs_.begin()){
      if(!fs_.format()){recordError(ErrorCode::FS_FORMAT);return false;}
      if(!fs_.begin()){recordError(ErrorCode::FS_MOUNT);return false;}
      if(!commit())return false;
    }
    for(size_t i=0;i<length();++i)storage_.write(i,0xff);
    const bool ok=storage_.commit();if(!ok)recordError(ErrorCode::FS_EEPROM_WRITE);return ok;
  }
  static const char* path(){return "/.chaosuhr-security.journal";}
 private:
  struct Record {uint32_t magic,version;uint8_t image[128];uint32_t checksum;};
  Storage& storage_;Filesystem& fs_;bool fault_=false,syncPending_=false;
  static const char* tempPath(){return "/.chaosuhr-security.pending";}
  static uint32_t checksum(const Record& record){
    uint32_t value=2166136261u;const auto* p=reinterpret_cast<const uint8_t*>(&record);
    for(size_t i=0;i<offsetof(Record,checksum);++i)value=(value^p[i])*16777619u;
    return value;
  }
  bool load(const char* name,Record& record){
    File file=fs_.open(name,"r");if(!file)return false;
    const bool ok=file.size()==sizeof record && file.read(reinterpret_cast<uint8_t*>(&record),sizeof record)==sizeof record;
    file.close();return ok && record.magic==0x384a5543 && record.version==1 && checksum(record)==record.checksum;
  }
};
