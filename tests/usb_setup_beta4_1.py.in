"""Local UART first-setup helper. Never prints or stores the setup password."""
import argparse,getpass,re,time
import serial
p=argparse.ArgumentParser(description=__doc__);p.add_argument('port');a=p.parse_args()
code=getpass.getpass('Eigenen zufälligen 12-stelligen Einrichtungscode eingeben (verdeckt): ')
if not re.fullmatch('[0-9]{12}',code):raise SystemExit('Genau zwölf Ziffern erforderlich.')
if getpass.getpass('Code wiederholen: ')!=code:raise SystemExit('Eingaben stimmen nicht überein.')
print('Nur innerhalb fünf Minuten nach Start und vor Admin-Einrichtung verfügbar.')
port=serial.Serial(port=None,baudrate=115200,timeout=1)
port.dtr=False;port.rts=False;port.port=a.port
with port:
    port.reset_input_buffer();port.write(('SETUP '+code+'\n').encode());port.flush()
    code=None
    # Do not print arbitrary UART output: panic dumps may expose RAM secrets.
    deadline=time.monotonic()+5;accepted=False
    while time.monotonic()<deadline:
        if port.readline().strip()==b'USB setup accepted; connect to the ChaosUhr setup AP.':accepted=True;break
    if not accepted:raise SystemExit('Keine Bestätigung. Setupzustand, Port und Fünf-Minuten-Fenster prüfen.')
print('Bestätigt. Mit ChaosUhr-Setup-… und dem eingegebenen Code verbinden; http://192.168.4.1/ öffnen.')
