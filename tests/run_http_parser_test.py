"""Compile the vendored production parser itself; do not substitute a parser."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
source = (root / 'lib/ChaosUhrWebServer/src/Parsing-impl.h').read_text()
source = re.sub(r'^#include[^\n]*\n', '', source, flags=re.M)
fixture = (root / 'tests/test_http_parser.cpp.in').read_text().replace('@PARSER@', source)
binary = root / '.pio/host-tests/test_http_parser'
binary.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pedantic',
                '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                '-I', str(root / 'include'),
                '-x', 'c++', '-', '-o', str(binary)], input=fixture, text=True, check=True)
subprocess.run([str(binary)], check=True)
