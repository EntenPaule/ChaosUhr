#pragma once
#include <stddef.h>
#include <string.h>

struct HttpAuthority {const char* host;size_t length;unsigned port;};
inline unsigned char httpLower(unsigned char c){return c>='A'&&c<='Z'?c+('a'-'A'):c;}
inline bool parseHttpAuthority(const char* text,HttpAuthority& result){
  if(!text || !*text || strlen(text)>127)return false;
  size_t length=0;
  while(text[length] && text[length]!=':'){
    const unsigned char c=httpLower(text[length]);
    if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='-'||c=='_'))return false;
    ++length;
  }
  if(!length)return false;
  unsigned port=80;
  if(text[length]){
    port=0;const char* p=text+length+1;if(!*p)return false;
    for(;*p;++p){if(*p<'0'||*p>'9'||port>6553)return false;port=port*10+(*p-'0');if(port>65535)return false;}
    if(!port)return false;
  }
  result={text,length,port};return true;
}
inline bool validHostSyntax(const char* host){HttpAuthority value;return parseHttpAuthority(host,value);}
inline bool sameHttpAuthority(const char* first,const char* second){
  HttpAuthority a,b;if(!parseHttpAuthority(first,a)||!parseHttpAuthority(second,b)||a.port!=b.port||a.length!=b.length)return false;
  for(size_t i=0;i<a.length;++i)if(httpLower(a.host[i])!=httpLower(b.host[i]))return false;
  return true;
}
inline bool originMatchesHost(const char* origin,const char* host){
  if(!validHostSyntax(host))return false;
  if(!origin || !*origin)return true; // Token is still mandatory for mutations.
  // RFC 6454: host comparison is case insensitive; omitted HTTP port means 80.
  // Still reject null/multiple origins, userinfo, paths, foreign ports and TLS.
  return !strncmp(origin,"http://",7) && sameHttpAuthority(origin+7,host);
}
inline bool setupHostAllowed(const char* host,const char* apAddress){
  return host && (sameHttpAuthority(host,"chaosuhr.local") || sameHttpAuthority(host,"chaosuhr") || sameHttpAuthority(host,apAddress));
}
