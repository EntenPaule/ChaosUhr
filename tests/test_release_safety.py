from pathlib import Path
import base64
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'scripts'))
from release_safety import ReleaseSafety

with tempfile.TemporaryDirectory() as temp:
    folder = Path(temp)
    (folder / 'include').mkdir()
    (folder / '.pio/github-auth').mkdir(parents=True)
    header_secret = 'fixture-legacy-secret-42'
    current_secret = ' fixture-admin-secret-84 '
    github_secret = 'fixture-local-access-token-99'
    (folder / 'include/secrets.local.h').write_text('#define CHAOSUHR_OTA_PASSWORD "' + header_secret + '"\n')
    (folder / '.pio/device-admin-password.txt').write_text(current_secret + '\n')
    (folder / '.pio/github-auth/hosts.yml').write_text('github.com:\n    oauth_token: ' + github_secret + '\n')
    checker = ReleaseSafety(folder)
    checker.check(b'Harmless public source', 'source')
    for secret in (header_secret, current_secret, github_secret):
        for data in (secret.encode(), base64.b64encode(secret.encode()), base64.b64encode(b'admin:' + secret.encode())):
            try:
                checker.check(data, 'fixture')
            except SystemExit as error:
                assert secret not in str(error) and 'fixture' in str(error)
            else:
                raise AssertionError('Credential was not rejected')
    for data in (b'-----BEGIN ' + b'PRIVATE KEY-----', b'gho_' + b'X' * 40):
        try:
            checker.check(data, 'fixture')
        except SystemExit:
            pass
        else:
            raise AssertionError('Credential pattern was not rejected')
print('PASS release safety: historical/current passwords, preserved spaces, Basic/base64 encodings, local GitHub credentials and private-key/token patterns without echoed values')
