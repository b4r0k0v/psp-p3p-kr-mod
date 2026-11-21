#!/usr/bin/env python3
"""Test a battle reward multiplier once through the PPSSPP debugger."""

import argparse
import base64
from dataclasses import dataclass
import hashlib
import json
import os
import socket
import struct
import time


GAME = ('UCKS45140', '1.00')
MODULE = (0x08804000, 7069440)
RATES = {'AUTO': (1, 1), '1.5': (3, 2), '2': (2, 1), '4': (4, 1), '8': (8, 1)}


@dataclass(frozen=True)
class RewardSite:
    label: str
    address: int
    original: bytes
    register: str
    total_address: int | None
    cap: int
    total_register: str | None = None

    @property
    def marker(self):
        return f'p3p-kr {self.label} test'


SITES = {
    'xp': RewardSite(
        label='protagonist XP', address=0x08A56C4C,
        original=bytes.fromhex('01 00 04 24 52 47 20 0e 21 88 40 00 63 38 20 0e 21 20 22 02'),
        register='v0', total_address=0x08E659A0, cap=0xFFFF,
    ),
    'coin': RewardSite(
        label='Shuffle Time money', address=0x0895A614,
        original=bytes.fromhex('21 10 11 02 7f 96 84 34 48 35 20 0e 2d 20 44 00 21 28 00 02'),
        register='s0', total_address=0x08E6A4C4, cap=9_999_999, total_register='s1',
    ),
}

PARTY_XP_SITES = (
    RewardSite('companion XP', 0x08A56D4C, bytes.fromhex('21 a0 40 00 00 00 a2 8f 01 00 42 24 00 00 a2 af'), 'v0', None, 0xFFFF),
    RewardSite('special companion XP', 0x08A56E30, bytes.fromhex('21 98 40 00 63 00 22 2a 79 00 40 14 00 00 00 00'), 'v0', None, 0xFFFF),
    RewardSite('Persona XP', 0x08A56F70, bytes.fromhex('63 00 62 2a 0a 00 40 14 00 00 00 00 01 00 b5 26'), 's1', None, 0xFFFF),
)
PARTY_DONE = RewardSite('party XP prepared', 0x08A54B78, bytes.fromhex('2c 57 29 0e a8 09 24 8e 05 00 02 24 c9 52 29 0a'), '', None, 0)
PARTY_SITES = (*PARTY_XP_SITES, PARTY_DONE)


class Debugger:
    def __init__(self, host, port):
        self.host = host
        self.port = port

    def __enter__(self):
        self.socket = socket.create_connection((self.host, self.port), timeout=5)
        self.stream = self.socket.makefile('rb')
        try:
            key = base64.b64encode(os.urandom(16)).decode('ascii')
            expected = base64.b64encode(hashlib.sha1(
                (key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode('ascii')
            ).digest())
            self.socket.sendall(
                (f'GET /debugger HTTP/1.1\r\nHost: {self.host}:{self.port}\r\n'
                 'Upgrade: websocket\r\nConnection: Upgrade\r\n'
                 f'Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n').encode('ascii')
            )
            status = self.stream.readline(4096).split()
            headers = {}
            while line := self.stream.readline(4096):
                if line == b'\r\n':
                    break
                name, _, value = line.partition(b':')
                headers[name.lower()] = value.strip()
            if status[1:2] != [b'101'] or headers.get(b'sec-websocket-accept') != expected:
                raise RuntimeError('PPSSPP WebSocket handshake failed')
        except Exception:
            self.__exit__()
            raise
        self.ticket = 0
        return self

    def __exit__(self, *_):
        self.stream.close()
        self.socket.close()

    def send(self, payload, opcode=1):
        length = len(payload)
        if length < 126:
            header = bytes((128 | opcode, 128 | length))
        else:
            header = bytes((128 | opcode, 128 | 126)) + struct.pack('!H', length)
        mask = os.urandom(4)
        self.socket.sendall(header + mask + bytes(value ^ mask[i % 4] for i, value in enumerate(payload)))

    def read_exact(self, size):
        data = self.stream.read(size)
        if len(data) != size:
            raise RuntimeError('PPSSPP closed the WebSocket')
        return data

    def receive(self):
        fragments = bytearray()
        while True:
            flags, length = self.read_exact(2)
            if length & 128:
                raise RuntimeError('Unexpected masked WebSocket response')
            if length == 126:
                length = struct.unpack('!H', self.read_exact(2))[0]
            elif length == 127:
                length = struct.unpack('!Q', self.read_exact(8))[0]
            if length > 1024 * 1024:
                raise RuntimeError('Unexpectedly large WebSocket response')
            payload = self.read_exact(length)
            opcode = flags & 15
            if opcode == 9:
                self.send(payload, opcode=10)
            elif opcode == 8:
                raise RuntimeError('PPSSPP closed the WebSocket')
            elif opcode in (0, 1):
                fragments.extend(payload)
                if flags & 128:
                    return json.loads(fragments)

    def query(self, event, **fields):
        self.ticket += 1
        self.send(json.dumps(dict(event=event, ticket=self.ticket, **fields)).encode())
        while True:
            message = self.receive()
            if message.get('ticket') != self.ticket:
                continue
            if message.get('event') != event:
                raise RuntimeError(f'{event} failed: {message}')
            return message


def scale(reward, mode, limit):
    if reward < 0 or limit < 0:
        raise ValueError('Reward and limit must be nonnegative')
    if mode == 'AUTO':
        return reward
    numerator, denominator = RATES[mode]
    return min(limit, reward * numerator // denominator)


def verify(debugger, site):
    game = debugger.query('game.status').get('game') or {}
    if (game.get('id'), game.get('version')) != GAME:
        raise RuntimeError('Expected the Korean PSP release UCKS45140 v1.00')
    modules = debugger.query('hle.module.list')['modules']
    if not any(m['name'] == 'p3p' and m['isActive'] and (m['address'], m['size']) == MODULE for m in modules):
        raise RuntimeError('Unexpected p3p module range')
    reply = debugger.query('memory.read', address=site.address, size=len(site.original))
    if base64.b64decode(reply['base64'], validate=True) != site.original:
        raise RuntimeError(f'Unexpected original instructions at the {site.label} test site')


def breakpoint(debugger, site):
    return next((p for p in debugger.query('cpu.breakpoint.list')['breakpoints'] if p['address'] == site.address), None)


def remove(debugger, site):
    debugger.query('cpu.breakpoint.remove', address=site.address)
    if breakpoint(debugger, site) is not None:
        raise RuntimeError('Test breakpoint is still present')


def total(debugger, site):
    return debugger.query('memory.read_u32', address=site.total_address)['value']


def arm(debugger, site):
    status = debugger.query('cpu.status')
    if status['stepping'] or status['paused']:
        raise RuntimeError('The game must be running')
    if breakpoint(debugger, site) is not None:
        raise RuntimeError(f'A breakpoint already exists at the {site.label} test site')
    before = total(debugger, site) if site.total_address is not None else None
    debugger.query('cpu.breakpoint.add', address=site.address, enabled=True, log=False, logFormat=site.marker)
    point = breakpoint(debugger, site)
    if not point or not point['enabled'] or point['logFormat'] != site.marker:
        if point and point.get('logFormat') == site.marker:
            remove(debugger, site)
        raise RuntimeError('Could not verify the test breakpoint')
    if before is None:
        print(f'Armed: {site.label}')
    else:
        print(f'Armed. Current {site.label} total: {before}')


def resume(debugger):
    debugger.send(b'{"event":"cpu.resume"}')


def verify_resumed(debugger):
    for _ in range(40):
        if not debugger.query('cpu.status')['stepping']:
            return
        time.sleep(0.025)
    raise RuntimeError('The debugger still reports a paused CPU')


def step_over_reward_instruction(debugger, address):
    # Resume skips breakpoints at its current PC until emulated ticks advance.
    # Advance past this instruction first so a tight loop can hit it again.
    debugger.send(b'{"event":"cpu.stepInto"}')
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline:
        cpu = debugger.query('cpu.status')
        if cpu['stepping'] and cpu['pc'] != address:
            if cpu['pc'] != address + 4:
                raise RuntimeError(f'Unexpected PC after stepping: {cpu["pc"]:#010x}')
            return
        time.sleep(0.025)
    raise RuntimeError('Could not step past the reward instruction')


def finish(debugger, site, mode):
    point = breakpoint(debugger, site)
    if not point or point.get('logFormat') != site.marker:
        raise RuntimeError(f'The {site.label} test breakpoint is missing')
    cpu = debugger.query('cpu.status')
    stopped_here = cpu['stepping'] and cpu['pc'] == site.address and point['hits'] > 0
    if mode is not None and not stopped_here:
        raise RuntimeError('Wait for the test breakpoint or use cancel to remove it')
    if mode is not None:
        original = debugger.query('cpu.getReg', name=site.register)['uintValue']
        if not 0 <= original <= site.cap:
            raise RuntimeError(f'Unexpected original {site.label} reward: {original}')
        before = total(debugger, site)
        limit = site.cap
        if site.total_register:
            register_total = debugger.query('cpu.getReg', name=site.total_register)['uintValue']
            if register_total != before or not 0 <= before <= site.cap:
                raise RuntimeError(f'Unexpected current {site.label} total: {before} / {register_total}')
            limit -= before
        adjusted = scale(original, mode, limit)
        changed = adjusted != original
        try:
            if changed:
                actual = debugger.query('cpu.setReg', name=site.register, value=adjusted)['uintValue']
                if actual != adjusted:
                    raise RuntimeError('Debugger did not apply the expected reward')
            remove(debugger, site)
        except Exception:
            if changed:
                rollback = debugger.query('cpu.setReg', name=site.register, value=original)['uintValue']
                if rollback != original:
                    raise RuntimeError(f'Could not restore {site.register}. The game is still paused.')
            raise
    else:
        remove(debugger, site)
    if stopped_here:
        resume(debugger)
        verify_resumed(debugger)
    if mode is not None:
        print(f'{site.label} total before payout: {before}. Original reward: {original}. Test reward: {adjusted}.')
    else:
        print('Test breakpoint removed.')


def party_points(debugger):
    sites = {site.address: site for site in PARTY_SITES}
    points = debugger.query('cpu.breakpoint.list')['breakpoints']
    return {
        point['address']: (sites[point['address']], point)
        for point in points
        if point['address'] in sites and point.get('logFormat') == sites[point['address']].marker
    }


def clear_party(debugger):
    points = party_points(debugger)
    cpu = debugger.query('cpu.status')
    for site, _ in points.values():
        remove(debugger, site)
    if cpu['stepping'] and cpu['pc'] in points and points[cpu['pc']][1]['hits'] > 0:
        resume(debugger)
        verify_resumed(debugger)


def scale_party_recipient(debugger, site, mode):
    registers = next(category for category in debugger.query('cpu.getAllRegs')['categories'] if category['id'] == 0)
    gpr = dict(zip(registers['registerNames'], registers['uintValues'], strict=True))
    pointer = gpr['s2']
    persona = site.register == 's1'
    start, stride, count = (0x08E66994, 52, 12) if persona else (0x08E6A9AC, 280, 9)
    if not start <= pointer < start + stride * count or (pointer - start) % stride:
        raise RuntimeError(f'Unexpected {site.label} record address: {pointer:#010x}')
    level = gpr['s3' if persona else 's1']
    original = gpr[site.register]
    if not 0 <= original <= site.cap or not 0 <= level <= 99:
        raise RuntimeError(f'Unexpected {site.label} level/reward: {level}/{original}')
    if level == 99:
        print(f'{site.label} at {pointer:#010x} has already reached level 99.', flush=True)
        return
    total_address = pointer + 8
    before = debugger.query('memory.read_u32', address=total_address)['value']
    adjusted = scale(original, mode, min(site.cap, 0x7fffffff - before))
    if adjusted != original:
        try:
            actual = debugger.query('cpu.setReg', name=site.register, value=adjusted)['uintValue']
            if actual != adjusted:
                raise RuntimeError('Debugger did not apply the expected reward')
        except Exception:
            if debugger.query('cpu.setReg', name=site.register, value=original)['uintValue'] != original:
                raise RuntimeError(f'Could not restore {site.register}')
            raise
    print(f'{site.label} at {total_address:#010x}: {original} → {adjusted}, before {before}, expected total {before + adjusted}', flush=True)


def apply_party(debugger, mode):
    points = party_points(debugger)
    cpu = debugger.query('cpu.status')
    if len(points) != len(PARTY_SITES) or not cpu['stepping'] or cpu['pc'] not in points:
        raise RuntimeError('Wait for a party XP test breakpoint or use cancel')
    stepped_pc = None
    try:
        while True:
            site, point = points[cpu['pc']]
            if point['hits'] == 0:
                raise RuntimeError('The party XP breakpoint did not trigger')
            if site == PARTY_DONE:
                break
            scale_party_recipient(debugger, site, mode)
            previous_hits = {address: p['hits'] for address, (_, p) in points.items()}
            stepped_pc = site.address + 4
            step_over_reward_instruction(debugger, site.address)
            resume(debugger)
            stepped_pc = None
            deadline = time.monotonic() + 2
            while time.monotonic() < deadline:
                cpu = debugger.query('cpu.status')
                if cpu['stepping'] and cpu['pc'] in previous_hits:
                    points = party_points(debugger)
                    if points[cpu['pc']][1]['hits'] > previous_hits[cpu['pc']]:
                        break
                time.sleep(0.025)
            else:
                raise RuntimeError('The next party XP breakpoint was not reached')
    finally:
        clear_party(debugger)
        if stepped_pc is not None:
            cpu = debugger.query('cpu.status')
            if cpu['stepping'] and cpu['pc'] == stepped_pc:
                resume(debugger)
                verify_resumed(debugger)
    print('Party XP calculation finished. Test breakpoints removed.')


def run_party(debugger, args):
    for site in PARTY_SITES:
        verify(debugger, site)
    if args.command == 'verify':
        print(f'{GAME[0]} v{GAME[1]} party XP sites verified.')
    elif args.command == 'arm':
        if any(breakpoint(debugger, site) is not None for site in PARTY_SITES):
            raise RuntimeError('A breakpoint already exists at a party XP test site')
        try:
            for site in PARTY_SITES:
                arm(debugger, site)
        except Exception:
            clear_party(debugger)
            raise
    elif args.command == 'cancel':
        clear_party(debugger)
    else:
        apply_party(debugger, args.mode)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    parser.add_argument('--port', required=True, type=int)
    parser.add_argument('--reward', choices=(*SITES, 'party'), default='xp')
    commands = parser.add_subparsers(dest='command', required=True)
    for command in ('verify', 'arm', 'cancel'):
        commands.add_parser(command)
    apply = commands.add_parser('apply')
    apply.add_argument('--mode', choices=RATES, required=True)
    args = parser.parse_args()
    with Debugger(args.host, args.port) as debugger:
        if args.reward == 'party':
            run_party(debugger, args)
            return
        site = SITES[args.reward]
        verify(debugger, site)
        if args.command == 'verify':
            print(f'{GAME[0]} v{GAME[1]} {site.label} site verified. Current total: {total(debugger, site)}')
        elif args.command == 'arm':
            arm(debugger, site)
        else:
            finish(debugger, site, args.mode if args.command == 'apply' else None)


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, ValueError) as error:
        raise SystemExit(str(error)) from error
