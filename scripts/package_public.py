"""Package only the public BIN and an allowlisted, matching source tree.

Run after: pio run -e d1_mini_public
No upload, no device access, no inclusion of private build artifacts.
"""
import ast
import base64
import hashlib
import json
from pathlib import Path
import re
import struct
import warnings
from zipfile import ZipFile, ZIP_DEFLATED
from release_safety import ReleaseSafety
from public_sources import collect_public_sources

project=Path(__file__).resolve().parents[1]
safety=ReleaseSafety(project)
version=re.search(r'^#define CHAOSUHR_VERSION "([^"]+)"$',
                  (project/"include/firmware_version.h").read_text(),re.M).group(1)
name="chaosuhr-"+version+"-esp8266"
binary=project/".pio"/"build"/"d1_mini_public"/"firmware.bin"
raw=binary.read_bytes()
if version.encode() not in raw:
    raise SystemExit("Expected public firmware version was not found")
if raw[0]!=0xe9 or raw[2:4]!=b"\x02\x40" or raw[0x1000]!=0xe9:
    raise SystemExit("Unexpected ESP8266 image/4-MiB-DIO configuration")
declared_length,declared_crc=struct.unpack_from("<II",raw,0x1010)
checked=bytearray(raw)
struct.pack_into("<II",checked,0x1010,0,0)
table=[]
for value in range(256):
    value<<=24
    for _ in range(8):
        value=((value<<1)^(0x04c11db7 if value&0x80000000 else 0))&0xffffffff
    table.append(value)
crc=0xffffffff
for value in checked:crc=((crc<<8)&0xffffffff)^table[((crc>>24)^value)&0xff]
if declared_length!=len(raw) or crc!=declared_crc:
    raise SystemExit("Invalid ESP8266 image length or core CRC")

# Check both actual personal passwords and their Basic-Auth representation.
# This optional local leak check never copies the private header into a release.
forbidden=[]
secret_path=project/"include"/"secrets.local.h"
if secret_path.exists():
    for quoted in re.findall(r'^#define\s+CHAOSUHR_\w+_PASSWORD\s+("(?:[^"\\]|\\.)*")',secret_path.read_text(),re.M):
        try:
            with warnings.catch_warnings():
                warnings.simplefilter("ignore",SyntaxWarning)
                secret=ast.literal_eval(quoted).encode()
        except (SyntaxError,ValueError):
            raise SystemExit("Local password literal cannot be checked safely; no value displayed") from None
        forbidden.extend((secret,base64.b64encode(b"admin:"+secret)))

def check_clean(data,label):
    safety.check(data,label)
    if any(secret and secret in data for secret in forbidden):
        raise SystemExit("Personal credential detected in "+label)

check_clean(raw,"public firmware")
source=collect_public_sources(project,"d1_mini_public")
manual=source["docs/PUBLIC_FIRMWARE.md"]
for path,data in source.items():check_clean(data,path)
if any("secrets.local.h"==Path(path).name or ".pio" in Path(path).parts for path in source):
    raise SystemExit("Private artifact in source allowlist")

# Require evidence that this environment was built without the private header.
dependencies=list(binary.parent.rglob("*.d"))
if not dependencies or any("secrets.local.h" in path.read_text(errors="replace") for path in dependencies):
    raise SystemExit("Public build dependencies are missing or include the personal header")

digest=hashlib.sha256(raw).hexdigest()
output=project/"dist"/version/"esp8266"
output.mkdir(parents=True,exist_ok=True)
(output/(name+".bin")).write_bytes(raw)
(output/"LIESMICH.md").write_bytes(manual)
automatic=source["docs/AUTOMATIK_2_0_7.md"]
(output/"AUTOMATIK_2_0_7.md").write_bytes(automatic)
extras={"ANIMATIONEN_FORMAT.md":source["docs/ANIMATIONEN_FORMAT.md"],
        "GITHUB_UPDATES.md":source["docs/GITHUB_UPDATES.md"],
        "UPGRADE_2_0_8.md":source["docs/UPGRADE_2_0_8.md"],
        "SICHERHEIT_2_0_8.md":source["docs/SICHERHEIT_2_0_8.md"],
        "RELEASE_v2.0.7.md":source["docs/RELEASE_v2.0.7.md"],
        "RELEASE_v2.0.10.md":source["docs/RELEASE_v2.0.10.md"],
        "UPGRADE_2_0_10.md":source["docs/UPGRADE_2_0_10.md"],
        "MQTT_2_0_10.md":source["docs/MQTT_2_0_10.md"],
        "RELEASE_v2.0.8.md":source["docs/RELEASE_v2.0.8.md"],
        "ESP32_FIRMWARE.md":source["docs/ESP32_FIRMWARE.md"],
        "CHATGPT_ANIMATION_PROMPT.txt":source["docs/CHATGPT_ANIMATION_PROMPT.txt"],
        "examples/Herz.chaos":source["examples/Herz.chaos"]}
for path, data in source.items():
    if path.startswith("docs/"):
        extras[Path(path).name] = data.replace(b"../THIRD_PARTY.md", b"THIRD_PARTY.md")
for path,data in extras.items():
    destination=output/path;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(data)
metadata={"version":version,"board":"d1_mini","chip":"esp8266","led_gpio":12,
          "flash_bytes":4194304,"binary_bytes":len(raw),"hardware_tested":False,
          "animation_uploads":True,"upload_format":"chaos/1","max_uploads":8,"max_upload_bytes":24576,
          "sha256":{name+".bin":digest}}
metadata.update(configurable_led_pin=True,supported_led_gpio=[4,5,12,13,14],mqtt=True,sensor_slots=4,settings_backup_schema=2,
                ota_max_bytes=1044464,ota_image_reserve_bytes=1044464-len(raw),
                credentials_format=2,password_kdf="PBKDF2-HMAC-SHA256",password_rounds=10000,
                legacy_migration_requires_readable_filesystem=True,settings_backup="chaosuhr-settings/2",
                ota_integrity="ESP8266 core CRC and product check",ota_signature_required=False,tls=False)
metadata["github_update_channel"]="https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/"
build_info=(json.dumps(metadata,indent=2)+"\n").encode()
(output/"BUILD_INFO.json").write_bytes(build_info)
zip_path=output/(name+".zip")
with ZipFile(zip_path,"w",ZIP_DEFLATED) as archive:
    archive.writestr(name+".bin",raw)
    archive.writestr("LIESMICH.md",manual)
    archive.writestr("BUILD_INFO.json",build_info)
    for path,data in extras.items():archive.writestr(path,data)
    archive.writestr("SHA256SUMS",digest+"  "+name+".bin\n")
    archive.writestr("LICENSE",source["LICENSE"])
    archive.writestr("THIRD_PARTY.md",source["THIRD_PARTY.md"])
    for path,data in source.items():
        if path.startswith("third_party/") or path in ("lib/ChaosUhrWebServer/LICENSE", "lib/ChaosUhrESP32WebServer/LICENSE"):
            archive.writestr(path,data)
    for path,data in sorted(source.items()):archive.writestr("source/"+path,data)
with ZipFile(zip_path) as archive:
    assert archive.testzip() is None
    assert len(archive.namelist()) == len(set(archive.namelist()))
    assert archive.read(name+".bin") == raw
zip_digest=hashlib.sha256(zip_path.read_bytes()).hexdigest()
(output/"SHA256SUMS").write_text(digest+"  "+name+".bin\n"+zip_digest+"  "+name+".zip\n")
print(f"Public release: {name}.bin ({len(raw)} bytes), {name}.zip ({zip_path.stat().st_size} bytes)")
print("Checked: personal passwords absent, private header absent from build/source, matching source and licenses included.")

# Prepared locally only. Publishing this directory requires a separate explicit
# instruction. Keep previous stable.json when publishing a preview release.
channel=output/"update-channel";channel.mkdir(exist_ok=True)
manifest={"format":"chaosuhr-update","schema":1,"version":version,"prerelease":True,
          "board":"d1_mini","chip":"esp8266","flash_bytes":4194304,"led_gpio":12,
          "binary":name+".bin","bytes":len(raw),"sha256":digest,
          "configurable_led_pin":True,"supported_led_gpio":[4,5,12,13,14]}
(channel/"preview.json").write_text(json.dumps(manifest,indent=2)+"\n")
(channel/(name+".bin")).write_bytes(raw)
print("Prepared update-channel/preview.json and matching BIN locally; not published.")
