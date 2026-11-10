"""Offline internal ESP32 candidate only. No upload, erase, tag, push or release."""
from pathlib import Path
import hashlib,importlib.util,json,subprocess
from release_safety import ReleaseSafety
from public_sources import collect_public_sources
ROOT=Path(__file__).resolve().parents[1]
VERSION='2.0.11-beta.4.2'
def sha(data):return hashlib.sha256(data).hexdigest()
def run(args):return subprocess.check_output(list(map(str,args)),text=True,stderr=subprocess.STDOUT)
def main():
 out=ROOT/'dist'/VERSION;assert not out.exists(),'Never overwrite a distributed candidate'
 build=ROOT/'.pio/build/esp32dev_public';core=Path.home()/'.platformio'
 metadata=json.loads((ROOT/('.pio/metadata-'+VERSION+'-esp32.json')).read_text())['esp32dev_public']
 spec=importlib.util.spec_from_file_location('partitions',core/'packages/framework-arduinoespressif32/tools/gen_esp32part.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
 raw=(build/'partitions.bin').read_bytes();table=mod.PartitionTable.from_binary(raw);table.verify();table.verify_size_fits(0x400000)
 assert raw==mod.PartitionTable.from_csv((ROOT/'partitions/esp32_4mb_ota.csv').read_text()).to_binary()
 slots=[p for p in table if p.type==0];assert [(p.offset,p.size) for p in slots]==[(0x10000,0x1e0000),(0x1f0000,0x1e0000)]
 fs=next(p for p in table if p.name=='spiffs');assert (fs.offset,fs.size)==(0x3d0000,0x30000)
 app=(build/'firmware.bin').read_bytes();assert VERSION.encode() in app and len(app)<slots[0].size
 assert (len(app)+4095)//4096*4096<=slots[0].size
 assert slots[0].size-len(app)>=65536,'Reserve below internal 64 KiB minimum'
 components=[(int(x['offset'],0),Path(x['path'])) for x in metadata['extra']['flash_images']]
 assert {p.name for _,p in components}=={'bootloader.bin','partitions.bin','boot_app0.bin'}
 assert {p.name:offset for offset,p in components}=={'bootloader.bin':0x1000,'partitions.bin':0x8000,'boot_app0.bin':0xe000}
 assert int(metadata['extra']['application_offset'],0)==slots[0].offset
 components.append((slots[0].offset,build/'firmware.bin'));components.sort()
 esptool=core/'packages/tool-esptool/esptool'
 for p in [build/'bootloader.bin',build/'firmware.bin']:assert '(valid)' in run([esptool,'--chip','esp32','image-info',p])
 temporary=ROOT/'.pio/foundations-factory.bin';args=[esptool,'--chip','esp32','merge-bin','--output',temporary,'--flash-mode','dio','--flash-freq','40m','--flash-size','4MB','--pad-to-size','4MB']
 expected=bytearray(b'\xff'*0x400000);end=0;details=[]
 for offset,path in components:
  data=path.read_bytes();assert offset>=end;end=offset+len(data);expected[offset:end]=data
  args.extend([hex(offset),path]);details.append(dict(file=path.name,offset=hex(offset),bytes=len(data),sha256=sha(data)))
 run(args);factory=temporary.read_bytes();assert factory==expected
 assert factory[fs.offset:fs.offset+fs.size]==b'\xff'*fs.size
 staging=ROOT/'.pio/foundations-validation';staging.mkdir(exist_ok=True)
 target=staging/'validator';run(['g++','-std=c++17','-DESP32=1','-I',ROOT/'include',ROOT/'tests/validate_firmware_file.cpp','-o',target,'-lcrypto'])
 validation=run([target,build/'firmware.bin',ROOT/'dist/2.0.11-beta.3/ChaosUhr_2.0.11-beta.3_Wemos_OTA.bin',temporary])
 nm=run([core/'packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32-elf-nm','-C',build/'firmware.elf'])
 for name in ('esp_core_dump_init','esp_core_dump_write'):
  assert any(line.endswith(' __wrap_'+name) for line in nm.splitlines())
  assert not any(line.endswith(' '+name) for line in nm.splitlines())
 assert 'panicHandler' in nm or 'panic_handler' in nm
 for name in ('INDEX_HTML','OTA_HTML','ERROR_JS'):
  assert any(line.endswith(' '+name+'_GZ') for line in nm.splitlines())
  assert not any(line.endswith(' '+name) for line in nm.splitlines())
 deps=list(build.rglob('*.d'));assert deps and not any('secrets.local.h' in p.read_text(errors='replace') for p in deps)
 safety=ReleaseSafety(ROOT)
 for name,data in collect_public_sources(ROOT,'d1_mini_public').items():safety.check(data,name)
 files={f'ChaosUhr_{VERSION}_ESP32_OTA.bin':app,f'ChaosUhr_{VERSION}_ESP32_Factory.bin':factory}
 info=dict(source_commit=run(["git","-C",ROOT/".pio/dev-2.0.11","rev-parse","HEAD"]).strip(),version=VERSION,internal_only=True,hardware_tested=False,bytes=len(app),ota_slot_bytes=slots[0].size,ota_reserve_bytes=slots[0].size-len(app),sector_reserve_bytes=slots[0].size-((len(app)+4095)//4096*4096),components=details,partitions=[dict(name=p.name,offset=hex(p.offset),bytes=p.size) for p in table],factory_flash_offset='0x0',filesystem='erased; explicit authorized GUI initialization required; no automatic formatting',validation=validation.strip(),coredump='no persistent dumps; both entrypoints interposed, original symbols absent, panic handler present')
 files['BUILD_INFO_'+VERSION+'.json']=(json.dumps(info,indent=2)+'\n').encode()
 files['ANLEITUNG_'+VERSION+'.md']=(ROOT/'docs/ESP32_UART_BETA_4_2.md').read_bytes()
 for name,data in files.items():safety.check(data,name)
 files['SHA256SUMS.txt']=''.join(sha(data)+'  '+name+'\n' for name,data in sorted(files.items())).encode()
 out.mkdir()
 for name,data in files.items():(out/name).write_bytes(data)
 print(json.dumps(info,indent=2));print(out)
if __name__=='__main__':main()
