#pragma once
#include "sha256_stream.h"
#include <string.h>
#include "chaos_errors.h"

// Streaming validation of an unsigned classic ESP32 application, including its
// segment checksum and appended SHA-256. No whole-image allocation.
class Esp32ImageValidator {
 public:
  bool feed(uint8_t byte){
    if(error_!=ErrorCode::None)return false;
    if(phase_==Done)return fail(ErrorCode::OTA_TRAILING);
    if(phase_!=Hash)hashByte(byte);
    if(phase_==Header){
      header_[position_++]=byte;
      if(position_==24){
        if(header_[0]!=0xe9 || header_[1]<1 || header_[1]>16 || header_[23]!=1)return fail(ErrorCode::OTA_HEADER);
        if(header_[2]!=2 || (header_[3]&0xf0)!=0x20 || header_[12] || header_[13])return fail(ErrorCode::OTA_TARGET);
        segments_=header_[1];position_=0;phase_=Segment;
      }
    }else if(phase_==Segment){
      segment_[position_++]=byte;
      if(position_==8){
        memcpy(&remaining_,segment_+4,4);if(remaining_>0x1e0000)return fail(ErrorCode::OTA_SIZE);
        position_=0;
        if(remaining_)phase_=Data;
        else phase_=--segments_?Segment:Padding;
      }
    }else if(phase_==Data){
      checksum_^=byte;
      if(!--remaining_)phase_=--segments_?Segment:Padding;
    }else if(phase_==Padding){
      if((count_&15)==15){
        if(byte!=checksum_)return fail(ErrorCode::OTA_CRC);
        flushHash();if(!imageHash_.digest(expected_))return fail(ErrorCode::SYS_CRYPTO);
        phase_=Hash;position_=0;
      }else if(byte)return fail(ErrorCode::OTA_PADDING);
    }else if(phase_==Hash){
      if(byte!=expected_[position_++])return fail(ErrorCode::OTA_SHA);
      if(position_==32)phase_=Done;
    }
    ++count_;return true;
  }
  bool complete()const{return phase_==Done&&error_==ErrorCode::None;}
  ErrorCode errorCode()const{return error_==ErrorCode::None&&!complete()?ErrorCode::OTA_INCOMPLETE:error_;}
 private:
  ErrorCode error_=ErrorCode::None;
  bool fail(ErrorCode code){error_=code;return false;}
  enum Phase {Header,Segment,Data,Padding,Hash,Done};Phase phase_=Header;
  uint8_t header_[24]={},segment_[8]={},expected_[32]={},block_[64]={},checksum_=0xef,segments_=0;
  size_t position_=0,used_=0;uint32_t count_=0,remaining_=0;Sha256Stream imageHash_;
  void hashByte(uint8_t value){block_[used_++]=value;if(used_==sizeof block_)flushHash();}
  void flushHash(){if(used_)imageHash_.add(block_,used_);used_=0;}
};
