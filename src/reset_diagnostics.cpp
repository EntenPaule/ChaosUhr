#include "reset_diagnostics.h"
#if defined(ESP8266) && defined(ARDUINO)
#include <Arduino.h>
#include <Esp.h>
extern "C" {
#include <user_interface.h>
}
// Offset is in 32-bit words. First 128 bytes belong to eboot OTA commands.
// Our 32 bytes are at user RTC byte 128..159, outside the SDK reset block.
static constexpr uint32_t TRACE_RTC_WORD=32;
static void sampleTrace(MqttResetTrace& t){t.uptime=millis();t.heap=ESP.getFreeHeap();t.stackFree=ESP.getFreeContStack();
#if defined(UMM_INFO)
  t.heapBlock=ESP.getMaxFreeBlockSize();
#endif
}
static bool persistTrace(MqttResetTrace& t){return ESP.rtcUserMemoryWrite(TRACE_RTC_WORD,reinterpret_cast<uint32_t*>(&t),sizeof t);}
// A crash-only record at RTC byte 160..191 preserves the core callback reason
// (including stack-guard 253) even if the subsequent SDK reason says restart.
static constexpr uint32_t CRASH_RTC_WORD=40;
extern "C" void custom_crash_callback(struct rst_info* info,uint32_t,uint32_t){
  MqttResetTrace crash{};crash.magic=0x43525432;crash.phase=0;
  crash.uptime=info->reason;crash.heap=info->exccause;crash.stackFree=info->epc1;
  crash.checksum=resetTraceChecksum(crash);
  system_rtc_mem_write(64+CRASH_RTC_WORD,&crash,sizeof crash);
}
void initializeResetDiagnostics(){
  auto& d=resetDiagnostics();MqttResetTrace retained{};
  const bool read=ESP.rtcUserMemoryRead(TRACE_RTC_WORD,reinterpret_cast<uint32_t*>(&retained),sizeof retained);
  const auto* info=ESP.getResetInfoPtr();
  d.available=true;d.sample=sampleTrace;d.persist=persistTrace;
  d.begin(info?info->reason:0,retained,read);
  if(info && info->reason==2){d.exceptionCause=info->exccause;d.epc1=info->epc1;}
  MqttResetTrace crash{};
  const bool crashRead=ESP.rtcUserMemoryRead(CRASH_RTC_WORD,reinterpret_cast<uint32_t*>(&crash),sizeof crash);
  d.crashValid=d.reason!=0 && crashRead && validResetTrace(crash);
  if(d.crashValid){d.crashReason=crash.uptime;d.exceptionCause=crash.heap;d.epc1=crash.stackFree;}
  // Clear the crash marker once per boot; never clear SDK/eboot RTC areas.
  uint32_t zero=0;ESP.rtcUserMemoryWrite(CRASH_RTC_WORD,&zero,sizeof zero);
  Serial.printf("[RESET] reason=%lu (%s) exception=%lu epc1=0x%08lx mqttPrevious=%s retained=%u crash=%s\n",
    static_cast<unsigned long>(d.reason),resetReasonName(d.reason),static_cast<unsigned long>(d.exceptionCause),
    static_cast<unsigned long>(d.epc1),d.previousValid?mqttPhaseName(d.previous.phase):"unavailable",d.previousValid?1:0,d.crashValid?resetReasonName(d.crashReason):"unavailable");
}
#else
void initializeResetDiagnostics(){}
#endif
