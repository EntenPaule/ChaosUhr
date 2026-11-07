#pragma once
#include <stddef.h>
#include <string.h>

inline bool validHostSyntax(const char* host){
  if(!host || !*host || strlen(host)>127)return false;
  for(const char* p=host;*p;++p)
    if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||strchr(".-_:",*p)))return false;
  return true;
}
inline bool originMatchesHost(const char* origin,const char* host){
  if(!validHostSyntax(host))return false;
  if(!origin || !*origin)return true; // Token is still mandatory for mutations.
  return !strncmp(origin,"http://",7) && !strcmp(origin+7,host);
}
inline bool setupHostAllowed(const char* host,const char* apAddress){
  return host && ((!strcmp(host,"chaosuhr.local") || !strcmp(host,"chaosuhr")) || !strcmp(host,apAddress));
}
