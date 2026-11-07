#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "sha256_stream.h"

static constexpr char FIRMWARE_PRODUCT[]="ChaosUhr|ESP8266|4M1M|OTA2";
static constexpr uint32_t FIRMWARE_MAX_BYTES=1044464;

class FirmwareValidator {
 public:
  bool expectSha256(const char* hex){
    if(!hex || !*hex){checkSha_=false;return true;}
    if(strlen(hex)!=64)return false;
    for(size_t i=0;i<64;++i){const char c=hex[i];const int v=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;if(v<0)return false;if(!(i&1))expectedSha_[i/2]=v<<4;else expectedSha_[i/2]|=v;}
    checkSha_=true;return true;
  }
  bool feed(const uint8_t* data,size_t size){
    if(size>FIRMWARE_MAX_BYTES-count_)return false;
    sha_.add(data,size);
    for(size_t i=0;i<size;++i){
      const uint8_t byte=data[i];
      if((count_==0 || count_==0x1000) && byte!=0xe9)return false;
      if(count_==2 && byte!=2)return false;
      if(count_==3 && byte!=0x40)return false;
      if(count_>=0x1010 && count_<0x1018)metadata_[count_-0x1010]=byte;
      if(byte==static_cast<uint8_t>(FIRMWARE_PRODUCT[matched_])){
        if(++matched_==sizeof FIRMWARE_PRODUCT-1){product_=true;matched_=0;}
      }else matched_=byte==FIRMWARE_PRODUCT[0]?1:0;
      const uint8_t value=count_>=0x1010 && count_<0x1018?0:byte;
      crc_^=static_cast<uint32_t>(value)<<24;
      for(unsigned b=0;b<8;++b)crc_=(crc_<<1)^((crc_&0x80000000)?0x04c11db7:0);
      ++count_;
    }
    return true;
  }
  bool complete()const{
    uint32_t length,crc;memcpy(&length,metadata_,4);memcpy(&crc,metadata_+4,4);
    uint8_t digest[32];
    return count_>=0x1018 && count_==length && crc_==crc && product_ &&
      (!checkSha_ || (sha_.digest(digest) && !memcmp(digest,expectedSha_,32)));
  }
  size_t size()const{return count_;}
 private:
  uint32_t count_=0,crc_=0xffffffff;uint8_t metadata_[8]={};size_t matched_=0;bool product_=false;
  Sha256Stream sha_;uint8_t expectedSha_[32]={};bool checkSha_=false;
};

template<typename Backend>class FirmwareUpload {
 public:
  explicit FirmwareUpload(Backend& backend):backend_(backend){}
  void start(bool authorized,bool correctField,const char* expectedSha=""){
    if(parts_){fail(400,"Genau eine Firmware-Datei erforderlich.");return;}
    parts_=1;status_=200;message_="";validator_=FirmwareValidator{};ended_=false;
    if(!authorized){fail(403,"Admin-Anmeldung und Sitzungstoken erforderlich.");return;}
    if(!correctField){fail(400,"Nur eine unkomprimierte Wemos-Firmware im Feld firmware erlaubt.");return;}
    if(!validator_.expectSha256(expectedSha)){fail(400,"Ungueltige SHA-256-Pruefsumme.");return;}
  }
  void write(const uint8_t* data,size_t size){
    if(status_!=200 || !parts_ || !size)return;
    if(!validator_.feed(data,size)){fail(400,"Ungueltiges Image, falsches Board oder Firmware zu gross.");return;}
    if(!running_){if(!backend_.begin()){fail(507,"Nicht genug OTA-Speicher oder Flash nicht bereit.");return;}running_=true;}
    if(backend_.write(data,size)!=size)fail(500,"Flash-Schreibfehler. Firmware wurde nicht aktiviert.");
  }
  void end(){ended_=true;}
  void abort(){fail(400,"Upload abgebrochen. Firmware wurde nicht aktiviert.");reset();}
  bool finish(bool authorized,bool noExtraFields){
    if(!authorized)fail(403,"Admin-Anmeldung und Sitzungstoken erforderlich.");
    if(status_==200 && (parts_!=1 || !ended_ || !running_ || !noExtraFields || !validator_.complete()))
      fail(400,"Firmware unvollstaendig, beschaedigt oder nicht fuer ChaosUhr ab 2.0.8 geeignet.");
    bool success=false;
    if(status_==200){
      if(backend_.finish()){running_=false;success=true;}
      else fail(500,"Firmware-Pruefung oder Abschluss fehlgeschlagen.");
    }
    reset();return success;
  }
  int status()const{return status_;}
  const char* error()const{return message_;}
 private:
  Backend& backend_;FirmwareValidator validator_;uint8_t parts_=0;bool ended_=false,running_=false;
  int status_=200;const char* message_="";
  void fail(int status,const char* message){if(running_){backend_.cancel();running_=false;}status_=status;message_=message;}
  void reset(){if(running_)backend_.cancel();running_=false;parts_=0;ended_=false;}
};
