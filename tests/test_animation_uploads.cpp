#include "animation_library.h"
#include "memory_animation_fs.h"
#include "auto_playback.h"
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>

using Library=AnimationLibrary<MemoryAnimationFS,AnimationFile>;
static bool parse(const std::string& text,ChaosAnimation& result){
  MemoryAnimationFS fs;fs.begin();fs.put("/input",text);auto file=fs.open("/input","r");
  ChaosReader<AnimationFile> reader(file);char error[96];return parseChaosAnimation(reader,result,error);
}
static std::string replaceOnce(std::string text,const std::string& from,const std::string& to){
  auto at=text.find(from);assert(at!=std::string::npos);text.replace(at,from.size(),to);return text;
}
static uint8_t upload(Library& library,const std::string& text){
  library.startUpload(true);
  for(size_t at=0;at<text.size();at+=57)library.writeUpload(reinterpret_cast<const uint8_t*>(text.data()+at),std::min(size_t{57},text.size()-at));
  library.endUpload();return library.finishUpload(true);
}
static void testParser(const std::string& example){
  ChaosAnimation result;assert(parse(example,result));
  assert(std::string(result.name)=="Herz" && result.count==2 && result.colors==3 && result.duration==600);
  assert(result.palette[1]==0xff2020 && result.palette[2]==0xff8080);
  uint8_t pixels[300];const auto first=result.frames[0];
  assert(decodeChaosFrame(result,0,reinterpret_cast<const uint8_t*>(example.data()+first.offset),first.bytes,pixels));
  assert(pixels[0]==0 && pixels[25]==1);
  assert(!decodeChaosFrame(result,0,reinterpret_cast<const uint8_t*>(example.data()+first.offset),first.bytes-1,pixels));
  for(const auto& bad:std::vector<std::string>{
      replaceOnce(example,"CHAOSUHR 1","CHAOSUHR 2"),replaceOnce(example,"NAME Herz","NAME <script>"),
      replaceOnce(example,"NAME Herz","NAME "+std::string(33,'a')),replaceOnce(example,"PALETTE 3","PALETTE 0"),
      replaceOnce(example,"PALETTE 3","PALETTE 17"),replaceOnce(example,"#FF2020","#ff2020"),
      replaceOnce(example,"FRAME 300","FRAME 19"),replaceOnce(example,"FRAME 300","FRAME 10001"),
      replaceOnce(example,"FRAME 300","FRAME -1"),replaceOnce(example,"FRAME 300","FRAME 99999999999999999999999999999"),
      replaceOnce(example,std::string(20,'0'),std::string(19,'0')),replaceOnce(example,std::string(20,'0'),std::string(21,'0')),
      replaceOnce(example,std::string(20,'0'),std::string(19,'0')+"F"),replaceOnce(example,"END\n",""),
      example+"FRAME 100\n",example+std::string(CHAOS_MAX_BYTES,'\n'),std::string("\xef\xbb\xbf")+example,
      replaceOnce(example,"NAME Herz",std::string("NAME He\0rz",10))})assert(!parse(bad,result));
  std::string crlf;for(char ch:example){if(ch=='\n')crlf+='\r';crlf+=ch;}
  assert(parse(crlf,result));
  const auto windowsFrame=result.frames[0];
  assert(windowsFrame.bytes==330 && decodeChaosFrame(result,0,reinterpret_cast<const uint8_t*>(crlf.data()+windowsFrame.offset),windowsFrame.bytes,pixels));
  const auto start=example.find("FRAME "),second=example.find("FRAME ",start+1);
  const std::string frame=example.substr(start,second-start),header=example.substr(0,start);
  std::string many=header;for(unsigned i=0;i<64;++i)many+=frame;
  assert(parse(many+"END\n",result) && result.count==64);
  assert(!parse(many+frame+"END\n",result));assert(!parse(header+"END\n",result));
  std::cout<<"PASS .chaos parser: exact dimensions/palettes/timing, 64-frame bound, malformed input, oversized data, CRLF, indexed frame decoding\n";
}
static void testLibrary(const std::string& example){
  MemoryAnimationFS fs;fs.mountable=false;Library library(fs);library.begin();
  assert(!library.ready() && fs.formats==0);library.startUpload(true);library.endUpload();
  assert(!library.finishUpload(true) && library.status()==409 && fs.files.empty());
  assert(library.initialize() && fs.formats==1);assert(!library.initialize() && fs.formats==1);
  library.startUpload(false);library.writeUpload(reinterpret_cast<const uint8_t*>(example.data()),example.size());library.endUpload();
  assert(!library.finishUpload(false) && library.status()==403 && fs.files.empty());
  library.startUpload(true,false);library.endUpload();assert(!library.finishUpload(true) && library.status()==400);
  assert(upload(library,example)==31 && library.playable(31) && library.available()==1);
  assert(library.slot(0).frames==2 && std::string(library.slot(0).name)=="Herz");
  auto downloaded=library.openDownload(31);std::vector<uint8_t> bytes(downloaded.size());
  assert(downloaded.read(bytes.data(),bytes.size())==example.size());assert(std::string(bytes.begin(),bytes.end())==example);
  std::array<uint32_t,300> pixels;
  assert(library.render(31,0,[&](uint16_t i,uint32_t color){pixels.at(i)=color;}));assert(pixels[25]==0xff2020);
  assert(library.render(31,300,[&](uint16_t i,uint32_t color){pixels.at(i)=color;}));assert(pixels[25]==0xff8080);
  assert(library.render(31,600,[&](uint16_t i,uint32_t color){pixels.at(i)=color;}));assert(pixels[25]==0xff2020);
  const auto existing=*fs.files.at("/animations/0.chaos");
  assert(!upload(library,"not an animation") && library.status()==400 && library.available()==1);
  assert(*fs.files.at("/animations/0.chaos")==existing && fs.files.size()==1);
  library.startUpload(true);library.writeUpload(reinterpret_cast<const uint8_t*>(example.data()),12);library.abortUpload();
  assert(fs.files.size()==1);
  library.startUpload(true);std::vector<uint8_t> large(CHAOS_MAX_BYTES+1,'x');library.writeUpload(large.data(),large.size());library.endUpload();
  assert(!library.finishUpload(true) && library.status()==413 && fs.files.size()==1);
  library.startUpload(true);library.writeUpload(reinterpret_cast<const uint8_t*>(example.data()),example.size());library.endUpload();
  for(unsigned i=0;i<300;++i)library.startUpload(true);
  assert(!library.finishUpload(true) && library.status()==400 && fs.files.size()==1);
  fs.writeFails=true;assert(!upload(library,example) && library.status()==507);fs.writeFails=false;
  fs.renameFails=true;assert(!upload(library,example) && library.status()==500);fs.renameFails=false;
  assert(fs.files.size()==1 && library.available()==1);
  for(unsigned i=1;i<UPLOAD_SLOTS;++i)assert(upload(library,example)==31+i);
  assert(!upload(library,example) && library.status()==409 && fs.files.size()==8);
  assert(library.remove(33) && !library.exists(33));assert(upload(library,example)==33);
  assert(!library.remove(30) && !library.remove(39));
  fs.put("/animations/.incoming.chaos","unfinished");fs.put("/animations/7.chaos","corrupt");
  Library reboot(fs);reboot.begin();
  assert(!fs.exists("/animations/.incoming.chaos") && reboot.slot(7).present && !reboot.slot(7).valid);
  assert(reboot.remove(38));
  fs.formatFails=true;assert(!reboot.factoryReset() && !reboot.ready() && reboot.available()==0);
  fs.formatFails=false;assert(reboot.factoryReset() && reboot.available()==0 && fs.files.empty());
  std::cout<<"PASS animation storage: explicit initialization, authorization, bounded/chunked/aborted/multiple uploads, failed writes/rename, capacity/delete/reuse, reboot/corrupt files, timed playback/download and reset\n";
}
static void testUploadSelection(){
  Settings settings=defaultSettings();settings.autoEffects=0;settings.autoUploads=0x81;settings.effect=38;sealSettings(settings);
  assert(validSettings(settings));AutoPlayback playback;settings.autoOrder=1;
  auto unused=[](uint8_t)->uint8_t{assert(false);return 0;};
  assert(playback.next(settings,unused)==31);assert(playback.next(settings,unused)==38);assert(playback.next(settings,unused)==31);
  assert(playback.next(settings,unused,0x80)==38);assert(playback.next(settings,unused,0)==0);
  settings.autoOrder=0;playback.reset();uint64_t seen=0;
  for(unsigned i=0;i<2;++i){auto id=playback.next(settings,[](uint8_t){return 0;});assert(!(seen&autoEffectBit(id)));seen|=autoEffectBit(id);}
  assert(seen==(autoEffectBit(31)|autoEffectBit(38)));
  assert(forgetUploadedAnimation(settings,38) && settings.effect==1 && settings.autoUploads==1 && settings.autoEffects==0);
  assert(forgetUploadedAnimation(settings,31) && settings.autoUploads==0 && settings.autoEffects==1 && settings.mode==1);
  sealSettings(settings);assert(validSettings(settings));assert(!forgetUploadedAnimation(settings,30));
  Settings older=defaultSettings();older.brightness=10;older.autoEffects=2;older.autoOrder=1;sealSettings(older);
  memset(reinterpret_cast<uint8_t*>(&older)+28,0xff,4);
  assert(!normalizeLoadedSettings(older) && validSettings(older) && older.brightness==10 && older.autoEffects==2 && older.autoOrder==1 && older.autoUploads==0);
  std::cout<<"PASS uploaded selection: IDs 31/38, uploads-only playlist, unavailable files, random/order, deletion fallback, 28-byte migration\n";
}
int main(int argc,char** argv){
  assert(argc==2);std::ifstream input(argv[1],std::ios::binary);assert(input);
  const std::string example((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
  testParser(example);testLibrary(example);testUploadSelection();
}
