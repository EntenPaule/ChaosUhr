#pragma once
#include "Arduino.h"
#include <array>
#include <cassert>
struct FakeEEPROM {
  std::array<uint8_t,128> buffer,flash;
  bool fail=false;
  unsigned commits=0;
  FakeEEPROM(){buffer.fill(0xff);flash=buffer;}
  size_t length(){return buffer.size();}
  void begin(size_t){}
  uint8_t read(size_t at){return buffer.at(at);}
  bool durable(){return false;}
  template<typename T>void get(size_t offset,T& value){assert(offset+sizeof value<=length());memcpy(&value,buffer.data()+offset,sizeof value);}
  template<typename T>void put(size_t offset,const T& value){assert(offset+sizeof value<=length());memcpy(buffer.data()+offset,&value,sizeof value);}
  void write(size_t offset,uint8_t value){buffer.at(offset)=value;}
  bool commit(){++commits;if(fail)return false;flash=buffer;return true;}
  void reboot(){buffer=flash;}
};
extern FakeEEPROM EEPROM;
