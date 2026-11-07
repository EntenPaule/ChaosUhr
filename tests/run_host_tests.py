"""Run firmware logic tests locally, without a device or PlatformIO downloads."""
from contextlib import redirect_stdout
from datetime import datetime, timezone
import io
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import stat
import struct
import subprocess
import tempfile
import time

project = Path(__file__).resolve().parents[1]
subprocess.run(["python3",str(project/"tests/test_release_safety.py")],check=True)
subprocess.run(["python3",str(project/"tests/run_http_parser_test.py")],check=True)
build_dir = project / ".pio" / "host-tests"
build_dir.mkdir(parents=True, exist_ok=True)
binary = build_dir / "test_logic"
subprocess.run([
    "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic",
    "-I", str(project / "include"), str(project / "tests" / "test_logic.cpp"),
    "-o", str(binary),
], check=True)
subprocess.run([str(binary)], check=True)

for name,sources,flags in (
    ("security",[project/"tests/test_security.cpp"],["-I",str(project/"tests")]),
    ("animation_uploads",[project/"tests/test_animation_uploads.cpp"],["-I",str(project/"tests")]),
    ("public_credentials",[project/"tests"/"test_public_credentials.cpp"],[]),
    ("public_access",[project/"tests"/"test_public_access.cpp",project/"src"/"public_access.cpp"],
      ["-DCHAOSUHR_PUBLIC_BUILD=1","-I",str(project/"tests"/"public_stubs")]),
    ("public_access_esp32",[project/"tests"/"test_public_access.cpp",project/"src"/"public_access.cpp"],
      ["-DCHAOSUHR_PUBLIC_BUILD=1","-DESP32=1","-I",str(project/"tests"/"public_stubs")]),
):
    target=build_dir/("test_"+name)
    subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror","-pedantic",
                    "-fsanitize=address,undefined","-fno-omit-frame-pointer",
                    *flags,"-I",str(project/"include"),*map(str,sources),"-o",str(target),"-lcrypto"],check=True)
    arguments=[str(target)]
    if name=="animation_uploads":arguments.append(str(project/"examples/Herz.chaos"))
    subprocess.run(arguments,check=True)

clock_binary=build_dir/"test_clock"
subprocess.run([
    "g++","-std=c++17","-Wall","-Wextra","-Werror","-pedantic",
    "-fsanitize=address,undefined","-fno-omit-frame-pointer",
    "-I",str(project/"include"),str(project/"tests"/"test_clock.cpp"),
    "-o",str(clock_binary),
],check=True)
subprocess.run([str(clock_binary)],check=True)

legacy_binary = build_dir / "test_legacy"
subprocess.run([
    "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic",
    "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
    "-I", str(project/"include"), str(project/"tests"/"test_legacy.cpp"),
    str(project/"src"/"legacy_animation.cpp"), "-o", str(legacy_binary),
], check=True)
subprocess.run([str(legacy_binary)], check=True)
manifest = json.loads((project/"tests"/"legacy_assets_manifest.json").read_text())
dump = subprocess.check_output([str(legacy_binary),"--dump"], text=True)
frames_checked = set()
sequence_counts = {}
for line in dump.splitlines():
    words = line.split()
    effect, index, duration = map(int,words[:3])
    expected = manifest["sequences"][str(effect)][index]
    assert duration == expected["ms"]
    pixels = [int(value,16) for value in words[3:]]
    assert len(pixels) == 300
    digest = hashlib.sha256(struct.pack("<300I",*pixels)).hexdigest()
    assert digest == manifest["frames"][expected["frame"]]
    frames_checked.add(expected["frame"])
    sequence_counts[str(effect)] = index+1
assert frames_checked == set(manifest["frames"])
assert sequence_counts == {effect:len(frames) for effect,frames in manifest["sequences"].items()}
print(f"PASS original assets: {len(frames_checked)} source-verified frames, exact RGB888 pixels, sequences and timings")

subprocess.run(["node",str(project/"tests"/"test_web_ui.js")], check=True)
subprocess.run(["node",str(project/"tests"/"test_public_ui.js")], check=True)
subprocess.run(["node",str(project/"tests"/"test_backup_ui.js")], check=True)
subprocess.run(["node",str(project/"tests"/"test_ota_ui.js")], check=True)
subprocess.run(["node",str(project/"tests"/"test_github_update_ui.js")], check=True)

source = (project / "src" / "main.cpp").read_text()
example=(project/"examples/Herz.chaos").read_text()
examples_header=(project/"include/animation_examples.h").read_text()
assert re.search(r'R"CHAOS\((.*?)\)CHAOS"',examples_header,re.S).group(1)==example
assert re.search(r'R"PROMPT\((.*?)\)PROMPT"',examples_header,re.S).group(1)==(project/"docs/CHATGPT_ANIMATION_PROMPT.txt").read_text()
print("PASS exchange files: shipped example and ChatGPT prompt exactly match firmware downloads")
animation_api=(project/"tests/test_animation_api.cpp.in").read_text()
for marker,name,result in (("@AUTH@","auth","bool"),("@MUTATION@","animationMutationAllowed","bool"),
    ("@INITIALIZE@","animationInitApi","void"),("@UPLOAD_CHUNK@","animationUploadChunk","void"),
    ("@UPLOAD_DONE@","animationUploadDone","void"),("@DELETE@","animationDeleteApi","void")):
    function=re.search(r"static "+result+" "+name+r"\(\)\{.*?\n\}",source,re.DOTALL).group(0)
    animation_api=animation_api.replace(marker,function)
animation_api_binary=build_dir/"test_animation_api"
subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror","-pedantic","-DCHAOSUHR_PUBLIC_BUILD=1",
                "-fsanitize=address,undefined","-fno-omit-frame-pointer","-I",str(project/"include"),
                "-I",str(project/"tests"),"-x","c++","-","-o",str(animation_api_binary)],
                input=animation_api,text=True,check=True)
subprocess.run([str(animation_api_binary),str(project/"examples/Herz.chaos")],check=True)
automatic_source=(project/"tests/test_automatic.cpp.in").read_text()
for marker,name,result in (("@ACTIVE_EFFECT@","activeEffect","uint8_t"),
                           ("@NEXT_SCENE@","nextScene","void"),("@SAVE_API@","saveApi","void")):
    function=re.search(r"static "+result+" "+name+r"\(\)\{.*?\n\}",source,re.DOTALL).group(0)
    automatic_source=automatic_source.replace(marker,function)
automatic_binary=build_dir/"test_automatic"
subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror","-pedantic",
                "-fsanitize=address,undefined","-fno-omit-frame-pointer","-I",str(project/"include"),
                "-x","c++","-","-o",str(automatic_binary)],input=automatic_source,text=True,check=True)
subprocess.run([str(automatic_binary)],check=True)
network = re.search(r"static void serviceNetwork\(\)\{.*?\n\}", source, re.DOTALL).group(0)
network_source = (project / "tests" / "test_network.cpp.in").read_text().replace("@SERVICE_NETWORK@", network)
network_binary = build_dir / "test_network"
subprocess.run([
    "g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic",
    "-x", "c++", "-", "-o", str(network_binary),
], input=network_source, text=True, check=True)
subprocess.run([str(network_binary)], check=True)
public_network_binary=build_dir/"test_public_network"
subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror","-pedantic",
                "-DCHAOSUHR_PUBLIC_BUILD=1","-x","c++","-","-o",str(public_network_binary)],
               input=network_source,text=True,check=True)
subprocess.run([str(public_network_binary)],check=True)
esp32_network_binary=build_dir/"test_esp32_network"
subprocess.run(["g++","-std=c++17","-Wall","-Wextra","-Werror","-pedantic",
                "-DCHAOSUHR_PUBLIC_BUILD=1","-DESP32=1","-x","c++","-","-o",str(esp32_network_binary)],
               input=network_source,text=True,check=True)
subprocess.run([str(esp32_network_binary)],check=True)

configured_zones = re.findall(r'config(?:Tz)?Time\("([^"]+)"', source)
assert len(configured_zones)==2 and len(set(configured_zones))==1
configured_zone = configured_zones[0]
os.environ["TZ"] = configured_zone
time.tzset()
for utc, expected, daylight in [
    ("2026-03-29T00:59:59", "2026-03-29 01:59:59", 0),
    ("2026-03-29T01:00:00", "2026-03-29 03:00:00", 1),
    ("2026-10-25T00:59:59", "2026-10-25 02:59:59", 1),
    ("2026-10-25T01:00:00", "2026-10-25 02:00:00", 0),
]:
    timestamp = datetime.fromisoformat(utc).replace(tzinfo=timezone.utc).timestamp()
    local = time.localtime(timestamp)
    assert time.strftime("%Y-%m-%d %H:%M:%S", local) == expected
    assert local.tm_isdst == daylight
print("PASS timezone: configured POSIX rule at both 2026 DST transitions (host libc)")

with tempfile.TemporaryDirectory(prefix="credentials-", dir=build_dir) as temporary:
    class BuildEnv:
        def subst(self, variable):
            assert variable == "$PROJECT_DIR"
            return temporary

    def run_generator():
        captured = io.StringIO()
        with redirect_stdout(captured):
            runpy.run_path(str(project / "scripts" / "prepare_secrets.py"), init_globals={
                "env": BuildEnv(), "Import": lambda name: None,
            })
        return captured.getvalue()

    logs = run_generator()
    path = Path(temporary) / "include" / "secrets.local.h"
    contents = path.read_text()
    passwords = re.findall(r'#define CHAOSUHR_\w+_PASSWORD "([^"]+)"', contents)
    assert len(passwords) == 2 and len(passwords[0]) >= 16
    assert 8 <= len(passwords[1]) <= 63 and passwords[0] != passwords[1]
    assert stat.S_IMODE(path.stat().st_mode) == 0o600
    assert not any(password in logs for password in passwords)
    assert not run_generator() and path.read_text() == contents
print("PASS credentials: generated once, private file permissions, no passwords in logs")
