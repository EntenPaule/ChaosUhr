"""Create an immutable local Wemos prerelease package; never build/publish/upload."""
from pathlib import Path
import hashlib,json,re,subprocess
from public_sources import collect_public_sources
from release_safety import ReleaseSafety
from package_test_2_0_11 import zip_bytes
from check_mqtt_stack import check as check_stack

root=Path(__file__).resolve().parents[1]
version=re.search(r'CHAOSUHR_VERSION "([^"]+)"',(root/'include/firmware_version.h').read_text())[1]
assert re.fullmatch(r'2\.0\.11-(dev|beta|rc)\.[1-9][0-9]*',version),'Numbered prerelease required'
out=root/'dist'/version
assert not out.exists(),'Immutable package already exists; choose a new version for changed firmware'
repo=root/'.pio/dev-2.0.11'
commit=subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip()
source=collect_public_sources(root,'d1_mini_public');safety=ReleaseSafety(root)
for name,data in source.items():
 safety.check(data,name)
 assert subprocess.check_output(['git','-C',str(repo),'show',commit+':'+name])==data,'Uncommitted source: '+name
build=root/'.pio/build/d1_mini_public';raw=(build/'firmware.bin').read_bytes()
assert version.encode() in raw and 4096<len(raw)<=1044464
assert raw[:1]==b'\xe9' and raw[2:4]==b'\x02\x40','Unexpected board/header'
safety.check(raw,'Wemos firmware')
deps=list(build.rglob('*.d'));assert deps and all('secrets.local.h' not in f.read_text() for f in deps)
log=(root/'.pio'/('build-'+version+'-wemos.log')).read_text()
assert 'Done cleaning' in log and '[SUCCESS]' in log and '[FAILED]' not in log
host=(root/'.pio'/('tests-'+version+'.log')).read_text()
assert 'Traceback' not in host and 'PASS actual browser MQTT requests' in host and 'PASS actual admin sessions/MQTT:' in host
assert 'PASS credentials:' in host,'Host suite did not finish'
validator=root/'.pio/host-tests/validate_wemos_prerelease'
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I',str(root/'include'),str(root/'tests/validate_firmware_file.cpp'),'-o',str(validator),'-lcrypto'],check=True)
validated=subprocess.check_output([str(validator),str(build/'firmware.bin')],text=True)
(root/'.pio'/('validate-'+version+'.log')).write_text(validated)
ram=re.findall(r'RAM:.*used (\d+) bytes from (\d+) bytes',log)[-1]
flash=re.findall(r'Flash:.*used (\d+) bytes from (\d+) bytes',log)[-1]
files={'ChaosUhr_'+version+'_Wemos_OTA.bin':raw}
for name in ('TESTANLEITUNG_'+version+'.md','BERICHT_'+version+'.md'):
 files[name]=(root/'docs'/name).read_bytes()
files['FEHLERCODES_'+version+'.md']=(root/'docs/FEHLERCODES.md').read_bytes()
files['LICENSE_'+version+'.txt']=(root/'LICENSE').read_bytes()
info=dict(version=version,source_commit=commit,board='d1_mini',chip='esp8266',binary_bytes=len(raw),ota_limit_bytes=1044464,ota_reserve_bytes=1044464-len(raw),static_ram_bytes=int(ram[0]),static_ram_limit=int(ram[1]),linker_flash_bytes=int(flash[0]),host_pass_groups=host.count('PASS '),firmware_sha256=hashlib.sha256(raw).hexdigest(),hardware_tested=False,mqtt007_hardware_root_cause_confirmed=False,reset_hardware_cause_confirmed=False,mqtt_stack_frames=check_stack(root),esp32_built=False,known_limitations=['Hardware reset cause and earlier MQTT-007 still require device confirmation; stack-copy reduction and retained reset diagnostics','Local HTTP has no TLS confidentiality'],validation=validated.strip())
files['BUILD_INFO_'+version+'.json']=(json.dumps(info,indent=2,ensure_ascii=False)+'\n').encode()
for name,data in files.items():safety.check(data,name)
checks=lambda entries:''.join(hashlib.sha256(data).hexdigest()+'  '+name+'\n' for name,data in sorted(entries.items())).encode()
checksum_name='SHA256SUMS_'+version+'.txt';files[checksum_name]=checks(files)
archive_name='ChaosUhr_'+version+'_TEST.zip';archive=zip_bytes(files)
# Create only after every check has succeeded, never overwrite an older version.
out.mkdir()
for name,data in files.items():(out/name).write_bytes(data)
(out/archive_name).write_bytes(archive)
(out/checksum_name).write_bytes(checks({k:v for k,v in files.items() if k!=checksum_name})+checks({archive_name:archive}))
print(json.dumps(info,indent=2));print('Package:',out/archive_name)
