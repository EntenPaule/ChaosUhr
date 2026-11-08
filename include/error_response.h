#pragma once
#include <stdio.h>
#include "chaos_errors.h"
// Include after Arduino.h (or the native String adapter).
inline String errorText(const char* text){
#if defined(ARDUINO)
  return String(FPSTR(text));
#else
  return String(text);
#endif
}
inline String errorJsonText(const char* text){
  String result="\"";
  for(size_t i=0;errorChar(text,i);++i){const char c=errorChar(text,i);
    if(c=='"'||c=='\\')result+='\\';
    if(c=='\n')result+="\\n";else if(c=='\r')result+="\\r";else if(c=='\t')result+="\\t";else result+=c;
  }
  return result+"\"";
}
inline String errorJson(ErrorCode code,int status=0,const char* field=nullptr){
  const auto d=errorDefinition(code);char number[16];snprintf(number,sizeof number,"%d",status);
  return String("{\"code\":")+errorJsonText(d.code)+",\"component\":"+errorJsonText(d.component)+",\"message\":"+errorJsonText(d.message)+",\"details\":"+errorJsonText(d.details)+",\"action\":"+errorJsonText(d.action)+",\"status\":"+number+(field?String(",\"field\":")+errorJsonText(field):String(""))+"}";
}
template<typename Server>inline void sendError(Server& server,int status,ErrorCode code){recordError(code);server.send(status,"application/json; charset=utf-8",errorJson(code,status));}
// Logging must remain usable when heap allocation itself has failed.
template<typename Stream>inline void printErrorEvent(Stream& stream,const ErrorEvent& event){
  auto ram=[&](const char* text){for(size_t i=0;text[i];++i)stream.write(static_cast<uint8_t>(text[i]));};
  auto flash=[&](const char* text){char buffer[32];size_t at=0;
    while(errorChar(text,at)){size_t size=0;while(size<sizeof buffer&&errorChar(text,at))buffer[size++]=errorChar(text,at++);stream.write(reinterpret_cast<const uint8_t*>(buffer),size);}
  };
  const auto d=errorDefinition(event.code);ram("[");flash(d.code);ram("] ");flash(d.message);
  ram(" | ");flash(d.details);ram(" | ");flash(d.action);ram(" | uptime=");
  char number[16];snprintf(number,sizeof number,"%lu",static_cast<unsigned long>(event.uptimeMs));ram(number);ram(" ms\n");
}
inline int errorHttpStatus(ErrorCode code){
  switch(code){
    case ErrorCode::HTTP_TIMEOUT:return 408;
    case ErrorCode::HTTP_HEADERS:return 431;
    case ErrorCode::HTTP_URI:return 414;
    case ErrorCode::HTTP_BODY_LIMIT:return 413;
    default:return 400;
  }
}
