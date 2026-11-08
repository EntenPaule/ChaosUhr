"""Build the local 2.0.11 tester bundle from fresh, already built firmware.

First run clean builds of both public environments and tests/run_host_tests.py.
This script never opens a serial port, uploads firmware or changes update channels.
Flash component addresses come from PlatformIO metadata, not a guessed recipe.
"""
import argparse
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
from zipfile import ZipFile, ZipInfo, ZIP_DEFLATED

from public_sources import collect_public_sources
from release_safety import ReleaseSafety


VERSION = "2.0.11"
PREFIX = "ChaosUhr_" + VERSION


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def zip_bytes(files):
    output = io.BytesIO()
    with ZipFile(output, "w", ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            entry = ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            entry.compress_type = ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, data)
    result = output.getvalue()
    with ZipFile(io.BytesIO(result)) as archive:
        require(archive.testzip() is None, "ZIP CRC validation failed")
        require(len(archive.namelist()) == len(set(archive.namelist())), "Duplicate ZIP member")
        require(set(archive.namelist()) == set(files), "Unexpected ZIP member")
        for name, data in files.items():
            require(archive.read(name) == data, "ZIP bytes differ: " + name)
    return result


def partition_rows(table):
    return [dict(name=p.name, type=p.type, subtype=p.subtype,
                 offset=hex(p.offset), bytes=p.size, flags=p.get_flags_list()) for p in table]


def main():
    project = Path(__file__).resolve().parents[1]
    core = Path(os.environ.get("PLATFORMIO_CORE_DIR", str(Path.home() / ".platformio")))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--metadata", type=Path)
    parser.add_argument("--source-repo", type=Path)
    args = parser.parse_args()
    repo = args.source_repo or (project / ".pio/dev-2.0.11" if (project / ".pio/dev-2.0.11").exists() else project)
    require(re.search(r'^#define CHAOSUHR_VERSION "2\.0\.11"$',
                      (project / "include/firmware_version.h").read_text(), re.M), "Wrong firmware version")
    output = project / "dist" / VERSION
    private = project / ".pio"
    private.mkdir(exist_ok=True)
    log_path = private / "validate-testpackage-2.0.11.log"
    with log_path.open("w") as log:
        def run(command):
            result = subprocess.run(list(map(str, command)), check=True, capture_output=True, text=True)
            log.write(result.stdout + result.stderr)
            log.flush()
            return result.stdout

        metadata_path = args.metadata or private / "metadata-2.0.11-esp32.json"
        if args.metadata is None:
            run([core / "penv/bin/pio", "project", "metadata", "-d", project, "-e", "esp32dev_public",
                 "--json-output-path", metadata_path])
        metadata = json.loads(metadata_path.read_text())["esp32dev_public"]
        build = project / ".pio/build/esp32dev_public"
        require(Path(metadata["prog_path"]).resolve() == (build / "firmware.elf").resolve(), "Foreign build metadata")
        extra = metadata["extra"]
        components = [(int(item["offset"], 0), Path(item["path"])) for item in extra["flash_images"]]
        require(len(components) == 3 and {p.name for _, p in components} ==
                {"bootloader.bin", "partitions.bin", "boot_app0.bin"}, "Unreviewed extra flash components")
        application_offset = int(extra["application_offset"], 0)
        components.append((application_offset, build / "firmware.bin"))
        components.sort()

        spec = importlib.util.spec_from_file_location("chaosuhr_partitions", core / "packages/framework-arduinoespressif32/tools/gen_esp32part.py")
        partitions_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(partitions_module)
        csv = project / "partitions/esp32_4mb_ota.csv"
        partition_binary = (build / "partitions.bin").read_bytes()
        table = partitions_module.PartitionTable.from_binary(partition_binary)
        declared = partitions_module.PartitionTable.from_csv(csv.read_text())
        table.verify()
        declared.verify()
        require(table.to_binary() == partition_binary == declared.to_binary(), "CSV/binary partitions differ")
        flash_bytes = 4 * 1024 * 1024
        table.verify_size_fits(flash_bytes)
        ota0 = next(p for p in table if p.type == 0 and p.subtype == 0x10)
        ota1 = next(p for p in table if p.type == 0 and p.subtype == 0x11)
        require(application_offset == ota0.offset and ota0.size == ota1.size, "Wrong application/OTA layout")
        ota_data = next(p for p in table if p.type == 1 and p.subtype == 0)
        require(next(at for at, path in components if path.name == "boot_app0.bin") == ota_data.offset,
                "boot_app0 does not start in OTA data")
        broken_table = bytearray(partition_binary)
        broken_table[36] ^= 1
        try:
            partitions_module.PartitionTable.from_binary(bytes(broken_table))
        except partitions_module.InputError:
            pass
        else:
            raise SystemExit("Corrupt partition table MD5 was accepted")

        source = collect_public_sources(project, "d1_mini_public")
        safety = ReleaseSafety(project)
        for name, data in source.items():
            safety.check(data, name)
        firmware_commit = run(["git", "-C", repo, "log", "-1", "--format=%H", "--",
                               "src", "include", "lib", "platformio.ini", "partitions"]).strip()
        require(re.fullmatch(r"[0-9a-f]{40}", firmware_commit), "Missing firmware source commit")
        for name, data in source.items():
            if name.startswith(("src/", "include/", "lib/", "partitions/")) or name == "platformio.ini":
                actual = subprocess.check_output(["git", "-C", str(repo), "show", firmware_commit + ":" + name])
                require(actual == data, "Firmware differs from source commit: " + name)

        images = {}
        resources = {}
        for env, suffix, max_bytes, build_log in (
                ("d1_mini_public", "Wemos_OTA.bin", 1044464, "build-2.0.11-test-wemos-clean.log"),
                ("esp32dev_public", "ESP32_OTA.bin", ota0.size, "build-2.0.11-test-esp32-clean.log")):
            directory = project / ".pio/build" / env
            data = (directory / "firmware.bin").read_bytes()
            require(VERSION.encode() in data and len(data) <= max_bytes, "Wrong version or image too large: " + env)
            deps = list(directory.rglob("*.d"))
            require(deps and all("secrets.local.h" not in p.read_text() for p in deps), "Private/missing build dependencies")
            text = (private / build_log).read_text()
            require("[SUCCESS]" in text and "Done cleaning" in text and "[FAILED]" not in text,
                    "No successful fresh clean build: " + env)
            ram = re.findall(r"RAM:.*used (\d+) bytes from (\d+) bytes", text)[-1]
            flash = re.findall(r"Flash:.*used (\d+) bytes from (\d+) bytes", text)[-1]
            resources[env] = dict(static_ram_bytes=int(ram[0]), static_ram_limit=int(ram[1]),
                                  linker_flash_bytes=int(flash[0]), ota_slot_bytes=max_bytes,
                                  binary_bytes=len(data), ota_binary_reserve_bytes=max_bytes - len(data),
                                  dependency_files_without_private_header=len(deps))
            images[PREFIX + "_" + suffix] = data

        # Match board flash parameters and actual image header before merging.
        board = json.loads((core / "platforms/espressif32/boards/esp32dev.json").read_text())
        require(board["build"]["flash_mode"] == "dio" and board["build"]["f_flash"] == "40000000L"
                and board["upload"]["flash_size"] == "4MB", "Unreviewed flash settings")
        esp32 = images[PREFIX + "_ESP32_OTA.bin"]
        require(esp32[0] == 0xe9 and esp32[2:4] == bytes([2, 0x20]), "Unexpected ESP32 DIO/40MHz/4MiB header")
        esptool = core / "packages/tool-esptool/esptool"
        for path in (build / "bootloader.bin", build / "firmware.bin"):
            info = run([esptool, "--chip", "esp32", "image-info", path])
            require(re.search(r"Validation hash: [0-9a-f]{64} \(valid\)", info, re.I), "Invalid image hash: " + path.name)
        with tempfile.TemporaryDirectory(prefix="testpackage-2.0.11-", dir=private) as temporary:
            factory_path = Path(temporary) / (PREFIX + "_ESP32_Factory.bin")
            command = [esptool, "--chip", "esp32", "merge-bin", "--output", factory_path,
                       "--flash-mode", "dio", "--flash-freq", "40m", "--flash-size", "4MB", "--pad-to-size", "4MB"]
            for at, path in components:
                command.extend([hex(at), path])
            run(command)
            factory = factory_path.read_bytes()
        expected = bytearray(b"\xff" * flash_bytes)
        end = 0
        component_info = []
        for at, path in components:
            data = path.read_bytes()
            require(at >= end and at + len(data) <= flash_bytes, "Overlapping/out-of-bounds component")
            expected[at:at + len(data)] = data
            end = at + len(data)
            component_info.append(dict(file=path.name, offset=hex(at), bytes=len(data), sha256=sha(data)))
        require(factory == expected, "Factory component/padding bytes differ")
        # Full equality also proves blank NVS, second OTA slot and LittleFS.
        images[PREFIX + "_ESP32_Factory.bin"] = factory
        log.write("PASS factory: metadata offsets, partition CSV/binary/MD5, component bytes and all unused flash erased\n")

        host_log = (private / "tests-2.0.11-testpackage.log").read_text()
        require("PASS credentials:" in host_log and len(re.findall(r"^PASS", host_log, re.M)) >= 55
                and host_log.count("PASS actual admin sessions/MQTT:")==2 and "PASS session GUI:" in host_log
                and not re.search(r"Traceback|ERROR: AddressSanitizer|runtime error:", host_log), "Host suite not complete")
        with tempfile.TemporaryDirectory(prefix="ota-test-2.0.11-", dir=private) as temporary:
            tmp = Path(temporary)
            for name, data in images.items():
                (tmp / name).write_bytes(data)
            for chip in ("Wemos", "ESP32"):
                target = tmp / ("validator-" + chip)
                flags = ["-DESP32=1"] if chip == "ESP32" else []
                run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic", "-fsanitize=address,undefined",
                     "-fno-omit-frame-pointer", *flags, "-I", project / "include", project / "tests/validate_firmware_file.cpp",
                     "-o", target, "-lcrypto"])
                other = "ESP32" if chip == "Wemos" else "Wemos"
                run([target, tmp / (PREFIX + "_" + chip + "_OTA.bin"), tmp / (PREFIX + "_" + other + "_OTA.bin"),
                     tmp / (PREFIX + "_ESP32_Factory.bin")])

        artifacts = dict(images)
        for name in ("FLASHANLEITUNG", "TESTCHECKLISTE", "BEKANNTE_EINSCHRAENKUNGEN"):
            artifacts[name + "_" + VERSION + ".md"] = source["docs/" + name + "_" + VERSION + ".md"]
        artifacts["FEHLERCODES_" + VERSION + ".md"] = source["docs/FEHLERCODES.md"]
        artifacts["SITZUNGEN_" + VERSION + ".md"] = source["docs/SITZUNGEN_2_0_11.md"].replace(
            b"(FEHLERCODES.md)", ("(FEHLERCODES_" + VERSION + ".md)").encode()).replace(
            b"[Entwicklungsbericht](ENTWICKLUNGSBERICHT_2_0_11.md)",
            ("[Quellcode-ZIP mit Entwicklungsbericht unter docs/](" + PREFIX + "_Quellcode.zip)").encode())
        artifacts[PREFIX + "_Quellcode.zip"] = zip_bytes(source)
        artifacts["LICENSE_" + VERSION + ".txt"] = source["LICENSE"]
        artifacts["DRITTANBIETER_" + VERSION + ".md"] = ("# ChaosUhr " + VERSION + " – Lizenzhinweise\n\nVollständige Drittanbieter-Lizenztexte liegen im beigefügten Quellcode-ZIP unter `third_party/` und `lib/`.\n\n").encode() + source["THIRD_PARTY.md"]
        info = dict(version=VERSION, status="local-test", hardware_tested=False, real_broker_tested=False,
                    firmware_source_commit=firmware_commit, platformio_profiles=resources,
                    esp32_factory_flash_offset="0x0", esp32_factory_bytes=len(factory),
                    esp32_factory_components=component_info, esp32_partition_csv="partitions/esp32_4mb_ota.csv",
                    esp32_partition_csv_sha256=sha(csv.read_bytes()), esp32_partition_binary_sha256=sha(partition_binary),
                    esp32_partitions=partition_rows(table), flash_mode="dio", flash_frequency_hz=40000000,
                    flash_bytes=flash_bytes, host_pass_groups=len(re.findall(r"^PASS", host_log, re.M)),
                    ota_validation="Both production validators accept matching OTA; reject mutation/truncation/other chip/factory/wrong SHA",
                    factory_validation="PlatformIO metadata; core partition parser and MD5; exact CSV and byte/padding comparison",
                    credential_check="Known private credentials/token patterns absent; no private header in compiler dependencies",
                    source_archive_sha256=sha(artifacts[PREFIX + "_Quellcode.zip"]),
                    firmware={name: dict(bytes=len(data), sha256=sha(data), ota_allowed="_OTA.bin" in name)
                              for name, data in images.items()},
                    admin_authentication="server-session-cookie", admin_idle_seconds=1800, admin_session_slots=2,
                    admin_cookie_attributes=["HttpOnly","SameSite=Strict","Path=/","host-only"],
                    admin_csrf="per-session", get_requests_renew_session=False, basic_auth_accepted=False,
                    published=False, update_channels_changed=False)
        artifacts["BUILD_INFO_" + VERSION + ".json"] = (json.dumps(info, ensure_ascii=False, indent=2) + "\n").encode()
        for name, data in artifacts.items():
            require(VERSION in name, "Unversioned distribution filename")
            safety.check(data, name)
            if name.endswith(".md"):
                for target in re.findall(r'(?<!!)\[[^\]]+\]\(([^\s)]+)\)', data.decode()):
                    if "://" in target or target.startswith(("#", "mailto:")):
                        continue
                    require(target.split("#")[0] in artifacts, "Broken distribution document link: " + name)
        checksums_name = "SHA256SUMS_" + VERSION + ".txt"
        checksums = "".join(sha(data) + "  " + name + "\n" for name, data in sorted(artifacts.items()))
        artifacts[checksums_name] = checksums.encode()
        package_name = PREFIX + "_TEST.zip"
        package = zip_bytes(artifacts)
        # The external checksum list includes the ZIP; an archive cannot hash itself.
        artifacts[package_name] = package
        artifacts[checksums_name] = (checksums + sha(package) + "  " + package_name + "\n").encode()
        require(not output.exists() or not (set(p.name for p in output.iterdir()) - set(artifacts)), "Unexpected existing distribution files")
        output.mkdir(parents=True, exist_ok=True)
        for name, data in artifacts.items():
            (output / name).write_bytes(data)
        with ZipFile(output / package_name) as archive:
            for name, data in images.items():
                require(archive.read(name) == data == (output / name).read_bytes(), "Distributed firmware differs")
            for line in archive.read(checksums_name).decode().splitlines():
                digest, name = line.split("  ", 1)
                require(sha(archive.read(name)) == digest, "Internal checksum differs")
        for line in artifacts[checksums_name].decode().splitlines():
            digest, name = line.split("  ", 1)
            require(sha((output / name).read_bytes()) == digest, "External checksum differs")
        safety.check(log_path.read_bytes(), log_path.name)
        log.write("PASS test ZIP: complete firmware, versioned manuals/checklist/limits/checksums, sanitized source and licenses; no channel changes\n")
    print("Test package:", output / package_name)
    print("PASS both actual OTA validators, full factory layout, source/credential scan and all ZIP checksums")
    for name, data in images.items():
        print(name, len(data), sha(data))
    print(package_name, len(package), sha(package))


if __name__ == "__main__":
    main()
