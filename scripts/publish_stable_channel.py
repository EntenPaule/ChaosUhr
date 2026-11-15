"""Publish only verified, explicitly approved Stable assets to firmware-updates.
No build, release creation, device access, factory publication or force push.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
import urllib.request
import urllib.parse

REPO = 'EntenPaule/ChaosUhr'
BASE = f'https://raw.githubusercontent.com/{REPO}/firmware-updates/'
APPROVAL = '<!-- chaosuhr-stable-ota-approved -->'
BOARDS = {'esp8266': ('d1_mini', 'stable.json', 1044464), 'esp32': ('esp32dev', 'stable-esp32.json', 1966080)}
CASES = [('2.0.10', True), ('2.0.11-beta.5.3', True), ('2.0.11-beta.5.2', True), ('2.0.11', False), ('2.0.12-beta.1', False)]


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def version(tag):
    require(bool(re.fullmatch(r'v(0|[1-9][0-9]{0,2})\.(0|[1-9][0-9]{0,2})\.(0|[1-9][0-9]{0,2})', tag)), 'Only numbered Stable tags are permitted')
    return tag[1:]


class SafeRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, msg, headers, newurl):
        require(newurl.startswith('https://'), 'Refusing non-HTTPS redirect')
        result = super().redirect_request(request, fp, code, msg, headers, newurl)
        if result is not None and urllib.parse.urlsplit(newurl).hostname != 'api.github.com':
            result.remove_header('Authorization')
        return result


def fetch(url, limit=5*1024*1024):
    headers = {'User-Agent': 'ChaosUhr-Stable-channel', 'Cache-Control': 'no-cache'}
    # Authentication never follows a redirect to assets/raw hosts.
    token = os.environ.get('GH_TOKEN') or os.environ.get('GITHUB_TOKEN')
    if url.startswith('https://api.github.com/') and token:
        headers['Authorization'] = 'Bearer ' + token
    with urllib.request.build_opener(SafeRedirect()).open(urllib.request.Request(url, headers=headers), timeout=30) as response:
        require(response.status == 200, 'HTTP download failed')
        data = response.read(limit + 1)
        require(len(data) <= limit, 'Download exceeds limit')
        return data, {key.lower():value for key,value in response.headers.items()}


def api(path):
    return json.loads(fetch(f'https://api.github.com/repos/{REPO}/{path}')[0])


def run(args, cwd=None):
    return subprocess.check_output(list(map(str, args)), cwd=cwd, text=True, stderr=subprocess.STDOUT).strip()


def checksum_entries(data):
    result = {}
    for line in data.decode('ascii').splitlines():
        match = re.fullmatch(r'([a-f0-9]{64})  ([A-Za-z0-9._-]+)', line)
        require(match is not None and match[2] not in result, 'Invalid or duplicate checksum entry')
        result[match[2]] = match[1]
    return result


def asset_bytes(asset, download):
    name = asset['name']
    url = asset['browser_download_url']
    require(url.startswith(f'https://github.com/{REPO}/releases/download/') and url.endswith('/' + name), 'Unexpected release asset URL')
    require(type(asset['size']) is int and 0 < asset['size'] <= 5*1024*1024, 'Invalid asset size')
    data = download(url)[0]
    require(len(data) == asset['size'], 'Release asset size mismatch')
    if asset.get('digest'):
        require(asset['digest'] == 'sha256:' + sha(data), 'GitHub asset digest mismatch')
    return data


def prepare(release, latest, display_source, download=fetch):
    tag = release['tag_name']; v = version(tag)
    require(not release['draft'] and not release['prerelease'], 'Drafts/prereleases cannot change Stable')
    require(latest['tag_name'] == tag and latest['id'] == release['id'], 'Only the current Latest Stable can be published')
    assets = {a['name']: a for a in release['assets']}
    require(len(assets) == len(release['assets']), 'Duplicate release assets')
    require('SHA256SUMS.txt' in assets, 'Missing SHA256SUMS.txt')
    checksums = checksum_entries(asset_bytes(assets['SHA256SUMS.txt'], download))
    # Read pin policy from the immutable release source. Fail closed on changed syntax/layout.
    arrays = re.search(r'if\(esp32\)\{for\(uint8_t p:\{([0-9,]+)\}.*?else\{for\(uint8_t p:\{([0-9,]+)\}', display_source, re.S)
    defaults = re.search(r'value.ledPin=esp32\?(\d+):(\d+)', display_source)
    require(arrays and defaults, 'Unrecognized release pin policy; review required')
    plan = {}
    for chip, (board, manifest, maximum) in BOARDS.items():
        suffix = 'esp32-ota' if chip == 'esp32' else 'esp8266'
        # Exact filenames are a firmware contract. URLs come from actual asset metadata.
        name = f'chaosuhr-{v}-{suffix}.bin'
        require(name in assets and name in checksums and 'factory' not in name.lower(), 'Missing matching OTA asset/checksum')
        data = asset_bytes(assets[name], download)
        require(4120 <= len(data) <= maximum and data[0] == 233, 'Invalid OTA size/type')
        require(chip != 'esp32' or (len(data)+4095)//4096*4096 <= maximum, 'ESP32 image does not fit slot')
        require(sha(data) == checksums[name], 'Release SHA256SUMS mismatch')
        require(v.encode()+b'\0' in data, 'OTA lacks the exact Stable firmware version')
        index = 1 if chip == 'esp32' else 2
        pins = list(map(int, arrays[index].split(','))); pin = int(defaults[index])
        require(pin in pins and len(set(pins)) == len(pins) <= 16, 'Invalid release pins')
        document = dict(format='chaosuhr-update', schema=1, version=v, prerelease=False,
                        board=board, chip=chip, flash_bytes=4194304, led_gpio=pin,
                        binary=name, bytes=len(data), sha256=sha(data), configurable_led_pin=True, supported_led_gpio=pins)
        plan[manifest] = {'manifest': document, 'bytes': data, 'release_url': assets[name]['browser_download_url']}
    return plan


def verify_public(url, expected, timeout=420):
    deadline = time.monotonic() + timeout
    while True:
        try:
            data, headers = fetch(url)
            require(data == expected, 'Public bytes differ (possibly cached)')
            require(headers.get('access-control-allow-origin') == '*', 'Public URL lacks browser CORS')
            return
        except Exception:
            if time.monotonic() >= deadline:
                raise RuntimeError('Public verification failed: ' + url) from None
            print('Waiting for canonical public URL: ' + url, flush=True)
            time.sleep(5)


def validate_images(root, directory, plan):
    for chip, (_, name, _) in BOARDS.items():
        binary = directory / ('validator-' + chip)
        args = ['g++', '-std=c++17', '-I', str(root/'include')]
        if chip == 'esp32': args.append('-DESP32=1')
        run([*args, root/'tests/validate_firmware_file.cpp', '-o', binary, '-lcrypto'])
        path = directory / plan[name]['manifest']['binary']; path.write_bytes(plan[name]['bytes'])
        print(run([binary, path]), flush=True)


def publish(channel, plan, public_verify=verify_public, git=run):
    # Protect all old files and both Preview manifests, not just their version fields.
    tracked = git(['git', 'ls-files'], channel).splitlines()
    baseline = {name: (channel/name).read_bytes() for name in tracked}
    for name, item in plan.items():
        old = json.loads(baseline[name]); old_v = old['version']
        require(re.fullmatch(r'\d+\.\d+\.\d+', old_v), 'Unrecognized current Stable version')
        require(tuple(map(int, old_v.split('.'))) <= tuple(map(int, item['manifest']['version'].split('.'))), 'Refusing Stable downgrade')
        p = channel / item['manifest']['binary']
        if p.exists(): require(p.read_bytes() == item['bytes'], 'Never replace an existing versioned binary')
    # Phase 1: binaries only. An interruption leaves the old manifests intact.
    for item in plan.values():
        p = channel/item['manifest']['binary']
        if not p.exists(): p.write_bytes(item['bytes'])
    git(['git', 'add', '--', *[item['manifest']['binary'] for item in plan.values()]], channel)
    if git(['git', 'diff', '--cached', '--name-only'], channel):
        git(['git', 'commit', '-m', 'Publish verified Stable OTA binaries ' + next(iter(plan.values()))['manifest']['version']], channel)
        git(['git', 'push', 'origin', 'HEAD:refs/heads/firmware-updates'], channel)
    binary_commit = git(['git', 'rev-parse', 'HEAD'], channel)
    for item in plan.values():
        public_verify(BASE + item['manifest']['binary'], item['bytes'])
    # Phase 2: manifests only, after both public binaries are verified.
    for name, item in plan.items():
        (channel/name).write_text(json.dumps(item['manifest'], indent=2) + '\n')
    changed = git(['git', 'diff', '--name-only'], channel).splitlines()
    require(set(changed) <= set(BOARDS[c][1] for c in BOARDS), 'Unexpected channel file mutation')
    git(['git', 'add', '--', *plan.keys()], channel)
    if git(['git', 'diff', '--cached', '--name-only'], channel):
        git(['git', 'commit', '-m', 'Activate verified Stable OTA manifests ' + next(iter(plan.values()))['manifest']['version']], channel)
        git(['git', 'push', 'origin', 'HEAD:refs/heads/firmware-updates'], channel)
    manifest_commit = git(['git', 'rev-parse', 'HEAD'], channel)
    for name, original in baseline.items():
        if name not in plan: require((channel/name).read_bytes() == original, 'Old/Preview file changed')
    for name in plan: public_verify(BASE+name, (channel/name).read_bytes())
    return binary_commit, manifest_commit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tag', required=True)
    parser.add_argument('--expected-commit', help='Optional immutable release commit constraint')
    parser.add_argument('--approved', action='store_true', help='Explicit Stable publishing authorization is already present')
    parser.add_argument('--require-marker', action='store_true')
    parser.add_argument('--publish', action='store_true', help='Without this flag only prepare and validate')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'tag':args.tag,'complete':False})+'\n')
    version(args.tag)
    require(args.approved, 'Explicit Stable authorization is required; a push is not authorization')
    root = Path(__file__).resolve().parents[1]
    remote = run(['git', 'remote', 'get-url', 'origin'], root)
    require(remote in (f'https://github.com/{REPO}.git', f'https://github.com/{REPO}', f'git@github.com:{REPO}.git'), 'Wrong origin repository')
    release = api('releases/tags/'+args.tag); latest = api('releases/latest')
    if args.require_marker: require(APPROVAL in release.get('body', ''), 'Release lacks explicit Stable OTA approval marker')
    commit = api('commits/'+args.tag)['sha']
    if args.expected_commit: require(commit == args.expected_commit, 'Release commit changed')
    header = fetch(f'https://raw.githubusercontent.com/{REPO}/{commit}/include/firmware_version.h')[0].decode()
    require('#define CHAOSUHR_VERSION '+json.dumps(version(args.tag)) in header, 'Release source has a different firmware version')
    display = fetch(f'https://raw.githubusercontent.com/{REPO}/{commit}/include/display_options.h')[0].decode()
    plan = prepare(release, latest, display)
    report = {'tag': args.tag, 'release_commit': commit, 'complete': False, 'release_asset_urls': {name:item['release_url'] for name,item in plan.items()}, 'manifests': {name:item['manifest'] for name,item in plan.items()}}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    with tempfile.TemporaryDirectory(prefix='chaosuhr-stable-') as temp:
        directory = Path(temp);validate_images(root, directory, plan)
        # Test the real source from the released older firmware and current firmware.
        run(['git', 'fetch', '--no-tags', 'origin', 'refs/tags/v2.0.10'], root)
        old_source = run(['git', 'show', 'FETCH_HEAD:include/ota_ui.h'], root)
        (directory/'ota-2.0.10.h').write_text(old_source)
        for name,item in plan.items(): (directory/name).write_text(json.dumps(item['manifest']))
        print(run(['node', root/'tests/test_channel_versions.js', directory, root/'include/ota_ui.h', directory/'ota-2.0.10.h']), flush=True)
        if not args.publish:
            print('Validated only; Stable publication is incomplete', flush=True);return
        run(['git', 'fetch', '--no-tags', 'origin', 'refs/heads/firmware-updates'], root)
        channel = directory/'channel';run(['git', 'worktree', 'add', '--detach', channel, 'FETCH_HEAD'], root)
        try:
            preview = {name:(channel/name).read_bytes() for name in ['preview.json','preview-esp32.json']}
            binary_commit, manifest_commit = publish(channel, plan)
            for name,data in preview.items(): verify_public(BASE+name, data)
            # Feed exactly the canonical downloaded JSON to both original updater implementations.
            for name in [*plan,*preview]: (directory/name).write_bytes(fetch(BASE+name)[0])
            result = run(['node', root/'tests/test_channel_versions.js', directory, root/'include/ota_ui.h', directory/'ota-2.0.10.h'])
            print(result, flush=True)
            report.update(complete=True, preview_sha256={name:sha(data) for name,data in preview.items()}, binary_commit=binary_commit, manifest_commit=manifest_commit, versions_test=result)
            args.report.write_text(json.dumps(report, indent=2)+'\n')
        finally:
            run(['git', 'worktree', 'remove', '--force', str(channel)], root)
    print('COMPLETE: both public Stable channels and actual updater tests verified', flush=True)


if __name__ == '__main__':
    try: main()
    except Exception as error:
        # Do not dump HTTP response bodies, environment, headers or credentials.
        print('INCOMPLETE: '+str(error), flush=True)
        raise SystemExit(1)
