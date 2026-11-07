"""Offline release checks. Report locations only, never credential values."""
import ast
import base64
import re
import warnings


class ReleaseSafety:
    def __init__(self, root):
        self.forbidden = []
        header = root / 'include/secrets.local.h'
        if header.exists():
            for literal in re.findall(r'^#define\s+CHAOSUHR_\w+_PASSWORD\s+("(?:[^"\\]|\\.)*")', header.read_text(), re.M):
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', SyntaxWarning)
                    try:
                        self.add(ast.literal_eval(literal).encode())
                    except (SyntaxError, ValueError):
                        raise SystemExit('Cannot check local credentials safely; value withheld') from None
        password = root / '.pio/device-admin-password.txt'
        if password.exists():
            # Spaces can be part of a password. Remove only its line ending.
            self.add(password.read_bytes().removesuffix(b'\n').removesuffix(b'\r'))
        github = root / '.pio/github-auth/hosts.yml'
        if github.exists():
            for token in re.findall(rb'(?m)^\s*oauth_token:\s*(\S+)', github.read_bytes()):
                self.add(token)

    def add(self, value):
        if value:
            self.forbidden.extend((value, base64.b64encode(value), base64.b64encode(b'admin:' + value)))

    def check(self, data, label):
        if any(secret in data for secret in self.forbidden):
            raise SystemExit('Personal credential detected in ' + label + '; value withheld')
        high_confidence = rb'(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{30,}|AKIA[0-9A-Z]{16}|-----BEGIN (?:RSA |EC |OPENSSH |ENCRYPTED )?PRIVATE KEY-----)'
        if re.search(high_confidence, data):
            raise SystemExit('Token/private-key pattern detected in ' + label + '; value withheld')
