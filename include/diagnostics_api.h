#pragma once
#include "reset_diagnostics.h"
// Included after authenticated HTTP helpers and device state.
static void diagnosticsApi(){
  if(!auth())return;
  JsonDocument document;
  document["version"]=VERSION;document["platform"]=DEVICE_FAMILY;
  document["uptimeMs"]=millis();document["heap"]=ESP.getFreeHeap();
  const auto& reset=resetDiagnostics();auto reboot=document["reset"].to<JsonObject>();
  reboot["available"]=reset.available;
  if(reset.available){
    reboot["reasonCode"]=reset.reason;reboot["reason"]=resetReasonName(reset.reason);
    reboot["crashCallbackValid"]=reset.crashValid;
    if(reset.crashValid){reboot["crashReasonCode"]=reset.crashReason;reboot["crashReason"]=resetReasonName(reset.crashReason);}
    reboot["exceptionCause"]=reset.exceptionCause;reboot["epc1"]=reset.epc1;
    reboot["rtcWriteOk"]=reset.rtcWriteOk;reboot["previousMqttValid"]=reset.previousValid;
    auto trace=[&](const char* key,const MqttResetTrace& t){auto item=reboot[key].to<JsonObject>();
      item["phase"]=mqttPhaseName(t.phase);item["uptimeMs"]=t.uptime;
      item["heapBytes"]=t.heap;item["heapMaxBlockBytes"]=t.heapBlock;item["stackHighWaterFreeBytes"]=t.stackFree;
    };
    if(reset.previousValid)trace("previousMqtt",reset.previous);
    trace("currentMqtt",reset.current);
  }
  const auto saved=savedWifiState();document["wifiCredentials"]=saved==SavedWifiState::Available?"available":saved==SavedWifiState::Missing?"missing":"unreadable";
  document["wifiConnected"]=WiFi.status()==WL_CONNECTED;
  document["mqttEnabled"]=mqttConfig.enabled!=0;document["mqttConnected"]=clockMqtt.connected();
  document["lastError"]=errorText(errorDefinition(errorRegistry().last()).code);
  auto history=document["errors"].to<JsonArray>();
  for(size_t i=0;i<errorRegistry().size();++i){
    const auto& event=errorRegistry().at(i);const auto definition=errorDefinition(event.code);
    auto entry=history.add<JsonObject>();
    entry["code"]=errorText(definition.code);entry["component"]=errorText(definition.component);
    entry["message"]=errorText(definition.message);entry["details"]=errorText(definition.details);
    entry["action"]=errorText(definition.action);entry["uptimeMs"]=event.uptimeMs;entry["occurrences"]=event.occurrences;
  }
  String output;serializeJson(document,output);server.send(200,"application/json",output);
}
