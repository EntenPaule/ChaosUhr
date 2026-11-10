"""Offline UART simulation: opening reset, lost first request/ACK and closed setup."""
import contextlib,io,runpy,sys,types,subprocess
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
CODE='123456789012'
ACK=b'USB setup accepted; connect to the ChaosUhr setup AP.'
class Link:
    def __init__(self,boot=3,closed=False,fail=False,drop_ack=False,silent=False):
        self.now=0.;self.boot=boot;self.closed=closed;self.fail=fail;self.drop_ack=drop_ack;self.silent=silent;self.queue=[];self.commands=0;self.ready_at=None
    def clock(self):return self.now
    def reset_input_buffer(self):self.queue=[]
    def write(self,data):
        if self.now<self.boot or self.silent:return len(data)
        if data.strip()==b'STATUS':
            self.ready_at=self.now;self.queue.append(b'CHAOS SETUP CLOSED' if self.closed else b'CHAOS SETUP READY 1')
        elif data.strip()==('SETUP '+CODE).encode():
            self.commands+=1
            if self.fail:self.queue.append(b'CHAOS SETUP AP_FAILED')
            elif not self.drop_ack or self.commands>1:self.queue.append(ACK)
        else:raise AssertionError('Unexpected command')
        return len(data)
    def flush(self):pass
    def readline(self,*args):
        self.now+=.25
        return (self.queue.pop(0)+b'\r\n') if self.queue else b'boot/status noise\n'
    def __enter__(self):assert self.port=='TEST_PORT' and not self.dtr and not self.rts;return self
    def __exit__(self,*args):pass
with patch.dict(sys.modules,{'serial':types.SimpleNamespace()}):
    module=runpy.run_path(str(ROOT/'scripts/esp32_usb_setup.py'))
for options,success in [({},True),({'boot':7},True),({'drop_ack':True},True),({'closed':True},False),({'fail':True},False),({'silent':True},False)]:
    link=Link(**options);output=io.StringIO()
    with contextlib.redirect_stdout(output):
        try:module['provision'](link,CODE,link.clock)
        except SystemExit:assert not success
        else:assert success
    assert CODE not in output.getvalue()
    if success:assert link.commands and link.ready_at>=link.boot
    if options.get('silent') or options.get('closed'):assert link.commands==0
# Reproduce beta.4.1 failure on the exact old helper, with a reset/3s boot on open.
old=(ROOT/'tests/usb_setup_beta4_1.py.in').read_text()
link=Link(boot=3)
def factory(**kwargs):
    assert kwargs['baudrate']==115200;link.port=None;link.dtr=True;link.rts=True;return link
with patch.dict(sys.modules,{'serial':types.SimpleNamespace(Serial=factory)}),patch.object(sys,'argv',['helper','TEST_PORT']),patch('getpass.getpass',return_value=CODE),patch('time.monotonic',side_effect=link.clock),contextlib.redirect_stdout(io.StringIO()):
    try:exec(compile(old,'beta4.1-helper','exec'),{'__name__':'__main__'})
    except SystemExit:pass
    else:raise AssertionError('Old single send unexpectedly succeeded')
assert link.commands==0
# Wrong input must never instantiate/open a serial port.
with patch.dict(sys.modules,{'serial':types.SimpleNamespace(Serial=lambda **kw:(_ for _ in ()).throw(AssertionError('port opened')))}),patch.object(sys,'argv',['helper','TEST_PORT']),patch('getpass.getpass',return_value='invalid'),contextlib.redirect_stdout(io.StringIO()):
    try:runpy.run_path(str(ROOT/'scripts/esp32_usb_setup.py'),run_name='__main__')
    except SystemExit:pass
    else:raise AssertionError('Invalid input accepted')
print('PASS UART: exact beta.4.1 boot race reproduced; readiness, delayed boot, lost ACK, closed/AP-failed/silent paths, no early secret send, invalid input without port access')
# Exercise actual main() including pyserial options and inactive lines before open.
link=Link(boot=2)
def new_factory(**kwargs):
    assert kwargs==dict(port=None,baudrate=115200,timeout=.25,write_timeout=2,exclusive=True)
    link.port=None;link.dtr=True;link.rts=True;return link
output=io.StringIO()
with patch.dict(sys.modules,{'serial':types.SimpleNamespace(Serial=new_factory,SerialException=OSError)}),patch.object(sys,'argv',['helper','TEST_PORT']),patch('getpass.getpass',return_value=CODE),patch('time.monotonic',side_effect=link.clock),contextlib.redirect_stdout(output):
    runpy.run_path(str(ROOT/'scripts/esp32_usb_setup.py'),run_name='__main__')
assert 'Bestätigt.' in output.getvalue() and CODE not in output.getvalue() and link.commands==1
print('PASS UART main: inactive DTR/RTS, 115200, exclusive access, bounded IO, boot handshake and secret-free success output')
