"""Exercise helper without opening any real port or printing a secret."""
import contextlib,io,runpy,sys,types
from pathlib import Path
from unittest.mock import patch
class FakePort:
    def __init__(self,port,baudrate,timeout):
        assert port is None and baudrate==115200 and timeout==1
        self.port=None;self.dtr=True;self.rts=True;self.written=False
    def __enter__(self):
        assert self.port=='TEST_PORT' and not self.dtr and not self.rts
        return self
    def __exit__(self,*args):pass
    def reset_input_buffer(self):pass
    def write(self,data):assert data==b'SETUP 123456789012\n';self.written=True
    def flush(self):assert self.written
    def readline(self):return b'USB setup accepted; connect to the ChaosUhr setup AP.\n'
root=Path(__file__).resolve().parents[1]
for answers,success in [(['123456789012']*2,True),(['invalid'],False),(['123456789012','000000000000'],False)]:
    output=io.StringIO()
    with patch.dict(sys.modules,{'serial':types.SimpleNamespace(Serial=FakePort)}),patch.object(sys,'argv',['helper','TEST_PORT']),patch('getpass.getpass',side_effect=answers),contextlib.redirect_stdout(output):
        try:runpy.run_path(str(root/'scripts/esp32_usb_setup.py'),run_name='__main__')
        except SystemExit:assert not success
        else:assert success
    assert '123456789012' not in output.getvalue()
print('PASS USB helper: port configured before opening, hidden input/confirmation, no secret output; mocked UART only')
