"""Decode only executable address candidates from a PRIVATE ESP8266 crash log.
Never prints raw log lines, stack words, config, credentials or invalid addresses.
Keep the original log local; stack dumps can contain secrets.
"""
import argparse,re,subprocess
from pathlib import Path

def candidates(text):
    found=[]
    for line in text.splitlines():
        for value in re.findall(r'\bepc[123]=(0x[0-9a-fA-F]{8})',line):found.append(int(value,16))
        if re.fullmatch(r'\s*[0-9a-fA-F]{8}:\s*(?:[0-9a-fA-F]{8}\s+){3}[0-9a-fA-F]{8}\s*',line):
            found.extend(int(v,16) for v in line.split(':',1)[1].split())
    return list(dict.fromkeys(found))

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--log',required=True,type=Path)
    parser.add_argument('--elf',required=True,type=Path)
    parser.add_argument('--tools',type=Path,default=Path.home()/'.platformio/packages/toolchain-xtensa/bin')
    args=parser.parse_args()
    if args.log.stat().st_size>4*1024*1024:raise SystemExit('Log too large; contents withheld')
    symbols=subprocess.check_output([str(args.tools/'xtensa-lx106-elf-nm'),'-S','--defined-only',str(args.elf)],text=True)
    ranges=[]
    for line in symbols.splitlines():
        m=re.match(r'^([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+[TtWw]\s+',line)
        if m:
            start,size=(int(v,16) for v in m.groups())
            if 0x40100000<=start<0x40300000:ranges.append((start,start+size))
    accepted=[value for value in candidates(args.log.read_text(errors='replace')) if any(a<=value<b for a,b in ranges)]
    print('Executable address candidates only; not a verified unwind. Use the exact firmware ELF.')
    if not accepted:print('No matching executable addresses found; raw log withheld.');return
    result=subprocess.check_output([str(args.tools/'xtensa-lx106-elf-addr2line'),'-a','-f','-C','-e',str(args.elf),*[hex(v) for v in accepted]],text=True)
    print(result,end='')
if __name__=='__main__':main()
