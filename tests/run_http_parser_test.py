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

esp32=(root/'lib/ChaosUhrESP32WebServer/src/Parsing.cpp').read_text()
helpers=esp32[esp32.index('static char *readBytesWithTimeout'):esp32.index('bool WebServer::_parseRequest')]
request=esp32[esp32.index('bool WebServer::_parseRequest'):esp32.index('bool WebServer::_collectHeader')]
collect=esp32[esp32.index('bool WebServer::_collectHeader'):esp32.index('void WebServer::_parseArguments')]
fixture=(root/'tests/test_esp32_ingress.cpp.in').read_text().replace('@HELPERS@',helpers).replace('@PARSE_REQUEST@',request).replace('@COLLECT_HEADER@',collect)
binary=root/'.pio/host-tests/test_esp32_ingress'
subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-pedantic',
                '-fsanitize=address,undefined','-fno-omit-frame-pointer',
                '-I',str(root/'include'),'-x','c++','-','-o',str(binary)],input=fixture,text=True,check=True)
subprocess.run([str(binary)],check=True)
