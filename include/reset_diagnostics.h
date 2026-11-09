#pragma once
#include <stdint.h>
#include <stddef.h>
// Numeric, bounded RTC breadcrumb only. No credentials, request bodies or flash writes.
enum class MqttSavePhase:uint32_t {Idle,Entered,Authenticated,Candidate,Validated,Mount,Open,Write,Flush,Verify, Rename,Committed,Reconfigure,Response,Complete,Rejected};
struct MqttResetTrace {
  uint32_t magic=0,format=1,phase=0,uptime=0,heap=0,stackFree=0,heapBlock=0,checksum=0;
};
static_assert(sizeof(MqttResetTrace)==32,"RTC wire layout");
inline uint32_t resetTraceChecksum(const MqttResetTrace& t){
  uint32_t hash=2166136261u;const auto* p=reinterpret_cast<const uint8_t*>(&t);
  for(size_t i=0;i<offsetof(MqttResetTrace,checksum);++i)hash=(hash^p[i])*16777619u;
  return hash;
}
inline bool validResetTrace(const MqttResetTrace& t){return t.magic==0x43525432 && t.format==1 && t.phase<=static_cast<uint32_t>(MqttSavePhase::Rejected) && t.checksum==resetTraceChecksum(t);}
inline const char* mqttPhaseName(uint32_t phase){
  switch(static_cast<MqttSavePhase>(phase)){
#define PHASE(x) case MqttSavePhase::x:return #x
    PHASE(Idle);PHASE(Entered);PHASE(Authenticated);PHASE(Candidate);PHASE(Validated);PHASE(Mount);PHASE(Open);PHASE(Write);PHASE(Flush);PHASE(Verify);PHASE(Rename);PHASE(Committed);PHASE(Reconfigure);PHASE(Response);PHASE(Complete);PHASE(Rejected);
#undef PHASE
  }return "unknown";
}
inline const char* resetReasonName(uint32_t reason){
  switch(reason){case 0:return "power-on-or-unclassified";case 1:return "hardware-watchdog";case 2:return "exception";case 3:return "software-watchdog";case 4:return "software-restart";case 5:return "deep-sleep";case 6:return "external-reset";case 253:return "stack-guard";case 254:return "software-exception";default:return "unknown";}
}
struct ResetDiagnostics {
  using Sample=void(*)(MqttResetTrace&);using Persist=bool(*)(MqttResetTrace&);
  bool available=false,previousValid=false,rtcWriteOk=false;
  uint32_t reason=0,exceptionCause=0,epc1=0,crashReason=0;
  bool crashValid=false;
  MqttResetTrace previous{},current{};Sample sample=nullptr;Persist persist=nullptr;
  void mark(MqttSavePhase phase){
    current.magic=0x43525432;current.phase=static_cast<uint32_t>(phase);
    if(sample)sample(current);
    current.checksum=resetTraceChecksum(current);
    if(persist)rtcWriteOk=persist(current);
  }
  void begin(uint32_t why,const MqttResetTrace& retained,bool readOk){
    reason=why;previousValid=why!=0&&readOk&&validResetTrace(retained);
    previous=previousValid?retained:MqttResetTrace{};current=MqttResetTrace{};
    mark(MqttSavePhase::Idle);
  }
};
inline ResetDiagnostics& resetDiagnostics(){static ResetDiagnostics state;return state;}
inline void mqttCheckpoint(MqttSavePhase phase){resetDiagnostics().mark(phase);}
// A normal early return is distinguished from interruption inside a phase.
struct MqttSaveAttempt {
  bool completed=false;
  MqttSaveAttempt(){mqttCheckpoint(MqttSavePhase::Entered);}
  ~MqttSaveAttempt(){if(!completed)mqttCheckpoint(MqttSavePhase::Rejected);}
  void complete(){completed=true;mqttCheckpoint(MqttSavePhase::Complete);}
};
void initializeResetDiagnostics();
