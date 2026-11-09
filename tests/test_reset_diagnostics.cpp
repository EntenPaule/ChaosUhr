#include "reset_diagnostics.h"
#include <cassert>
#include <cstdio>
static MqttResetTrace rtc;
static unsigned writes=0;
static void sample(MqttResetTrace& t){t.uptime=1234;t.heap=20000;t.stackFree=2048;}
static bool persist(MqttResetTrace& t){rtc=t;++writes;return true;}
int main(){
 auto& d=resetDiagnostics();d.sample=sample;d.persist=persist;d.available=true;
 d.begin(0,{},false);assert(!d.previousValid&&validResetTrace(rtc));
 {MqttSaveAttempt attempt;mqttCheckpoint(MqttSavePhase::Write);assert(rtc.phase==static_cast<unsigned>(MqttSavePhase::Write));}
 assert(rtc.phase==static_cast<unsigned>(MqttSavePhase::Rejected));
 {MqttSaveAttempt attempt;attempt.complete();}assert(rtc.phase==static_cast<unsigned>(MqttSavePhase::Complete));
 mqttCheckpoint(MqttSavePhase::Verify);const auto interrupted=rtc;
 d.begin(3,interrupted,true);assert(d.previousValid&&d.previous.phase==static_cast<unsigned>(MqttSavePhase::Verify));
 assert(d.current.phase==0&&d.previous.heap==20000&&d.previous.stackFree==2048);
 for(size_t at=0;at<sizeof interrupted;++at){auto bad=interrupted;reinterpret_cast<uint8_t*>(&bad)[at]^=1;d.begin(2,bad,true);assert(!d.previousValid);}
 d.begin(0,interrupted,true);assert(!d.previousValid); // power-on cannot prove RTC continuity
 d.begin(4,interrupted,false);assert(!d.previousValid);
 d.begin(4,interrupted,true);assert(d.previousValid);
 assert(resetReasonName(2)[0]=='e'&&writes>0);
 puts("PASS reset diagnostics: checksummed numeric RTC trace, corrupted/power-on/unreadable retention discarded, interrupted phase, normal rejection/completion and stable previous-boot snapshot");
}
