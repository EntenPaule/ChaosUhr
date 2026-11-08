#pragma once
#include "chaos_errors.h"
#include "chaos_animation.h"
#include "effect_catalog.h"

struct UploadedAnimation {
  bool present=false,valid=false;char name[33]={};uint8_t frames=0;uint32_t duration=0;
};

// Filesystem/File follow the small Arduino FS API; host tests use an in-memory FS.
template<typename Filesystem,typename File>class AnimationLibrary {
 public:
  explicit AnimationLibrary(Filesystem& fs,void(*tick)()=nullptr):fs_(fs),tick_(tick){}
  void setReserveGuard(bool(*guard)(size_t)){reserveGuard_=guard;}
  void begin(){ready_=fs_.begin();if(ready_){if(fs_.exists(tempPath())&&!fs_.remove(tempPath()))recordError(ErrorCode::FS_REMOVE);scan();}else recordError(ErrorCode::FS_MOUNT);}
  bool ready()const{return ready_;}
  const UploadedAnimation& slot(uint8_t index)const{return slots_[index];}
  uint8_t available()const{if(!ready_)return 0;uint8_t mask=0;for(uint8_t i=0;i<UPLOAD_SLOTS;++i)if(slots_[i].valid)mask|=1<<i;return mask;}
  bool exists(uint8_t id)const{return ready_ && id>EFFECT_COUNT && id<=TOTAL_EFFECT_COUNT && slots_[id-EFFECT_COUNT-1].present;}
  bool playable(uint8_t id)const{return ready_ && id>EFFECT_COUNT && id<=TOTAL_EFFECT_COUNT && slots_[id-EFFECT_COUNT-1].valid;}
  int status()const{return status_;}
  const char* error()const{return error_;}
  ErrorCode errorCode()const{return errorCode_;}
  bool initialize(){
    if(ready_){setError(409,ErrorCode::FS_ALREADY_READY);return false;}
    return factoryReset();
  }
  bool factoryReset(){
    resetPlayback();cleanup();fs_.end();ready_=false;
    if(!fs_.format()){setError(500,ErrorCode::FS_FORMAT);return false;}
    ready_=fs_.begin();if(!ready_){setError(500,ErrorCode::FS_MOUNT);return false;}
    scan();status_=200;return true;
  }
  void startUpload(bool authorized,bool fieldCorrect=true){
    if(parts_){parts_=2;setError(400,ErrorCode::CFG_UPLOAD_PARTS);return;}
    parts_=1;
    status_=200;error_[0]=0;
    if(!authorized){setError(403,ErrorCode::AUTH_REQUIRED);return;}
    if(!fieldCorrect){setError(400,ErrorCode::CFG_UPLOAD_FIELD);return;}
    if(!ready_){setError(409,ErrorCode::FS_MOUNT);return;}
    target_=UPLOAD_SLOTS;
    for(uint8_t i=0;i<UPLOAD_SLOTS;++i)if(!slots_[i].present){target_=i;break;}
    if(target_==UPLOAD_SLOTS){setError(409,ErrorCode::CFG_UPLOAD_SLOTS);return;}
    if(!fs_.exists("/animations") && !fs_.mkdir("/animations")){setError(500,ErrorCode::FS_DIRECTORY);return;}
    pending_=fs_.open(tempPath(),"w");
    if(!pending_)setError(500,ErrorCode::FS_OPEN_WRITE);
  }
  void writeUpload(const uint8_t* data,size_t length){
    if(status_!=200)return;
    if(!pending_){setError(500,ErrorCode::FS_OPEN_WRITE);return;}
    if(length>CHAOS_MAX_BYTES-bytes_){setError(413,ErrorCode::CFG_UPLOAD_SIZE);return;}
    if(reserveGuard_ && !reserveGuard_(length)){setError(507,ErrorCode::FS_RESERVE);return;}
    if(pending_.write(data,length)!=length){setError(507,ErrorCode::FS_WRITE);return;}
    bytes_+=length;
  }
  void endUpload(){if(pending_){pending_.flush();pending_.close();}complete_=true;}
  void abortUpload(){setError(400,ErrorCode::CFG_UPLOAD_ABORT);cleanup();}
  uint8_t finishUpload(bool authorized){
    if(!authorized)setError(403,ErrorCode::AUTH_REQUIRED);
    if(status_==200 && (parts_!=1 || !complete_ || !bytes_))setError(400,ErrorCode::CFG_UPLOAD_EMPTY);
    uint8_t id=0;
    if(status_==200){
      resetPlayback();File source=fs_.open(tempPath(),"r");
      if(!source || !parseFile(source,playback_)){
        if(status_==200)setError(400,ErrorCode::FS_READ);
      } else {
        char path[32];pathFor(target_,path);source.close();
        if(!fs_.rename(tempPath(),path))setError(500,ErrorCode::FS_RENAME);
        else{assignSlot(target_,playback_);id=EFFECT_COUNT+1+target_;status_=200;}
      }
      source.close();
    }
    cleanup();return id;
  }
  bool remove(uint8_t id){
    if(!exists(id)){setError(404,ErrorCode::CFG_UPLOAD_NOT_FOUND);return false;}
    resetPlayback();char path[32];pathFor(id-EFFECT_COUNT-1,path);
    if(!fs_.remove(path)){setError(500,ErrorCode::FS_REMOVE);return false;}
    slots_[id-EFFECT_COUNT-1]=UploadedAnimation{};status_=200;return true;
  }
  File openDownload(uint8_t id){
    if(!exists(id))return File();
    char path[32];pathFor(id-EFFECT_COUNT-1,path);return fs_.open(path,"r");
  }
  void resetPlayback(){playing_.close();playingId_=0;cachedFrame_=255;}
  template<typename Paint>bool render(uint8_t id,uint32_t elapsed,Paint paint){
    if(!playable(id))return false;
    if(playingId_!=id){
      resetPlayback();char path[32];pathFor(id-EFFECT_COUNT-1,path);playing_=fs_.open(path,"r");
      if(!playing_){setError(500,ErrorCode::FS_OPEN_READ);slots_[id-EFFECT_COUNT-1].valid=false;resetPlayback();return false;}
      if(!parseFile(playing_,playback_)){slots_[id-EFFECT_COUNT-1].valid=false;resetPlayback();return false;}
      playingId_=id;
    }
    uint32_t time=elapsed%playback_.duration;uint8_t frame=0;
    for(;frame+1<playback_.count && time>=playback_.frames[frame].ms;++frame)time-=playback_.frames[frame].ms;
    if(cachedFrame_!=frame){
      const ChaosFrame& description=playback_.frames[frame];uint8_t encoded[330];
      if(!playing_.seek(description.offset) || playing_.read(encoded,description.bytes)!=description.bytes){
        setError(500,ErrorCode::FS_READ);
        slots_[id-EFFECT_COUNT-1].valid=false;resetPlayback();return false;
      }
      if(!decodeChaosFrame(playback_,frame,encoded,description.bytes,pixels_)){
        setError(400,ErrorCode::ANIMATION_FORMAT_112);
        slots_[id-EFFECT_COUNT-1].valid=false;resetPlayback();return false;
      }
      cachedFrame_=frame;
    }
    for(uint16_t i=0;i<300;++i)paint(i,playback_.palette[pixels_[i]]);
    return true;
  }
 private:
  Filesystem& fs_;void(*tick_)();bool(*reserveGuard_)(size_t)=nullptr;bool ready_=false,complete_=false;
  UploadedAnimation slots_[UPLOAD_SLOTS];File pending_,playing_;
  ChaosAnimation playback_;uint8_t pixels_[300]={},playingId_=0,cachedFrame_=255;
  uint8_t parts_=0,target_=0;uint32_t bytes_=0;int status_=200;char error_[96]={};ErrorCode errorCode_=ErrorCode::None;
  static const char* tempPath(){return "/animations/.incoming.chaos";}
  static void pathFor(uint8_t index,char path[32]){snprintf(path,32,"/animations/%u.chaos",static_cast<unsigned>(index));}
  void setError(int status,ErrorCode code){status_=status;errorCode_=code;copyErrorText(error_,sizeof error_,errorDefinition(code).message);recordError(code);}
  void cleanup(){pending_.close();if(ready_&&fs_.exists(tempPath())&&!fs_.remove(tempPath()))recordError(ErrorCode::FS_REMOVE);parts_=0;bytes_=0;complete_=false;}
  bool parseFile(File& file,ChaosAnimation& definition){
    if(file.size()>CHAOS_MAX_BYTES){setError(400,ErrorCode::CFG_UPLOAD_SIZE);return false;}
    if(!file.seek(0)){setError(400,ErrorCode::FS_READ);return false;}
    ChaosReader<File> reader(file);
    if(!parseChaosAnimation(reader,definition,error_,tick_,&errorCode_)){status_=400;recordError(errorCode_);return false;}
    if(reader.position()!=file.size()){setError(400,ErrorCode::FS_READ);return false;}
    return true;
  }
  void assignSlot(uint8_t index,const ChaosAnimation& definition){
    auto& slot=slots_[index];slot.present=slot.valid=true;strcpy(slot.name,definition.name);
    slot.frames=definition.count;slot.duration=definition.duration;
  }
  void scan(){
    for(uint8_t i=0;i<UPLOAD_SLOTS;++i){
      slots_[i]=UploadedAnimation{};char path[32];pathFor(i,path);
      if(!fs_.exists(path))continue;
      slots_[i].present=true;strcpy(slots_[i].name,"Beschaedigter Upload");
      File file=fs_.open(path,"r");
      if(!file)recordError(ErrorCode::FS_OPEN_READ);
      if(file && parseFile(file,playback_))assignSlot(i,playback_);
      file.close();if(tick_)tick_();
    }
    status_=200;error_[0]=0;
  }
};
