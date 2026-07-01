import telnetlib
import time
import subprocess

p = subprocess.Popen(['./rtm32', '-d', 'telnet'])
time.sleep(1)
try:
    tn = telnetlib.Telnet('localhost', 4444)
    time.sleep(0.5)
    tn.read_very_eager()
    tn.write(b'help\r\n')
    time.sleep(0.5)
    print(tn.read_very_eager().decode('utf-8', errors='ignore'))
    tn.write(b'quit\r\n')
except Exception as e:
    pass
finally:
    p.kill()
