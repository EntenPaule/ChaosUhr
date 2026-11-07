#pragma once
#include "Arduino.h"
#include <functional>
#include <map>
#include <vector>
enum HTTPMethod {HTTP_ANY,HTTP_GET,HTTP_POST};
struct FakeClient {uint32_t address=0xc0a80401;uint32_t localIP(){return address;}};
class ESP8266WebServer {
 public:
  struct Route{std::string path;HTTPMethod method;std::function<void()> callback;};
  std::vector<Route> routes;
  std::vector<std::string> collectedHeaders;
  std::map<std::string,std::string> arguments,headers;
  std::string user,password,body,type;
  unsigned starts=0;
  int status=0;
  HTTPMethod currentMethod=HTTP_GET;String currentUri;
  FakeClient socket;
  void on(const char* path,HTTPMethod method,std::function<void()> callback){routes.push_back({path,method,callback});}
  void collectHeaders(const char* fields[],size_t count){
    collectedHeaders.clear();
    for(size_t i=0;i<count;++i)collectedHeaders.emplace_back(fields[i]);
  }
  void collectHeaders(const char* first,const char* second,const char* third){
    collectedHeaders={first,second,third};
  }
  void collectHeaders(const char* first,const char* second,const char* third,const char* fourth){collectedHeaders={first,second,third,fourth};}
  FakeClient& client(){return socket;}
  bool authenticate(const char* expectedUser,const char* expectedPassword){return user==expectedUser && password==expectedPassword;}
  void requestAuthentication(){status=401;}
  const String& header(const char* field){
    if(!strcmp(field,"Host") && !headers.count(field))headers[field]="chaosuhr.local";
    if(!strcmp(field,"Authorization") && !user.empty()){
      const std::string raw=user+":"+password;std::string encoded;const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      for(size_t i=0;i<raw.size();i+=3){
        uint32_t value=static_cast<uint8_t>(raw[i])<<16;
        if(i+1<raw.size())value|=static_cast<uint8_t>(raw[i+1])<<8;
        if(i+2<raw.size())value|=static_cast<uint8_t>(raw[i+2]);
        encoded+=alphabet[(value>>18)&63];encoded+=alphabet[(value>>12)&63];encoded+=i+1<raw.size()?alphabet[(value>>6)&63]:'=';encoded+=i+2<raw.size()?alphabet[value&63]:'=';
      }
      headers[field]="Basic "+encoded;
    }
    if(!strcmp(field,"Authorization"))return headers[field];
    for(const auto& name:collectedHeaders)if(name==field)return headers[field];
    static const String missing;return missing;
  }
  String arg(const char* field){return arguments[field];}
  bool hasArg(const char* field){return arguments.count(field);}
  unsigned args(){return arguments.size();}
  HTTPMethod method(){return currentMethod;}
  const String& uri(){return currentUri;}
  void sendHeader(const char*,const char*){}
  void send(int code,const char* mime,const String& text){status=code;type=mime;body=text;}
  void send_P(int code,const char* mime,const char* text){send(code,mime,text);}
  void begin(){++starts;}
  void dispatch(const char* path,HTTPMethod method){
    currentMethod=method;currentUri=path;
    status=404;body.clear();
    for(auto& route:routes)if(route.path==path && (route.method==method || route.method==HTTP_ANY)){route.callback();return;}
  }
};
