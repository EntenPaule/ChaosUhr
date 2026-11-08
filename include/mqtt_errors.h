#pragma once
#include "chaos_errors.h"
enum class MqttTransportCause:uint8_t {None,Dns,Tcp,Timeout,Other,AckTimeout};
inline ErrorCode mqttDisconnectCode(uint8_t reason,MqttTransportCause transport,bool established){
  // The pinned library may retain USER_OK from an earlier intentional close.
  // A concrete new transport failure must not be hidden by that old reason.
  if(reason==0&&transport==MqttTransportCause::None)return ErrorCode::None;
  if(established)return ErrorCode::MQTT_LINK_LOST;
  if(transport==MqttTransportCause::Dns)return ErrorCode::MQTT_DNS;
  if(transport==MqttTransportCause::Timeout)return ErrorCode::MQTT_TIMEOUT;
  if(transport==MqttTransportCause::AckTimeout)return ErrorCode::MQTT_ACK_TIMEOUT;
  if(transport==MqttTransportCause::Tcp)return ErrorCode::MQTT_TCP;
  switch(reason){
    case 1:return ErrorCode::MQTT_PROTOCOL;
    case 2:return ErrorCode::MQTT_CLIENT_ID;
    case 3:return ErrorCode::MQTT_REJECTED;
    case 4:return ErrorCode::MQTT_LOGIN;
    case 5:return ErrorCode::MQTT_PERMISSION;
    case 6:return ErrorCode::MQTT_TLS;
    default:return ErrorCode::MQTT_CONNECT_UNKNOWN;
  }
}
