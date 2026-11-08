#pragma once
#include "password_crypto.h"
#include "sha256_stream.h"

// Fixed RAM only. Retired fingerprints identify expiry without keeping usable
// session/CSRF tokens. Unsigned subtraction handles millis() rollover.
class AdminSessions {
 public:
  static constexpr uint32_t IDLE_MS=30UL*60*1000;
  static constexpr size_t CAPACITY=2;
  enum class State:uint8_t {Empty,Active,Expired,Revoked};
  struct Entry {uint8_t fingerprint[32]={},csrf[16]={};uint32_t lastActivity=0;State state=State::Empty;};
  void reset(){eraseSecret(entries_,sizeof entries_);}
  void expire(uint32_t now){for(auto& e:entries_)if(e.state==State::Active && static_cast<uint32_t>(now-e.lastActivity)>=IDLE_MS)retire(e,State::Expired);}
  static void hex(const uint8_t* bytes,char text[33]){
    constexpr char digits[]="0123456789abcdef";
    for(size_t i=0;i<16;++i){text[2*i]=digits[bytes[i]>>4];text[2*i+1]=digits[bytes[i]&15];}text[32]=0;
  }
  static bool decode(const char* text,size_t length,uint8_t bytes[16]){
    if(length!=32)return false;
    for(size_t i=0;i<16;++i){unsigned value=0;for(size_t j=0;j<2;++j){const char c=text[2*i+j];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;value=(value<<4)+(c<='9'?c-'0':c-'a'+10);}bytes[i]=value;}
    return true;
  }
  static bool fingerprint(const uint8_t sid[16],uint8_t output[32]){Sha256Stream hash;hash.add(sid,16);return hash.digest(output);}
  int find(const uint8_t digest[32])const{
    for(size_t i=0;i<CAPACITY;++i)if(entries_[i].state!=State::Empty && constantEqual(digest,entries_[i].fingerprint,32))return i;
    return -1;
  }
  int available(int replacement=-1)const{
    if(replacement>=0)return replacement;
    for(size_t i=0;i<CAPACITY;++i)if(entries_[i].state!=State::Active)return i;
    return -1;
  }
  void start(int slot,const uint8_t digest[32],const uint8_t csrf[16],uint32_t now){
    eraseSecret(&entries_[slot],sizeof(Entry));auto& e=entries_[slot];
    memcpy(e.fingerprint,digest,32);memcpy(e.csrf,csrf,16);e.lastActivity=now;e.state=State::Active;
  }
  const Entry& get(int slot)const{return entries_[slot];}
  void touch(int slot,uint32_t now){entries_[slot].lastActivity=now;}
  void revoke(int slot){retire(entries_[slot],State::Revoked);}
 private:
  Entry entries_[CAPACITY]={};
  static void retire(Entry& entry,State state){eraseSecret(entry.csrf,sizeof entry.csrf);entry.lastActivity=0;entry.state=state;}
};

// A host-only HttpOnly cookie; no URL/body/header fallback for session IDs.
inline bool readAdminCookie(const char* text,size_t length,uint8_t sid[16],bool& present){
  present=false;if(length>512)return false;
  const char* end=text+length;
  while(text<end){
    while(text<end && (*text==' '||*text==';'))++text;
    const char* item=text;while(text<end && *text!=';')++text;
    const char* finish=text;while(finish>item && finish[-1]==' ')--finish;
    const char* equal=item;while(equal<finish && *equal!='=')++equal;
    if(static_cast<size_t>(equal-item)==11 && !memcmp(item,"ChaosUhrSID",11)){
      if(present || equal==finish || !AdminSessions::decode(equal+1,finish-equal-1,sid))return false;
      present=true;
    }
  }
  return true;
}
