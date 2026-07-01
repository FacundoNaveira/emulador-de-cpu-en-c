import socket
import time
import subprocess

p = subprocess.Popen(['./rtm32', '-d', 'telnet'])
time.sleep(1)

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('localhost', 4444))
    
    def read_until_prompt():
        data = b''
        while not data.endswith(b'RTM32> '):
            chunk = s.recv(1024)
            if not chunk: break
            data += chunk
        return data.decode('utf-8', errors='ignore')

    out = read_until_prompt()
    print("PROMPT1:", out)
    
    s.sendall(b'help\r\n')
    out = read_until_prompt()
    print("HELP OUTPUT:")
    print(out)
    
    s.sendall(b'quit\r\n')
except Exception as e:
    print(e)
finally:
    p.kill()
