"""Local, bounded UART onboarding. No raw logs or secrets are printed/stored."""
import argparse,getpass,re,time
import serial
ACK=b'USB setup accepted; connect to the ChaosUhr setup AP.'
def provision(port,code,clock=time.monotonic):
    # Opening a USB-UART adapter can glitch reset lines despite inactive DTR/RTS.
    # Query the running firmware before sending credentials. Never actively reset.
    port.reset_input_buffer()
    deadline=clock()+20;next_send=0;ready=False
    while clock()<deadline:
        if clock()>=next_send:
            port.write(b'\nSTATUS\n');port.flush();next_send=clock()+1
        line=port.readline(256).strip()
        if line==b'CHAOS SETUP CLOSED':raise SystemExit('Setup gesperrt: Admin eingerichtet, Reset/Sperrzustand oder Fünf-Minuten-Fenster abgelaufen.')
        if line==b'CHAOS SETUP READY 1':ready=True;break
    if not ready:raise SystemExit('Keine Firmware-Bereitschaft. Beta.4.2, Port, exklusiven Portzugriff und Stromversorgung prüfen. Kein Code gesendet.')
    print('Firmware bereit; Einrichtungscode wird übertragen.')
    deadline=clock()+8;next_send=0
    while clock()<deadline:
        if clock()>=next_send:
            port.write(('\nSETUP '+code+'\n').encode());port.flush();next_send=clock()+1
        line=port.readline(256).strip()
        if line==ACK:return
        if line==b'CHAOS SETUP CLOSED':raise SystemExit('Setupfenster inzwischen geschlossen; keine weitere Übertragung.')
        if line==b'CHAOS SETUP AP_FAILED':raise SystemExit('Firmware erreichbar, aber AP-Konfiguration nicht bestätigt. Kein Erfolgsnachweis.')
    raise SystemExit('Firmware war bereit, aber kein Erfolgs-ACK erhalten. Codeübernahme nicht bestätigt.')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('port');a=p.parse_args()
    code=getpass.getpass('Eigenen zufälligen 12-stelligen Einrichtungscode eingeben (verdeckt): ')
    if not re.fullmatch('[0-9]{12}',code):raise SystemExit('Genau zwölf Ziffern erforderlich.')
    if getpass.getpass('Code wiederholen: ')!=code:raise SystemExit('Eingaben stimmen nicht überein.')
    print('Nur innerhalb fünf Minuten nach Start und vor Admin-Einrichtung verfügbar.')
    port=serial.Serial(port=None,baudrate=115200,timeout=0.25,write_timeout=2,exclusive=True)
    port.dtr=False;port.rts=False;port.port=a.port
    try:
        with port:provision(port,code)
    except (serial.SerialException,OSError):
        raise SystemExit('Serieller Zugriff fehlgeschlagen. Port/Berechtigung prüfen und andere serielle Programme schließen.') from None
    finally:code=None
    print('Bestätigt. Mit ChaosUhr-Setup-… und dem eingegebenen Code verbinden; http://192.168.4.1/ öffnen.')
if __name__=='__main__':main()
