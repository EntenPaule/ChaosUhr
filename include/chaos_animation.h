#pragma once
#include "chaos_errors.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static constexpr uint32_t CHAOS_MAX_BYTES=24*1024;
static constexpr uint8_t CHAOS_MAX_FRAMES=64,CHAOS_WIDTH=20,CHAOS_HEIGHT=15;
struct ChaosFrame {uint32_t offset;uint16_t bytes,ms;};
struct ChaosAnimation {
  char name[33]={};uint32_t palette[16]={};uint8_t colors=0,count=0;
  uint32_t duration=0;ChaosFrame frames[CHAOS_MAX_FRAMES]={};
};
template<typename File>class ChaosReader {
 public:
  explicit ChaosReader(File& source):source_(source){}
  int read(){
    if(cursor_==count_){
      const auto amount=source_.read(buffer_,sizeof buffer_);
      if(amount<=0 || static_cast<size_t>(amount)>sizeof buffer_)return -1;
      count_=amount;cursor_=0;
    }
    ++position_;return buffer_[cursor_++];
  }
  uint32_t position()const{return position_;}
 private:
  File& source_;uint8_t buffer_[128];size_t cursor_=0,count_=0;uint32_t position_=0;
};
inline int chaosHex(char c){return c>='0'&&c<='9'?c-'0':c>='A'&&c<='F'?c-'A'+10:-1;}
inline bool chaosNumber(const char* text,uint32_t low,uint32_t high,uint32_t& value){
  if(!text || !*text)return false;
  uint32_t result=0;
  for(;*text;++text){if(*text<'0'||*text>'9')return false;result=result*10+(*text-'0');if(result>high)return false;}
  if(result<low)return false;
  value=result;return true;
}
template<typename Reader>bool parseChaosAnimation(Reader& input,ChaosAnimation& result,char* error,void(*tick)()=nullptr,ErrorCode* code=nullptr){
  result=ChaosAnimation{};uint16_t lineNumber=0;char line[65];
  auto fail=[&](ErrorCode failure){if(code)*code=failure;char reason[80];copyErrorText(reason,sizeof reason,errorDefinition(failure).details);snprintf(error,96,"Zeile %u: %s",static_cast<unsigned>(lineNumber),reason);recordError(failure);return false;};
  // Return 1 for a complete line, 0 for EOF, -1 for invalid/oversized data.
  auto readLine=[&](){
    size_t length=0;int ch;bool seen=false;++lineNumber;
    while((ch=input.read())>=0){
      seen=true;if(input.position()>CHAOS_MAX_BYTES || ch==0)return -1;
      if(ch=='\n')break;
      if(length>=sizeof(line)-1)return -1;
      line[length++]=static_cast<char>(ch);
    }
    if(length && line[length-1]=='\r')--length;
    line[length]=0;return seen?1:0;
  };
  if(readLine()!=1 || strcmp(line,"CHAOSUHR 1"))return fail(ErrorCode::ANIMATION_FORMAT_101);
  if(readLine()!=1 || strncmp(line,"NAME ",5) || !line[5] || strlen(line+5)>32)return fail(ErrorCode::ANIMATION_FORMAT_102);
  for(const char* p=line+5;*p;++p)
    if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||strchr(" _.-",*p)))return fail(ErrorCode::ANIMATION_FORMAT_103);
  strcpy(result.name,line+5);uint32_t value;
  if(readLine()!=1 || strncmp(line,"PALETTE ",8) || !chaosNumber(line+8,1,16,value))return fail(ErrorCode::ANIMATION_FORMAT_104);
  result.colors=value;
  for(uint8_t i=0;i<result.colors;++i){
    if(readLine()!=1 || strlen(line)!=7 || line[0]!='#')return fail(ErrorCode::ANIMATION_FORMAT_105);
    uint32_t color=0;for(uint8_t j=1;j<7;++j){int digit=chaosHex(line[j]);if(digit<0)return fail(ErrorCode::ANIMATION_FORMAT_106);color=(color<<4)|digit;}
    result.palette[i]=color;
  }
  bool ended=false;
  for(;;){
    int state=readLine();if(state!=1)return fail(ErrorCode::ANIMATION_FORMAT_107);
    if(!line[0])continue;
    if(!strcmp(line,"END")){ended=true;break;}
    if(result.count>=CHAOS_MAX_FRAMES)return fail(ErrorCode::ANIMATION_FORMAT_108);
    if(strncmp(line,"FRAME ",6) || !chaosNumber(line+6,20,10000,value))return fail(ErrorCode::ANIMATION_FORMAT_109);
    ChaosFrame& frame=result.frames[result.count];frame.ms=value;frame.offset=input.position();
    for(uint8_t y=0;y<CHAOS_HEIGHT;++y){
      if(readLine()!=1 || strlen(line)!=CHAOS_WIDTH)return fail(ErrorCode::ANIMATION_FORMAT_110);
      for(char pixel:line){if(!pixel)break;const int index=chaosHex(pixel);if(index<0 || index>=result.colors)return fail(ErrorCode::ANIMATION_FORMAT_111);}
    }
    frame.bytes=input.position()-frame.offset;
    if(frame.bytes>330)return fail(ErrorCode::ANIMATION_FORMAT_112);
    ++result.count;result.duration+=frame.ms;if(tick)tick();
  }
  if(!ended || !result.count)return fail(ErrorCode::ANIMATION_FORMAT_113);
  for(;;){int state=readLine();if(state<0)return fail(ErrorCode::ANIMATION_FORMAT_114);if(!state)break;if(line[0])return fail(ErrorCode::ANIMATION_FORMAT_115);}
  error[0]=0;return true;
}
inline bool decodeChaosFrame(const ChaosAnimation& animation,uint8_t frame,const uint8_t* data,size_t length,uint8_t pixels[300]){
  if(frame>=animation.count || length!=animation.frames[frame].bytes)return false;
  size_t cursor=0;
  for(uint8_t y=0;y<CHAOS_HEIGHT;++y){
    for(uint8_t x=0;x<CHAOS_WIDTH;++x){
      if(cursor>=length)return false;
      const int index=chaosHex(data[cursor++]);
      if(index<0 || index>=animation.colors)return false;
      pixels[y*CHAOS_WIDTH+x]=index;
    }
    if(cursor<length && data[cursor]=='\r')++cursor;
    if(cursor>=length || data[cursor++]!='\n')return false;
  }
  return cursor==length;
}
