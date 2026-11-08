#pragma once
#include <stdint.h>
#include <stddef.h>
#if defined(ARDUINO)
#include <Arduino.h>
#define CHAOS_FLASH(text) PSTR(text)
#else
#define CHAOS_FLASH(text) text
#endif
enum class ErrorCode:uint16_t {None=0,
#define CHAOS_ERROR(symbol,code,component,message,details,action,client) symbol,
#include "error_catalog.def"
#undef CHAOS_ERROR
};
struct ErrorDefinition {const char *code,*component,*message,*details,*action;};
#if !defined(ARDUINO) || defined(CHAOS_ERRORS_IMPLEMENTATION)
#if !defined(ARDUINO)
inline
#endif
ErrorDefinition errorDefinition(ErrorCode code){
  switch(code){
#define CHAOS_ERROR(symbol,code,component,message,details,action,client) case ErrorCode::symbol:return {CHAOS_FLASH(code),CHAOS_FLASH(component),CHAOS_FLASH(message),CHAOS_FLASH(details),CHAOS_FLASH(action)};
#include "error_catalog.def"
#undef CHAOS_ERROR
    case ErrorCode::None:return {CHAOS_FLASH(""),CHAOS_FLASH(""),CHAOS_FLASH(""),CHAOS_FLASH(""),CHAOS_FLASH("" )};
  }
  return errorDefinition(ErrorCode::SYS_UNKNOWN);
}
#else
// Keep the large string catalogue in exactly one translation unit. Otherwise
// ESP32 keeps unused copies in each unit's shared string section after COMDAT.
ErrorDefinition errorDefinition(ErrorCode code);
#endif
struct ErrorEvent {ErrorCode code=ErrorCode::None;uint32_t uptimeMs=0;uint16_t occurrences=0;};
class ErrorRegistry {
 public:
  static constexpr size_t CAPACITY=8;
  using Sink=void(*)(const ErrorEvent&);
  void setSink(Sink sink){sink_=sink;}
  void record(ErrorCode code,uint32_t now){
    if(code==ErrorCode::None)return;
    ++sequence_;
    if(count_ && events_[(next_+CAPACITY-1)%CAPACITY].code==code){
      auto& last=events_[(next_+CAPACITY-1)%CAPACITY];
      if(last.occurrences!=65535)++last.occurrences;
      if(static_cast<uint32_t>(now-last.uptimeMs)<30000){last.uptimeMs=now;return;}
    }
    events_[next_]={code,now,1};if(sink_)sink_(events_[next_]);next_=(next_+1)%CAPACITY;if(count_<CAPACITY)++count_;
  }
  size_t size()const{return count_;}
  const ErrorEvent& at(size_t chronological)const{return events_[(next_+CAPACITY-count_+chronological)%CAPACITY];}
  ErrorCode last()const{return count_?at(count_-1).code:ErrorCode::None;}
  uint32_t sequence()const{return sequence_;}
 private:
  ErrorEvent events_[CAPACITY];size_t next_=0,count_=0;Sink sink_=nullptr;
  uint32_t sequence_=0;
};
inline ErrorRegistry& errorRegistry(){static ErrorRegistry registry;return registry;}
inline void recordError(ErrorCode code){
#if defined(ARDUINO)
  errorRegistry().record(code,millis());
#else
  errorRegistry().record(code,0);
#endif
}
// Byte-at-a-time copies avoid treating ESP8266 flash pointers as RAM pointers.
inline char errorChar(const char* text,size_t i){
#if defined(ARDUINO) && defined(ESP8266)
  return static_cast<char>(pgm_read_byte(text+i));
#else
  return text[i];
#endif
}
inline void copyErrorText(char* output,size_t capacity,const char* flash){if(!capacity)return;size_t i=0;for(;i+1<capacity && errorChar(flash,i);++i)output[i]=errorChar(flash,i);output[i]=0;}
