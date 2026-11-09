// ESP8266 RTC tracing has no backend on ESP32. Check the actual target guard.
#include "reset_diagnostics.h"
#include <cassert>
#include <cstdio>
int main(){
 auto& d=resetDiagnostics();assert(!d.available);const auto before=d.current;
 {MqttSaveAttempt attempt;mqttCheckpoint(MqttSavePhase::Write);attempt.complete();}
 assert(d.current.phase==before.phase&&d.current.magic==before.magic);
 puts("PASS ESP32 Arduino guard: unavailable ESP8266 RTC tracing remains disabled");
}
