"""Package the ESP32 development build without accessing any hardware.

Run after: pio run -e esp32dev_public
Produces USB factory and application-only OTA images in dist/<version>/esp32/.
"""
import argparse
import ast
import base64
import hashlib
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import warnings
from zipfile import ZipFile, ZIP_DEFLATED
from release_safety import ReleaseSafety
from public_sources import collect_public_sources


def main():
    project = Path(__file__).resolve().parents[1]
    core = Path(os.environ.get("PLATFORMIO_CORE_DIR", str(Path.home()/".platformio")))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--esptool", type=Path, default=core/"packages/tool-esptool/esptool")
    args = parser.parse_args()
    version = re.search(r'^#define CHAOSUHR_VERSION "([^"]+)"$',
                        (project/"include/firmware_version.h").read_text(),re.M).group(1)
    name = "chaosuhr-"+version+"-esp32"
    build = project/".pio/build/esp32dev_public"
    binary = build/"firmware.bin"
    raw = binary.read_bytes()
    if version.encode() not in raw or raw[0] != 0xe9 or struct.unpack_from("<H",raw,12)[0] != 0:
        raise SystemExit("Expected a classic ESP32 image with the ESP32 development version")
    if len(raw) > 0x1e0000:
        raise SystemExit("Application exceeds either OTA partition")
    dependencies = list(build.rglob("*.d"))
    if not dependencies or any("secrets.local.h" in p.read_text(errors="replace") for p in dependencies):
        raise SystemExit("Build dependencies missing or personal header included")

    # Verify both actual personal passwords and their HTTP Basic representation.
    safety=ReleaseSafety(project)
    forbidden = []
    secret_path = project/"include/secrets.local.h"
    if secret_path.exists():
        literals = re.findall(r'^#define\s+CHAOSUHR_\w+_PASSWORD\s+("(?:[^"\\]|\\.)*")',
                              secret_path.read_text(), re.M)
        for literal in literals:
            try:
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore", SyntaxWarning)
                    secret = ast.literal_eval(literal).encode()
            except (SyntaxError, ValueError):
                raise SystemExit("Cannot safely check local credential; value not displayed") from None
            forbidden.extend((secret, base64.b64encode(b"admin:"+secret)))

    def check_clean(data, label):
        safety.check(data,label)
        if any(secret and secret in data for secret in forbidden):
            raise SystemExit("Personal credential found in "+label)

    check_clean(raw, "ESP32 application")
    # Validate the actual partition binary, including two equally sized OTA slots.
    partition_bytes = (build/"partitions.bin").read_bytes()
    entries = {}
    for offset in range(0, len(partition_bytes)-31, 32):
        magic, kind, subtype, start, size, label, flags = struct.unpack_from("<HBBII16sI", partition_bytes, offset)
        if magic != 0x50aa:
            break
        entries[label.rstrip(b"\0").decode()] = (kind, subtype, start, size, flags)
    expected = {
        "nvs": (1, 2, 0x9000, 0x5000, 0),
        "otadata": (1, 0, 0xe000, 0x2000, 0),
        "app0": (0, 0x10, 0x10000, 0x1e0000, 0),
        "app1": (0, 0x11, 0x1f0000, 0x1e0000, 0),
        "spiffs": (1, 0x82, 0x3d0000, 0x30000, 0),
    }
    if entries != expected:
        raise SystemExit("Unexpected 4-MiB/dual-OTA partition layout")

    source=collect_public_sources(project,"esp32dev_public")
    manual=source["docs/ESP32_FIRMWARE.md"].replace(b"../THIRD_PARTY.md", b"THIRD_PARTY.md")
    for path, data in source.items():
        check_clean(data, path)

    esptool = str(args.esptool)
    # image-info and merge-bin are offline operations; no serial port is opened.
    verified = subprocess.run([esptool, "--chip", "esp32", "image-info", str(binary)],
                              check=True, capture_output=True, text=True)
    if not re.search(r"^Validation hash: [0-9a-f]{64} \(valid\)$",verified.stdout,re.I|re.M):
        raise SystemExit("esptool did not confirm application hash validation")
    bootloader = (build/"bootloader.bin").read_bytes()
    boot_app = core/"packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
    with tempfile.TemporaryDirectory(prefix="esp32-package-", dir=project/".pio") as temporary:
        merged = Path(temporary)/"factory.bin"
        subprocess.run([esptool, "--chip", "esp32", "merge-bin", "--output", str(merged),
                        "--flash-mode", "dio", "--flash-freq", "40m", "--flash-size", "4MB",
                        "--pad-to-size", "4MB", "0x1000", str(build/"bootloader.bin"),
                        "0x8000", str(build/"partitions.bin"), "0xe000", str(boot_app),
                        "0x10000", str(binary)], check=True, capture_output=True, text=True)
        factory = merged.read_bytes()
    assert len(factory) == 0x400000
    assert factory[0x1000:0x1000+len(bootloader)] == bootloader
    assert factory[0x8000:0x8000+len(partition_bytes)] == partition_bytes
    assert factory[0xe000:0xe000+boot_app.stat().st_size] == boot_app.read_bytes()
    assert factory[0x10000:0x10000+len(raw)] == raw
    assert factory[0x9000:0xe000] == b"\xff"*0x5000
    check_clean(factory, "factory image")
    artifacts = {name+"-ota.bin": raw, name+"-factory.bin": factory}
    manifest = {
        "version": version, "board": "esp32dev", "chip": "esp32", "led_gpio": 32,
        "flash_bytes": 4194304, "factory_flash_offset": "0x0", "application_flash_offset": "0x10000",
        "ota_slot_bytes": 0x1e0000, "hardware_tested": False,
        "ota_image_reserve_bytes":0x1e0000-len(raw),
        "configurable_led_pin":True,"supported_led_gpio":[13,14,18,19,21,22,23,25,26,27,32,33],
        "mqtt":True,"sensor_slots":4,"credentials_format":2,"password_rounds":10000,
        "settings_backup":"chaosuhr-settings/2","ota_signature_required":False,"tls":False,
        "binary_bytes": len(raw), "factory_bytes": len(factory),
        "platformio_platform": "espressif32@8.0.0", "arduino_core": "3.3.12",
        "sha256": {path: hashlib.sha256(data).hexdigest() for path, data in artifacts.items()},
    }
    import json
    artifacts["BUILD_INFO.json"] = (json.dumps(manifest, indent=2)+"\n").encode()
    artifacts["LIESMICH.md"] = manual
    artifacts["AUTOMATIK_2_0_7.md"] = source["docs/AUTOMATIK_2_0_7.md"]
    for doc in ("MQTT_2_0_10.md","UPGRADE_2_0_10.md","RELEASE_v2.0.10.md","ANIMATIONEN_FORMAT.md","PUBLIC_FIRMWARE.md","SICHERHEIT_2_0_8.md","GITHUB_UPDATES.md","CHATGPT_ANIMATION_PROMPT.txt"):
        artifacts[doc]=source["docs/"+doc]
    for path,data in source.items():
        if path.startswith("docs/"):
            artifacts[Path(path).name]=data.replace(b"../THIRD_PARTY.md", b"THIRD_PARTY.md")
    artifacts["examples/Herz.chaos"]=source["examples/Herz.chaos"]
    for key,data in source.items():
        if key.startswith('third_party/') or key in ('lib/ChaosUhrWebServer/LICENSE','lib/ChaosUhrESP32WebServer/LICENSE'):artifacts[key]=data
    artifacts["LICENSE"] = source["LICENSE"]
    artifacts["THIRD_PARTY.md"] = source["THIRD_PARTY.md"]
    artifacts["third_party/FastLED-LICENSE"] = source["third_party/FastLED-LICENSE"]
    checksums = "".join(hashlib.sha256(data).hexdigest()+"  "+path+"\n" for path,data in artifacts.items())
    artifacts["SHA256SUMS"] = checksums.encode()
    output = project/"dist"/version/"esp32"
    output.mkdir(parents=True, exist_ok=True)
    for path, data in artifacts.items():
        destination = output/path
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
    zip_path = output/(name+".zip")
    with ZipFile(zip_path, "w", ZIP_DEFLATED) as archive:
        for path, data in artifacts.items():
            archive.writestr(path, data)
        for path, data in sorted(source.items()):
            archive.writestr("source/"+path, data)
    with ZipFile(zip_path) as archive:
        assert archive.testzip() is None
        assert len(archive.namelist()) == len(set(archive.namelist()))
        assert archive.read(name+"-ota.bin") == raw
    (output/"SHA256SUMS").write_text(checksums+hashlib.sha256(zip_path.read_bytes()).hexdigest()+"  "+zip_path.name+"\n")
    channel=output/"update-channel";channel.mkdir(exist_ok=True)
    update={"format":"chaosuhr-update","schema":1,"version":version,"prerelease":True,
            "board":"esp32dev","chip":"esp32","flash_bytes":4194304,"led_gpio":32,
            "binary":name+"-ota.bin","bytes":len(raw),"sha256":hashlib.sha256(raw).hexdigest(),
            "configurable_led_pin":True,"supported_led_gpio":[13,14,18,19,21,22,23,25,26,27,32,33]}
    (channel/"preview-esp32.json").write_text(json.dumps(update,indent=2)+"\n")
    (channel/(name+"-ota.bin")).write_bytes(raw)
    print(f"ESP32 package: {zip_path.relative_to(project)}")
    print(f"OTA: {len(raw)} bytes; USB factory: {len(factory)} bytes; hardware tested: NO")
    print("Verified image hash/chip, both OTA slots, merged offsets, empty NVS, credentials absent, source/licenses included.")


if __name__ == "__main__":
    main()
