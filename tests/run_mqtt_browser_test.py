"""Real GUI URLSearchParams -> vendored ESP8266 parser -> actual MQTT API/storage.
All request values are synthetic. Never read device credentials or contact a broker.
"""
from pathlib import Path
import json,re,subprocess
root=Path(__file__).resolve().parents[1]
requests=json.loads(subprocess.check_output(['node',str(root/'tests/mqtt_browser_requests.js')]))
fixture=(root/'tests/test_http_parser.cpp.in').read_text()
parser=re.sub(r'^#include[^\n]*\n','',(root/'lib/ChaosUhrWebServer/src/Parsing-impl.h').read_text(),flags=re.M)
fixture=fixture.replace('@PARSER@',parser)
bridge=r'''
#include <ArduinoJson.h>
#include "settings.h"
#include "telemetry.h"
#include "memory_animation_fs.h"
static MqttConfig mqttConfig=defaultMqttConfig();static Telemetry telemetry;
static MemoryAnimationFS filesystem;
static MqttConfigStorage<MemoryAnimationFS,AnimationFile> mqttStorage(filesystem);
static struct {bool ready(){return true;}} animations;
static struct {unsigned changes=0;const char* base(){return "fixture";}bool connected(){return false;}uint8_t lastReason(){return 0;}ErrorCode errorCode(){return ErrorCode::None;}void reconfigure(){++changes;}} clockMqtt;
static bool auth(){return true;}
static struct {bool validMutationToken(){return true;}} publicAccess;
static struct {
 Server* parsed=nullptr;int status=0;std::string response;
 int args(){return parsed->_currentArgCount;}
 std::string argName(int i){return parsed->_currentArgs[i].key;}
 std::string arg(int i){return parsed->_currentArgs[i].value;}
 void send(int code,const char*,const std::string& body){status=code;response=body;}
} server;
// Adapt only the Arduino String API boundary; compile the actual MQTT handlers.
#define String std::string
#include "mqtt_api.h"
#undef String
'''
checks=['filesystem.begin();filesystem.files["/fixture.chaos"]=std::make_shared<std::vector<uint8_t>>(8,42);']
for i,r in enumerate(requests):
 checks.append('{Server parsed;assert(parse(parsed,String('+json.dumps(r['http'])+'))==Server::CLIENT_REQUEST_CAN_CONTINUE);server.parsed=&parsed;assert(server.args()==24);')
 for k,v in r['fields']:
  checks.append('assert(parsed.arg('+json.dumps(k)+')=='+json.dumps(v)+');')
 if r['prefix']:
  checks.append('mqttSaveApi();assert(server.status==200);assert(mqttConfig.enabled);')
  checks.append('MqttConfig loaded=defaultMqttConfig();MqttConfigStorage<MemoryAnimationFS,AnimationFile> afterRestart(filesystem);assert(afterRestart.load(loaded));assert(!memcmp(&loaded,&mqttConfig,sizeof loaded));mqttConfigApi();assert(server.status==200);assert(filesystem.formats==0);}')
 else:
  checks.append('const auto previous=mqttConfig;const auto writes=filesystem.writeCalls;mqttSaveApi();assert(server.status==400);assert(server.response.find("MQTT-022")!=std::string::npos);assert(server.response.find("discoveryPrefix")!=std::string::npos);assert(!memcmp(&previous,&mqttConfig,sizeof previous));assert(writes==filesystem.writeCalls);}')
checks.append('assert(filesystem.files.count("/fixture.chaos")&&*filesystem.files.at("/fixture.chaos")==std::vector<uint8_t>(8,42));')
fixture=fixture.replace('int main(){',bridge+'\nint main(){\n'+'\n'.join(checks))
headers=next((root/'.pio/libdeps').glob('*/ArduinoJson/src/ArduinoJson.h')).parent
binary=root/'.pio/host-tests/test_mqtt_browser';binary.parent.mkdir(parents=True,exist_ok=True)
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-pedantic','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',str(root/'include'),'-I',str(root/'tests'),'-I',str(headers),'-x','c++','-','-o',str(binary)],input=fixture,text=True,check=True)
subprocess.run([str(binary)],check=True)
print('PASS actual browser MQTT requests: 24 fields, hostname/IP, discovery off, save/reload and unchanged storage format')
