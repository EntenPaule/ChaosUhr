#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#if defined(ARDUINO) && defined(ESP32)
#include <mbedtls/md.h>
#elif defined(ARDUINO)
#include <bearssl/bearssl.h>
#else
#include <openssl/hmac.h>
#endif

inline void eraseSecret(void* data,size_t size){
  volatile uint8_t* p=static_cast<volatile uint8_t*>(data);while(size--)*p++=0;
}
inline bool constantEqual(const void* a,const void* b,size_t size){
  const auto* x=static_cast<const uint8_t*>(a);const auto* y=static_cast<const uint8_t*>(b);
  uint8_t diff=0;for(size_t i=0;i<size;++i)diff|=x[i]^y[i];return diff==0;
}
inline bool hmac256(const void* key,size_t keySize,const void* data,size_t size,uint8_t out[32]){
#if defined(ARDUINO) && defined(ESP32)
  return mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),static_cast<const uint8_t*>(key),keySize,
                        static_cast<const uint8_t*>(data),size,out)==0;
#elif defined(ARDUINO)
  br_hmac_key_context kc;br_hmac_context ctx;
  br_hmac_key_init(&kc,&br_sha256_vtable,key,keySize);br_hmac_init(&ctx,&kc,32);
  br_hmac_update(&ctx,data,size);br_hmac_out(&ctx,out);eraseSecret(&kc,sizeof kc);eraseSecret(&ctx,sizeof ctx);return true;
#else
  unsigned length=32;return HMAC(EVP_sha256(),key,static_cast<int>(keySize),static_cast<const uint8_t*>(data),size,out,&length)!=nullptr && length==32;
#endif
}
// Standard PBKDF2-HMAC-SHA256, one 32-byte block. No allocation and bounded input.
inline bool passwordKdf(const char* password,size_t length,const uint8_t salt[16],uint32_t rounds,
                        uint8_t out[32],void(*tick)()=nullptr){
  if(!password || length>64 || !rounds || rounds>600000)return false;
  uint8_t first[20],u[32],next[32];memcpy(first,salt,16);first[16]=first[17]=first[18]=0;first[19]=1;
  if(!hmac256(password,length,first,sizeof first,u))return false;
  memcpy(out,u,32);
  for(uint32_t i=1;i<rounds;++i){
    if(!hmac256(password,length,u,32,next)){eraseSecret(u,sizeof u);eraseSecret(next,sizeof next);return false;}
    memcpy(u,next,32);
    for(size_t j=0;j<32;++j)out[j]^=u[j];
    if(tick && !(i&31))tick();
  }
  eraseSecret(u,sizeof u);eraseSecret(next,sizeof next);eraseSecret(first,sizeof first);return true;
}
// Strict bounded decoder. Only admin:<16..64 printable ASCII> is accepted.
inline bool decodeAdminBasic(const char* text,size_t size,char password[65]){
  if(!text || size<6 || size>102 || memcmp(text,"Basic ",6))return false;
  text+=6;size-=6;if(!size || size%4)return false;
  uint8_t decoded[72]={};size_t used=0;
  for(size_t i=0;i<size;i+=4){
    uint32_t value=0;unsigned pads=0;
    for(size_t j=0;j<4;++j){
      const char c=text[i+j];int digit=-1;
      if(c>='A'&&c<='Z')digit=c-'A';else if(c>='a'&&c<='z')digit=c-'a'+26;
      else if(c>='0'&&c<='9')digit=c-'0'+52;else if(c=='+')digit=62;else if(c=='/')digit=63;
      else if(c=='=' && i+4==size && j>=2){digit=0;++pads;}
      if(digit<0 || (pads && c!='=')){eraseSecret(decoded,sizeof decoded);return false;}
      value=(value<<6)|static_cast<unsigned>(digit);
    }
    if(pads>2 || (pads==2 && (value&0xffff)) || (pads==1 && (value&0xff)) || used+3-pads>sizeof decoded){eraseSecret(decoded,sizeof decoded);return false;}
    for(unsigned j=0;j<3-pads;++j)decoded[used++]=value>>(16-8*j);
  }
  bool valid=used>=22 && used<=70 && !memcmp(decoded,"admin:",6);
  for(size_t i=6;valid && i<used;++i)valid=decoded[i]>=32 && decoded[i]<=126;
  if(valid){memcpy(password,decoded+6,used-6);password[used-6]=0;}
  eraseSecret(decoded,sizeof decoded);return valid;
}
