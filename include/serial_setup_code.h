#pragma once
#include <stddef.h>
#include <stdint.h>
// Fixed-size, no echo, no dynamic allocation. Only exact SETUP <12 digits> LF.
class SerialSetupCode {
 public:
  void clear(){volatile char* p=line_;for(size_t i=0;i<sizeof line_;++i)p[i]=0;used_=0;bad_=false;}
  template<class Apply> bool feed(char c,Apply apply){
    if(c=='\r')return false;
    if(c=='\n'){
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
