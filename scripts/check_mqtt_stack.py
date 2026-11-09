"""Check actual Xtensa -fstack-usage output, not host-stack estimates."""
from pathlib import Path
import json,re

def check(root):
    source=root/'.pio/build/d1_mini_public/src/main.cpp.su'
    frames={}
    for line in source.read_text().splitlines():
        if 'mqttSaveApi()' in line:key='mqtt_http_handler'
        elif 'MqttConfigStorage' in line and '::save(' in line:key='mqtt_storage_save'
        else:continue
        pieces=line.split('\t');assert pieces[-1]=='static', 'Unbounded MQTT stack frame'
        frames[key]=int(pieces[-2])
    assert set(frames)=={'mqtt_http_handler','mqtt_storage_save'}
    assert frames['mqtt_http_handler']<=256 and frames['mqtt_storage_save']<=512, 'MQTT stack regression'
    frames['combined_bytes_before_callees']=sum(frames.values())
    return frames
if __name__=='__main__':print(json.dumps(check(Path(__file__).resolve().parents[1]),indent=2))
