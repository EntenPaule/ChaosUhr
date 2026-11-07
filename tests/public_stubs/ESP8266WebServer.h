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
  FakeClient socket;
  void on(const char* path,HTTPMethod method,std::function<void()> callback){routes.push_back({path,method,callback});}
  void collectHeaders(const char* fields[],size_t count){
    collectedHeaders.clear();
    for(size_t i=0;i<count;++i)collectedHeaders.emplace_back(fields[i]);
  }
  void collectHeaders(const char* first,const char* second,const char* third){
    collectedHeaders={first,second,third};
  }
  FakeClient& client(){return socket;}
  bool authenticate(const char* expectedUser,const char* expectedPassword){return user==expectedUser && password==expectedPassword;}
  void requestAuthentication(){status=401;}
  const String& header(const char* field){
    for(const auto& name:collectedHeaders)if(name==field)return headers[field];
    static const String missing;return missing;
  }
  String arg(const char* field){return arguments[field];}
  bool hasArg(const char* field){return arguments.count(field);}
  unsigned args(){return arguments.size();}
  void sendHeader(const char*,const char*){}
  void send(int code,const char* mime,const String& text){status=code;type=mime;body=text;}
  void send_P(int code,const char* mime,const char* text){send(code,mime,text);}
  void begin(){++starts;}
  void dispatch(const char* path,HTTPMethod method){
    status=404;body.clear();
    for(auto& route:routes)if(route.path==path && (route.method==method || route.method==HTTP_ANY)){route.callback();return;}
  }
};
