"""Collect reviewed public sources; never read private build/device directories."""
from pathlib import Path
import re


def collect_public_sources(project, default_environment):
    if default_environment not in ('d1_mini_public', 'esp32dev_public'):
        raise ValueError('Unknown public build environment')
    source = {}
    patterns = ('src/*.cpp', 'src/*.h', 'include/*.h', 'include/*.def', 'scripts/*.py', 'scripts/*.js',
                'tests/*.cpp', 'tests/*.in', 'tests/*.py', 'tests/*.js',
                'tests/*.json', 'tests/*.h', 'tests/public_stubs/*.h', 'tests/mqtt_stubs/*.h',
                'examples/*.chaos', 'partitions/*.csv', 'third_party/*')
    for pattern in patterns:
        for path in project.glob(pattern):
            if path.is_file() and not path.name.startswith('secrets.'):
                source[path.relative_to(project).as_posix()] = path.read_bytes()
    for library in ('ChaosUhrWebServer', 'ChaosUhrESP32WebServer'):
        for path in (project/'lib'/library).rglob('*'):
            if path.is_file() and (path.suffix in ('.h', '.cpp') or
                                  path.name in ('library.json', 'UPSTREAM_SHA256.json', 'LICENSE')):
                source[path.relative_to(project).as_posix()] = path.read_bytes()
    docs = ('PUBLIC_FIRMWARE.md', 'ESP32_FIRMWARE.md', 'AUTOMATIK_2_0_7.md',
            'ANIMATIONEN_FORMAT.md', 'CHATGPT_ANIMATION_PROMPT.txt', 'RELEASE_v2.0.7.md',
            'RELEASE_v2.0.8.md', 'UPGRADE_2_0_8.md', 'SICHERHEIT_2_0_8.md',
            'GITHUB_UPDATES.md', 'MQTT_2_0_10.md', 'UPGRADE_2_0_10.md', 'RELEASE_v2.0.10.md',
            'VERGLEICH_GITHUB.md','FEHLERCODES.md','RELEASE_v2.0.11.md','ENTWICKLUNGSBERICHT_2_0_11.md',
            'FLASHANLEITUNG_2.0.11.md','TESTCHECKLISTE_2.0.11.md','BEKANNTE_EINSCHRAENKUNGEN_2.0.11.md',
            'SITZUNGEN_2_0_11.md','BERICHT_2.0.11-beta.1.md','TESTANLEITUNG_2.0.11-beta.1.md','BERICHT_2.0.11-beta.2.md','TESTANLEITUNG_2.0.11-beta.2.md')
    for name in ('README.md', 'LICENSE', 'THIRD_PARTY.md', 'CHANGELOG.md', '.gitignore',
                 *('docs/'+name for name in docs)):
        source[name] = (project/name).read_bytes()
    config = (project/'platformio.ini').read_text()
    config, count = re.subn(r'^default_envs = (?:d1_mini_public|esp32dev_public)$',
                           'default_envs = '+default_environment, config, count=1, flags=re.M)
    if count != 1:
        raise ValueError('Unexpected default build environment')
    source['platformio.ini'] = config.encode()
    if any('.pio' in Path(name).parts or Path(name).name == 'AGENTS.md' for name in source):
        raise ValueError('Private file in public source collection')
    return source
