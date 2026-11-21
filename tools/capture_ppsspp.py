#!/usr/bin/env python3
"""Save the current PPSSPP framebuffer as a PNG through the remote debugger."""

import argparse
import base64
from pathlib import Path
import time

from test_battle_rewards import Debugger, resume, verify_resumed


def capture(debugger):
    was_running = not debugger.query('cpu.status')['stepping']
    try:
        if was_running:
            debugger.send(b'{"event":"cpu.stepping","enabled":true}')
            for _ in range(40):
                if debugger.query('cpu.status')['stepping']:
                    break
                time.sleep(0.025)
            else:
                raise RuntimeError('PPSSPP did not pause for the screenshot')
        uri = debugger.query('gpu.buffer.screenshot')['uri']
        prefix, _, encoded = uri.partition(',')
        if prefix != 'data:image/png;base64':
            raise ValueError('Unexpected screenshot format')
        image = base64.b64decode(encoded, validate=True)
        if not image.startswith(b'\x89PNG\r\n\x1a\n'):
            raise ValueError('Debugger response did not contain a PNG')
        return image
    finally:
        if was_running:
            resume(debugger)
            verify_resumed(debugger)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    parser.add_argument('--port', required=True, type=int)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    try:
        with Debugger(args.host, args.port) as debugger:
            image = capture(debugger)
        with args.output.open('xb') as output:
            output.write(image)
    except (OSError, RuntimeError, ValueError, KeyError) as error:
        parser.exit(1, f'{error}\n')
    print(args.output)


if __name__ == '__main__':
    main()
