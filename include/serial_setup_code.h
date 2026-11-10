#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
inline bool serialSetupApMatches(const uint8_t* ssid,const uint8_t* password,bool wpa2,const char* expectedSsid,const char* code){
  const size_t length=strlen(expectedSsid);
  return wpa2 && length<32 && memcmp(ssid,expectedSsid,length+1)==0 && memcmp(password,code,13)==0;
}
// Fixed-size, no echo, no dynamic allocation. Only exact SETUP <12 digits> LF.
class SerialSetupCode {
 public:
  void clear(){volatile char* p=line_;for(size_t i=0;i<sizeof line_;++i)p[i]=0;used_=0;bad_=false;}
  template<class Apply> bool feed(char c,Apply apply){return feed(c,apply,[](){});}
  template<class Apply,class Status> bool feed(char c,Apply apply,Status status){
    if(c=='\r')return false;
    if(c=='\n'){
      if(!bad_ && used_==6 && line_[0]=='S' && line_[1]=='T' && line_[2]=='A' && line_[3]=='T' && line_[4]=='U' && line_[5]=='S'){status();clear();return false;}
      bool ok=!bad_ && used_==18;
      const char prefix[]="SETUP ";for(size_t i=0;ok && i<6;++i)ok=line_[i]==prefix[i];
      for(size_t i=6;ok && i<18;++i)ok=line_[i]>='0' && line_[i]<='9';
      if(ok)ok=apply(line_+6);
      clear();return ok;
    }
    if(used_<18 && c>=32 && c<=126)line_[used_++]=c;else bad_=true;
    return false;
  }
 private: char line_[19]={};size_t used_=0;bool bad_=false;
};
