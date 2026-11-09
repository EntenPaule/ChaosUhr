"""Prepare the shared beta.3 release offline; preserve the distributed Wemos BIN.
Uses actual PlatformIO metadata and the pinned core partition parser. No upload.
"""
import hashlib,importlib.util,json,re,subprocess
from pathlib import Path
from release_safety import ReleaseSafety
from public_sources import collect_public_sources

VERSION='2.0.11-beta.3';PREFIX='ChaosUhr_'+VERSION
WEMOS_SHA='d5f5c6c8026c66125cc9c651b6109fdae787611574e5d5c9313e55e9d5bed6c2'

def sha(data):return hashlib.sha256(data).hexdigest()
def require(ok,message):
 if not ok:raise SystemExit(message)

def main():
 root=Path(__file__).resolve().parents[1];core=Path.home()/'.platformio';repo=root/'.pio/dev-2.0.11'
 output=root/'dist'/VERSION/'release';require(not output.exists(),'Release staging already exists; do not overwrite')
 require(re.search(r'CHAOSUHR_VERSION "'+re.escape(VERSION)+'"',(root/'include/firmware_version.h').read_text()),'Wrong version')
 safety=ReleaseSafety(root);source=collect_public_sources(root,'d1_mini_public')
 firmware_commit=subprocess.check_output(['git','-C',str(repo),'log','-1','--format=%H','--','src','include','lib','platformio.ini','partitions'],text=True).strip()
 for name,data in source.items():
  safety.check(data,name)
  if name.startswith(('src/','include/','lib/','partitions/')) or name=='platformio.ini':
   require(subprocess.check_output(['git','-C',str(repo),'show',firmware_commit+':'+name])==data,'Runtime source differs from commit: '+name)
 log_path=root/'.pio/validate-beta3-shared-release.log'
 with log_path.open('w') as log:
  def run(command):
   result=subprocess.run([str(x) for x in command],capture_output=True,text=True,check=True)
   log.write(result.stdout+result.stderr);log.flush();return result.stdout
  build=root/'.pio/build/esp32dev_public';metadata=json.loads((root/'.pio/metadata-2.0.11-beta.3-esp32.json').read_text())['esp32dev_public']
  require(Path(metadata['prog_path']).resolve()==(build/'firmware.elf').resolve(),'Foreign build metadata')
  components=[(int(item['offset'],0),Path(item['path'])) for item in metadata['extra']['flash_images']]
  require(len(components)==3 and {p.name for _,p in components}=={'bootloader.bin','partitions.bin','boot_app0.bin'},'Unexpected components')
  app_offset=int(metadata['extra']['application_offset'],0);components.append((app_offset,build/'firmware.bin'));components.sort()
  spec=importlib.util.spec_from_file_location('release_partitions',core/'packages/framework-arduinoespressif32/tools/gen_esp32part.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
  partition_bytes=(build/'partitions.bin').read_bytes();table=module.PartitionTable.from_binary(partition_bytes)
  csv=root/'partitions/esp32_4mb_ota.csv';declared=module.PartitionTable.from_csv(csv.read_text())
  table.verify();declared.verify();table.verify_size_fits(0x400000)
  require(table.to_binary()==partition_bytes==declared.to_binary(),'Partition CSV/binary/MD5 mismatch')
  slot0=next(p for p in table if p.type==0 and p.subtype==0x10);slot1=next(p for p in table if p.type==0 and p.subtype==0x11)
  require(slot0.offset==app_offset and slot0.size==slot1.size==0x1e0000,'Unexpected OTA slots')
  otadata=next(p for p in table if p.type==1 and p.subtype==0)
  require(next(at for at,p in components if p.name=='boot_app0.bin')==otadata.offset,'boot_app0/OTA data mismatch')
  broken=bytearray(partition_bytes);broken[36]^=1
  try:module.PartitionTable.from_binary(bytes(broken))
  except module.InputError:pass
  else:raise SystemExit('Corrupt partition-table MD5 accepted')
  app=(build/'firmware.bin').read_bytes();wemos=(root/'dist'/VERSION/(PREFIX+'_Wemos_OTA.bin')).read_bytes()
  require(sha(wemos)==WEMOS_SHA,'Distributed Wemos firmware changed')
  require(VERSION.encode() in app and len(app)<=slot0.size,'ESP32 version/OTA size invalid')
  require((len(app)+4095)//4096*4096<=slot0.size,'ESP32 sector-rounded OTA image exceeds slot')
  dependencies=list(build.rglob('*.d'));require(dependencies and all('secrets.local.h' not in p.read_text(errors='replace') for p in dependencies),'Missing/private build dependencies')
  build_log=(root/'.pio/build-2.0.11-beta.3-esp32.log').read_text()
  require('[SUCCESS]' in build_log and 'Done cleaning' in build_log and '[FAILED]' not in build_log,'No successful fresh ESP32 build')
  wemos_log=(root/'.pio/build-2.0.11-beta.3-wemos.log').read_text();require('[SUCCESS]' in wemos_log and 'Done cleaning' in wemos_log,'Missing original Wemos build evidence')
  board=json.loads((core/'platforms/espressif32/boards/esp32dev.json').read_text())
  require(board['build']['flash_mode']=='dio' and board['build']['f_flash']=='40000000L' and board['upload']['flash_size']=='4MB','Unreviewed board flash parameters')
  esptool=core/'packages/tool-esptool/esptool'
  for path in [build/'bootloader.bin',build/'firmware.bin']:
   info=run([esptool,'--chip','esp32','image-info',path]);require(re.search(r'Validation hash: [0-9a-f]{64} \(valid\)',info,re.I),'Image hash not validated')
  factory_path=root/'.pio/esp32-beta3-factory-verified.bin'
  cmd=[esptool,'--chip','esp32','merge-bin','--output',factory_path,'--flash-mode','dio','--flash-freq','40m','--flash-size','4MB','--pad-to-size','4MB']
  for at,path in components:cmd.extend([hex(at),path])
  run(cmd);factory=factory_path.read_bytes();expected=bytearray(b'\xff'*0x400000);end=0;component_info=[]
  for at,path in components:
   data=path.read_bytes();require(at>=end and at+len(data)<=0x400000,'Overlapping/oversized component')
   expected[at:at+len(data)]=data;end=at+len(data)
   component_info.append(dict(file=path.name,offset=hex(at),bytes=len(data),sha256=sha(data)))
  require(factory==expected,'Factory components or erased flash differ')
  log.write('PASS factory: metadata offsets, partition CSV/binary/MD5; all component and unused bytes match\n')
  staging=root/'.pio/beta3-shared-validation';staging.mkdir(exist_ok=True)
  for name,data in [('wemos.bin',wemos),('esp32.bin',app),('factory.bin',factory)]:
   safety.check(data,name);(staging/name).write_bytes(data)
  for chip,other,flags in [('wemos','esp32',[]),('esp32','wemos',['-DESP32=1'])]:
   target=staging/('validator-'+chip)
   run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-pedantic','-fsanitize=address,undefined','-fno-omit-frame-pointer',*flags,'-I',root/'include',root/'tests/validate_firmware_file.cpp','-o',target,'-lcrypto'])
   run([target,staging/(chip+'.bin'),staging/(other+'.bin'),staging/'factory.bin'])
  files={PREFIX+'_Wemos_OTA.bin':wemos,PREFIX+'_ESP32_OTA.bin':app,PREFIX+'_ESP32_Factory.bin':factory}
  resources={}
  for chip,text,raw,limit in [('esp8266',wemos_log,wemos,1044464),('esp32',build_log,app,slot0.size)]:
   ram=re.findall(r'RAM:.*used (\d+) bytes from (\d+) bytes',text)[-1];flash=re.findall(r'Flash:.*used (\d+) bytes from (\d+) bytes',text)[-1]
   resources[chip]=dict(binary_bytes=len(raw),ota_limit_bytes=limit,ota_reserve_bytes=limit-len(raw),static_ram_bytes=int(ram[0]),linker_flash_bytes=int(flash[0]),hardware_tested=chip=='esp8266')
  info=dict(version=VERSION,prerelease=True,firmware_source_commit=firmware_commit,wemos_reused_unchanged=True,wemos_sha256=WEMOS_SHA,platforms=resources,esp32_factory_components=component_info,esp32_factory_offset='0x0',esp32_flash_bytes=0x400000,esp32_flash_mode='dio',esp32_flash_frequency_hz=40000000,esp32_partitions=[dict(name=p.name,type=p.type,subtype=p.subtype,offset=hex(p.offset),bytes=p.size) for p in table],partition_csv_sha256=sha(csv.read_bytes()),partition_binary_sha256=sha(partition_bytes),esp32_build_dependencies=len(dependencies),hardware_limits=['ESP32 AP association and WiFi persistence after full power loss remain unverified','ESP32 OTA reserve is very small; verify each future image'],validation='Both actual production OTA validators accept matching OTA and reject mutation/truncation/other-chip/factory/wrong SHA; factory byte exact')
  files['BUILD_INFO_'+VERSION+'_GEMEINSAM.json']=(json.dumps(info,indent=2)+'\n').encode()
  files['TESTANLEITUNG_'+VERSION+'_GEMEINSAM.md']=(root/'docs/TESTANLEITUNG_2.0.11-beta.3_GEMEINSAM.md').read_bytes()
  notes=(root/'docs/RELEASE_v2.0.11-beta.3.md').read_text()+'\n## Geprüfte Firmwaredateien\n\n| Datei | Byte | SHA-256 |\n|---|---:|---|\n'
  for name,data in files.items():
   if name.endswith('.bin'):notes+='| '+name+' | '+str(len(data))+' | `'+sha(data)+'` |\n'
  notes+='\nGemeinsamer Firmware-Quellcommit: `'+firmware_commit+'`.\n'
  files['RELEASE_NOTES_'+VERSION+'.md']=notes.encode()
  for name,data in files.items():safety.check(data,name)
  files['SHA256SUMS_'+VERSION+'_GEMEINSAM.txt']=''.join(sha(data)+'  '+name+'\n' for name,data in sorted(files.items())).encode()
  output.mkdir()
  for name,data in files.items():(output/name).write_bytes(data)
  print(json.dumps(info,indent=2));print('Release assets:',output)
if __name__=='__main__':main()
