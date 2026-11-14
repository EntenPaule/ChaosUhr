"""Offline beta.5.1 package from verified local builds. Never upload or publish."""
from pathlib import Path
import hashlib,importlib.util,json,re,subprocess,zipfile
from release_safety import ReleaseSafety
from public_sources import collect_public_sources
ROOT=Path(__file__).resolve().parents[1];VERSION='2.0.11-beta.5.1'
def sha(data):return hashlib.sha256(data).hexdigest()
def run(args):return subprocess.check_output(list(map(str,args)),text=True,stderr=subprocess.STDOUT)
def main():
 out=ROOT/'dist'/VERSION
 assert not out.exists(),'Never overwrite distributed prerelease artifacts'
 assert '#define CHAOSUHR_VERSION "'+VERSION+'"' in (ROOT/'include/firmware_version.h').read_text()
 commit=run(['git','-C',ROOT,'rev-parse','HEAD']).strip()
 source=collect_public_sources(ROOT,'d1_mini_public')
 workspace=ROOT.parent.parent if ROOT.parent.name=='.pio' else ROOT
 scanners=[ReleaseSafety(ROOT),ReleaseSafety(workspace)]
 def check(data,label):
  for safety in scanners:safety.check(data,label)
 for name,data in source.items():
  check(data,name)
  assert subprocess.check_output(['git','-C',str(ROOT),'show',commit+':'+name])==data,'Uncommitted public source: '+name
 tests=(ROOT/'.pio/tests-beta51-complete.log').read_text()
 assert 'Traceback' not in tests and 'PASS credentials:' in tests and 'PASS MQTT brightness:' in tests
 assert 'PASS actual brightness HTTP API:' in tests and 'PASS brightness GUI:' in tests
 log=(ROOT/'.pio/build-beta51-final.log').read_text()
 assert '[FAILED]' not in log and 'd1_mini_public' in log and 'esp32dev_public' in log and log.count('[SUCCESS]')==2
 build=ROOT/'.pio/build/esp32dev_public';core=Path.home()/'.platformio'
 metadata=json.loads((ROOT/'.pio/metadata-beta51-esp32.json').read_text())['esp32dev_public']
 assert Path(metadata['prog_path']).resolve()==(build/'firmware.elf').resolve()
 spec=importlib.util.spec_from_file_location('partitions',core/'packages/framework-arduinoespressif32/tools/gen_esp32part.py')
 mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
 raw=(build/'partitions.bin').read_bytes();table=mod.PartitionTable.from_binary(raw);table.verify();table.verify_size_fits(0x400000)
 assert raw==mod.PartitionTable.from_csv((ROOT/'partitions/esp32_4mb_ota.csv').read_text()).to_binary()
 baseline=subprocess.check_output(['git','-C',str(ROOT),'show','dc852e1:partitions/esp32_4mb_ota.csv'])
 assert baseline==(ROOT/'partitions/esp32_4mb_ota.csv').read_bytes()
 slots=[p for p in table if p.type==0];assert [(p.offset,p.size) for p in slots]==[(0x10000,0x1e0000),(0x1f0000,0x1e0000)]
 components=[(int(x['offset'],0),Path(x['path'])) for x in metadata['extra']['flash_images']]
 assert {p.name:offset for offset,p in components}=={'bootloader.bin':0x1000,'partitions.bin':0x8000,'boot_app0.bin':0xe000}
 assert int(metadata['extra']['application_offset'],0)==0x10000
 components.append((0x10000,build/'firmware.bin'));components.sort()
 esptool=core/'packages/tool-esptool/esptool'
 for p in [build/'bootloader.bin',build/'firmware.bin']:assert '(valid)' in run([esptool,'--chip','esp32','image-info',p])
 temporary=ROOT/'.pio/beta51-factory-verified.bin'
 args=[esptool,'--chip','esp32','merge-bin','--output',temporary,'--flash-mode','dio','--flash-freq','40m','--flash-size','4MB','--pad-to-size','4MB']
 expected=bytearray(b'\xff'*0x400000);end=0;details=[]
 for offset,path in components:
  data=path.read_bytes();assert offset>=end;end=offset+len(data);expected[offset:end]=data;args.extend([hex(offset),path]);details.append(dict(file=path.name,offset=hex(offset),bytes=len(data),sha256=sha(data)))
 run(args);factory=temporary.read_bytes();assert factory==expected
 apps={chip:(ROOT/'.pio/build'/env/'firmware.bin').read_bytes() for chip,env in [('esp8266','d1_mini_public'),('esp32','esp32dev_public')]}
 assets=(ROOT/'include/esp32_web_assets.h').read_text()
 for numbers in re.findall(r'_GZ\[\] PROGMEM=\{([0-9,]+)\}',assets):assert bytes(map(int,numbers.split(','))) in apps['esp32'],'Stale ESP32 GUI in BIN'
 resources={};validations={}
 for chip,env,limit in [('esp8266','d1_mini_public',1044464),('esp32','esp32dev_public',1966080)]:
  data=apps[chip];assert VERSION.encode() in data and len(data)<=limit
  assert (len(data)+4095)//4096*4096<=limit if chip=='esp32' else True
  deps=list((ROOT/'.pio/build'/env).rglob('*.d'));assert deps and all('secrets.local.h' not in p.read_text(errors='replace') for p in deps)
  target=ROOT/'.pio'/('validator-beta51-'+chip)
  flags=['-DESP32=1'] if chip=='esp32' else []
  run(['g++','-std=c++17',*flags,'-I',ROOT/'include',ROOT/'tests/validate_firmware_file.cpp','-o',target,'-lcrypto'])
  other='esp32' if chip=='esp8266' else 'd1_mini'
  other_env='esp32dev_public' if chip=='esp8266' else 'd1_mini_public'
  validations[chip]=run([target,ROOT/'.pio/build'/env/'firmware.bin',ROOT/'.pio/build'/other_env/'firmware.bin',temporary]).strip()
  section=log.split('Processing '+env,1)[1].split('Processing ',1)[0]
  ram=re.findall(r'RAM:.*used (\d+) bytes from (\d+) bytes',section)[-1]
  flash=re.findall(r'Flash:.*used (\d+) bytes from (\d+) bytes',section)[-1]
  resources[chip]=dict(binary_bytes=len(data),ota_limit_bytes=limit,ota_reserve_bytes=limit-len(data),static_ram_bytes=int(ram[0]),static_ram_limit=int(ram[1]),static_ram_remaining_bytes=int(ram[1])-int(ram[0]),linker_flash_bytes=int(flash[0]),hardware_tested=False)
  debug=ROOT/'.pio/debug'/VERSION/env;assert not debug.exists();debug.mkdir(parents=True)
  for name in ['firmware.bin','firmware.elf']: (debug/name).write_bytes((ROOT/'.pio/build'/env/name).read_bytes())
 info=dict(version=VERSION,source_commit=commit,internal_only=True,host_pass_groups=sum(line.startswith('PASS') for line in tests.splitlines()),platforms=resources,esp32_sector_reserve_bytes=1966080-((len(apps['esp32'])+4095)//4096*4096),esp32_factory_components=details,esp32_factory_flash_offset='0x0',partitions=[dict(name=p.name,offset=hex(p.offset),bytes=p.size) for p in table],validation=validations,limitations=['No hardware test or broker acknowledgment/durability proof','Backup3 requires new importer; old backup1/2 accepted','GUI clock settings and brightness policy saved independently'])
 files={f'ChaosUhr_{VERSION}_Wemos_OTA.bin':apps['esp8266'],f'ChaosUhr_{VERSION}_ESP32_OTA.bin':apps['esp32'],f'ChaosUhr_{VERSION}_ESP32_Factory.bin':factory,f'ANLEITUNG_{VERSION}.md':(ROOT/'docs/MQTT_HELLIGKEIT_BETA_5_1.md').read_bytes(),f'BUILD_INFO_{VERSION}.json':(json.dumps(info,indent=2)+'\n').encode()}
 for name,data in files.items():check(data,name)
 files['SHA256SUMS.txt']=''.join(sha(data)+'  '+name+'\n' for name,data in sorted(files.items())).encode()
 out.mkdir()
 for name,data in files.items():(out/name).write_bytes(data)
 with zipfile.ZipFile(out/f'ChaosUhr_{VERSION}_TEST.zip','w',zipfile.ZIP_DEFLATED) as archive:
  for name,data in files.items():archive.writestr(name,data)
 with zipfile.ZipFile(out/f'ChaosUhr_{VERSION}_TEST.zip') as archive:
  assert archive.testzip() is None
  for name,data in files.items():assert archive.read(name)==data
 print(json.dumps(info,indent=2));print(out)
if __name__=='__main__':main()
