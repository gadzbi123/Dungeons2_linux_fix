#!/usr/bin/env python3
"""Relay private Store requests to Xodus and Microsoft's real licensing API."""
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time

root = Path(sys.argv[1]).resolve()
cli = os.environ.get('XODUS_CLI', 'xodus-cli')
os.umask(0o077)
stop = False
child = None
processed = set()

def shutdown(signum, frame):
    global stop
    stop = True
    if child is not None and child.poll() is None:
        child.terminate()

signal.signal(signal.SIGTERM, shutdown)
signal.signal(signal.SIGINT, shutdown)
while not stop:
    for request in root.glob('*.request'):
        if stop:
            break
        if request.name in processed or not re.fullmatch(r'[0-9a-f]+-[0-9a-f]+\.request', request.name):
            continue
        processed.add(request.name)
        os.chmod(request, 0o600)
        response = request.with_suffix('.response')
        child = subprocess.Popen([cli, 'store-token', str(request), str(response)],
                                 stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            result = child.wait(timeout=90)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait()
            result = 124
        child = None
        if result:
            request.with_suffix('.error').write_text('Microsoft license-token request failed')
        print('Store license request completed: ' + ('success' if result == 0 else f'error {result}'), flush=True)
    time.sleep(0.1)
