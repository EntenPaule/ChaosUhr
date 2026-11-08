#pragma once
#include "chaos_errors.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "sha256_stream.h"
#if defined(ESP32)
#include "esp32_image.h"
static constexpr char FIRMWARE_PRODUCT[]="ChaosUhr|ESP32|4M192K|OTA2";
static constexpr uint32_t FIRMWARE_MAX_BYTES=0x1e0000;
#else

static constexpr char FIRMWARE_PRODUCT[]="ChaosUhr|ESP8266|4M1M|OTA2";
static constexpr uint32_t FIRMWARE_MAX_BYTES=1044464;
#endif

class FirmwareValidator {
 public:
  bool expectSha256(const char* hex){
    if(!hex || !*hex){checkSha_=false;return true;}
    if(strlen(hex)!=64)return fail(ErrorCode::OTA_EXPECTED_SHA);
    for(size_t i=0;i<64;++i){const char c=hex[i];const int v=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;if(v<0)return fail(ErrorCode::OTA_EXPECTED_SHA);if(!(i&1))expectedSha_[i/2]=v<<4;else expectedSha_[i/2]|=v;}
    checkSha_=true;return true;
  }
  bool feed(const uint8_t* data,size_t size){
    if(error_!=ErrorCode::None)return false;
    if(size>FIRMWARE_MAX_BYTES-count_)return fail(ErrorCode::OTA_SIZE);
    sha_.add(data,size);
    for(size_t i=0;i<size;++i){
      const uint8_t byte=data[i];
#if defined(ESP32)
      if(!image_.feed(byte))return fail(image_.errorCode());
#else
      if((count_==0 || count_==0x1000) && byte!=0xe9)return fail(ErrorCode::OTA_HEADER);
      if(count_==2 && byte!=2)return fail(ErrorCode::OTA_TARGET);
      if(count_==3 && byte!=0x40)return fail(ErrorCode::OTA_TARGET);
      if(count_>=0x1010 && count_<0x1018)metadata_[count_-0x1010]=byte;
#endif
      if(byte==static_cast<uint8_t>(FIRMWARE_PRODUCT[matched_])){
        if(++matched_==sizeof FIRMWARE_PRODUCT-1){product_=true;matched_=0;}
      }else matched_=byte==FIRMWARE_PRODUCT[0]?1:0;
#if !defined(ESP32)
      const uint8_t value=count_>=0x1010 && count_<0x1018?0:byte;
      crc_^=static_cast<uint32_t>(value)<<24;
      for(unsigned b=0;b<8;++b)crc_=(crc_<<1)^((crc_&0x80000000)?0x04c11db7:0);
#endif
      ++count_;
    }
    return true;
  }
  bool complete()const{
    if(error_!=ErrorCode::None)return false;
#if defined(ESP32)
    if(!image_.complete())return fail(image_.errorCode());
#else
    uint32_t length,crc;memcpy(&length,metadata_,4);memcpy(&crc,metadata_+4,4);
    if(count_<0x1018 || count_!=length)return fail(ErrorCode::OTA_INCOMPLETE);
    if(crc_!=crc)return fail(ErrorCode::OTA_CRC);
#endif
    if(!product_)return fail(ErrorCode::OTA_PRODUCT);
    if(checkSha_){uint8_t digest[32];if(!sha_.digest(digest))return fail(ErrorCode::SYS_CRYPTO);if(memcmp(digest,expectedSha_,32))return fail(ErrorCode::OTA_SHA);}
    return error_==ErrorCode::None;
  }
  size_t size()const{return count_;}
  ErrorCode errorCode()const{return error_;}
 private:
  mutable ErrorCode error_=ErrorCode::None;
  bool fail(ErrorCode code)const{error_=code;return false;}
  uint32_t count_=0,crc_=0xffffffff;uint8_t metadata_[8]={};size_t matched_=0;bool product_=false;
  Sha256Stream sha_;uint8_t expectedSha_[32]={};bool checkSha_=false;
#if defined(ESP32)
  Esp32ImageValidator image_;
#endif
};

template<typename Backend>class FirmwareUpload {
 public:
  explicit FirmwareUpload(Backend& backend):backend_(backend){}
  void start(bool authorized,bool correctField,const char* expectedSha=""){
    if(parts_){fail(400,ErrorCode::OTA_PARTS);return;}
    parts_=1;status_=200;message_="";validator_=FirmwareValidator{};ended_=false;
    if(!authorized){fail(403,ErrorCode::AUTH_REQUIRED);return;}
    if(!correctField){fail(400,ErrorCode::OTA_FIELD);return;}
    if(!validator_.expectSha256(expectedSha)){fail(400,ErrorCode::OTA_EXPECTED_SHA);return;}
  }
  void write(const uint8_t* data,size_t size){
    if(status_!=200 || !parts_ || !size)return;
    if(!validator_.feed(data,size)){fail(400,validator_.errorCode());return;}
    if(!running_){if(!backend_.begin()){fail(507,ErrorCode::OTA_BEGIN);return;}running_=true;}
    if(backend_.write(data,size)!=size)fail(500,ErrorCode::OTA_WRITE);
  }
  void end(){ended_=true;}
  void abort(){fail(400,ErrorCode::OTA_ABORT);reset();}
  bool finish(bool authorized,bool noExtraFields){
    if(!authorized)fail(403,ErrorCode::AUTH_REQUIRED);
    if(status_==200){
      if(!noExtraFields)fail(400,ErrorCode::OTA_EXTRA_FIELDS);
      else if(parts_!=1 || !ended_ || !running_)fail(400,ErrorCode::OTA_INCOMPLETE);
      else if(!validator_.complete())fail(400,validator_.errorCode());
    }
    bool success=false;
    if(status_==200){
      if(backend_.finish()){running_=false;success=true;}
      else fail(500,ErrorCode::OTA_FINISH);
    }
    reset();return success;
  }
  int status()const{return status_;}
  const char* error()const{return message_;}
  ErrorCode errorCode()const{return errorCode_;}
 private:
  Backend& backend_;FirmwareValidator validator_;uint8_t parts_=0;bool ended_=false,running_=false;
  int status_=200;const char* message_="";ErrorCode errorCode_=ErrorCode::None;char errorBuffer_[100]={};
  void fail(int status,ErrorCode code){if(running_){backend_.cancel();running_=false;}status_=status;errorCode_=code;copyErrorText(errorBuffer_,sizeof errorBuffer_,errorDefinition(code).message);message_=errorBuffer_;recordError(code);}
  void reset(){if(running_)backend_.cancel();running_=false;parts_=0;ended_=false;}
};
