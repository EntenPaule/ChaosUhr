#pragma once
#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct MemoryAnimationFS;
struct AnimationFile {
  struct Handle {MemoryAnimationFS* fs;std::shared_ptr<std::vector<uint8_t>> bytes;size_t position=0;bool open=true,writable=false;};
  std::shared_ptr<Handle> handle;
  explicit operator bool()const;
  size_t read(uint8_t* output,size_t count);
  size_t write(const uint8_t* input,size_t count);
  bool seek(size_t offset);
  size_t size()const{return handle?handle->bytes->size():0;}
  void flush(){}
  void close(){if(handle)handle->open=false;}
};
struct MemoryAnimationFS {
  bool mountable=true,mounted=false,formatFails=false,writeFails=false,renameFails=false,removeFails=false;
  unsigned formats=0;size_t capacity=1024*1024;
  std::map<std::string,std::shared_ptr<std::vector<uint8_t>>> files;
  std::set<std::string> directories;
  bool begin(){mounted=mountable;return mounted;}
  void end(){mounted=false;}
  bool format(){++formats;if(formatFails)return false;files.clear();directories.clear();mountable=true;return true;}
  bool exists(const char* path)const{return mounted && (files.count(path)||directories.count(path));}
  bool mkdir(const char* path){if(!mounted)return false;directories.insert(path);return true;}
  bool remove(const char* path){if(!mounted||removeFails)return false;return files.erase(path)!=0;}
  bool rename(const char* from,const char* to){
    if(!mounted||renameFails||!files.count(from))return false;
    files[to]=files[from];files.erase(from);return true;
  }
  AnimationFile open(const char* path,const char* mode){
    if(!mounted)return {};
    const bool writable=std::string(mode)=="w";
    if(writable)files[path]=std::make_shared<std::vector<uint8_t>>();
    if(!files.count(path))return {};
    AnimationFile file;file.handle=std::make_shared<AnimationFile::Handle>(AnimationFile::Handle{this,files[path],0,true,writable});return file;
  }
  size_t used()const{size_t size=0;for(const auto& item:files)size+=item.second->size();return size;}
  void put(const std::string& path,const std::string& data){files[path]=std::make_shared<std::vector<uint8_t>>(data.begin(),data.end());}
};
inline AnimationFile::operator bool()const{return handle && handle->open && handle->fs->mounted;}
inline size_t AnimationFile::read(uint8_t* output,size_t count){
  if(!*this)return 0;
  count=std::min(count,handle->bytes->size()-handle->position);
  if(count)memcpy(output,handle->bytes->data()+handle->position,count);
  handle->position+=count;return count;
}
inline size_t AnimationFile::write(const uint8_t* input,size_t count){
  if(!*this || !handle->writable || handle->fs->writeFails)return 0;
  const size_t end=handle->position+count,extra=end>size()?end-size():0;
  if(handle->fs->used()+extra>handle->fs->capacity)return 0;
  if(end>size())handle->bytes->resize(end);
  memcpy(handle->bytes->data()+handle->position,input,count);handle->position=end;return count;
}
inline bool AnimationFile::seek(size_t offset){if(!*this||offset>size())return false;handle->position=offset;return true;}
