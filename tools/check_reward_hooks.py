#!/usr/bin/env python3
"""Check UCKS45140 affinity, menu input, and shrine hooks in a patched ISO."""

import argparse
from pathlib import Path
from struct import pack_into, unpack_from

from build_reward_iso import (
    BLOCK, HOOKS, PT_LOAD, PT_PSPREL1, REL, Elf, R_MIPS_26, R_MIPS_HI16,
    R_MIPS_LO16, iso_entry, iso_number, require,
)


MASK = 0xffffffff
HOOK_SITES = {name: address for address, _, name in HOOKS}
TEST_SITES = {
    HOOK_SITES[name] for name in (
        'p3p_community_affinity_hook', 'p3p_config_input_hook', 'p3p_shrine_wait_hook',
        'p3p_shrine_selection_hook',
    )
}


def read_eboot(image):
    with image.open('rb') as source:
        _, entry, _ = iso_entry(source, (b'PSP_GAME', b'SYSDIR', b'EBOOT.BIN'))
        source.seek(iso_number(entry, 2) * BLOCK)
        return source.read(iso_number(entry, 10))


def signed16(word):
    return ((word & 0xffff) ^ 0x8000) - 0x8000


def relocate(elf, base):
    data = bytearray(elf.data)
    table = elf.programs[-1]
    pending_highs = []
    for offset, info in REL.iter_unpack(data[table[1]:table[1] + table[4]]):
        segment, kind = (info >> 8) & 255, info & 255
        if segment != 2 and (segment != 0 or offset not in TEST_SITES):
            continue
        position = elf.programs[segment][1] + offset
        word, = unpack_from('<I', data, position)
        if kind == R_MIPS_HI16:
            pending_highs.append((position, word))
            continue
        if kind == R_MIPS_26:
            word = (word & 0xfc000000) | (((word & 0x3ffffff) + (base >> 2)) & 0x3ffffff)
        elif kind == R_MIPS_LO16:
            for high_position, high_word in pending_highs:
                address = ((high_word & 0xffff) << 16) + signed16(word) + base
                high = ((address + 0x8000) >> 16) & 0xffff
                pack_into('<I', data, high_position, (high_word & 0xffff0000) | high)
            pending_highs.clear()
            word = (word & 0xffff0000) | ((word + base) & 0xffff)
        else:
            raise ValueError(f'Unsupported relocation {kind} at {offset:#x}')
        pack_into('<I', data, position, word)
    require(not pending_highs, 'Unpaired high relocations')
    return data


class HookRunner:
    """Execute the instruction subset used by the hooks, with explicit game-call stubs."""

    def __init__(self, eboot, symbols, base):
        elf = Elf(eboot)
        require(elf.header[1] == 0xffa0, 'Expected a patched PSP EBOOT')
        require([segment[0] for segment in elf.programs] == [PT_LOAD] * 3 + [PT_PSPREL1],
                'Unexpected patched EBOOT segments')
        self.data = relocate(elf, base) if base else elf.data
        self.segments = elf.programs[:3]
        self.base = base
        self.symbols = {name: address + base for name, address in symbols.items()}

    def instruction(self, pc):
        address = pc - self.base
        for segment in self.segments:
            if segment[2] <= address < segment[2] + segment[4]:
                return unpack_from('<I', self.data, segment[1] + address - segment[2])[0]
        raise AssertionError(f'PC outside loaded segments: {pc:#x}')

    def run(self, start, stops, registers, memory, calls=None):
        registers_full = [0] * 32
        for register, value in registers.items():
            registers_full[register] = value
        stop_addresses = {offset + self.base: offset for offset in stops}
        calls = calls or {}
        pc = start + self.base
        branch_target = None
        for _ in range(150):
            if pc in stop_addresses:
                return stop_addresses[pc], registers_full
            if pc in calls:
                pc = calls[pc](registers_full, memory)
                continue

            word = self.instruction(pc)
            op = word >> 26
            rs, rt, rd = (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31
            immediate = word & 0xffff
            displacement = signed16(word)
            left, right = registers_full[rs], registers_full[rt]
            next_pc = pc + 4 if branch_target is None else branch_target
            branch_target = None
            match op:
                case 0:
                    match word & 63:
                        case 0: registers_full[rd] = right << ((word >> 6) & 31)
                        case 8: branch_target = left
                        case 10:
                            if right == 0:
                                registers_full[rd] = left
                        case 33: registers_full[rd] = left + right
                        case 35: registers_full[rd] = left - right
                        case 37: registers_full[rd] = left | right
                        case 38: registers_full[rd] = left ^ right
                        case 43: registers_full[rd] = int(left < right)
                        case _: raise AssertionError(f'Unsupported instruction {word:#010x} at {pc:#x}')
                case 2 | 3:
                    branch_target = ((pc + 4) & 0xf0000000) | ((word & 0x3ffffff) << 2)
                    if op == 3:
                        registers_full[31] = pc + 8
                case 1 | 4 | 5:
                    if op == 1:
                        assert rt == 1, f'Unsupported REGIMM branch at {pc:#x}'
                    take = {1: left < 0x80000000, 4: left == right, 5: left != right}[op]
                    branch_target = pc + 4 + displacement * 4 if take else pc + 8
                case 9: registers_full[rt] = left + displacement
                case 11: registers_full[rt] = int(left < (displacement & MASK))
                case 12: registers_full[rt] = left & immediate
                case 13: registers_full[rt] = left | immediate
                case 14: registers_full[rt] = left ^ immediate
                case 15: registers_full[rt] = immediate << 16
                case 35: registers_full[rt] = memory[(left + displacement) & MASK]
                case 37: registers_full[rt] = memory[(left + displacement) & MASK] & 0xffff
                case 43: memory[(left + displacement) & MASK] = right
                case _: raise AssertionError(f'Unsupported instruction {word:#010x} at {pc:#x}')
            registers_full = [0] + [value & MASK for value in registers_full[1:]]
            pc = next_pc
        raise AssertionError(f'Hook at {start:#x} did not return')


def check_affinity(runner):
    toggle = runner.symbols['p3p_community_affinity_zero']
    start = HOOK_SITES['p3p_community_affinity_hook']
    for threshold, accumulated in ((0, 0), (10, 0), (10, 9), (10, 10), (10, 11),
                                   (65535, 65534), (65535, 65535), (65535, MASK)):
        for enabled in (0, 1):
            memory = {toggle: enabled, 0x100010: threshold}
            before = memory.copy()
            stop, regs = runner.run(start, {0xc1c0, 0xc1d8}, {2: 0x100000, 17: accumulated}, memory)
            blocked = not enabled and accumulated < threshold
            assert stop == (0xc1d8 if blocked else 0xc1c0)
            assert regs[3] == int(blocked)
            assert memory == before and regs[17] == accumulated


def check_menu(runner):
    def game_time_us(registers, _):
        registers[2] = 5000
        return registers[31]

    def push(button):
        memory[0x110004] = button
        return runner.run(
            HOOK_SITES['p3p_config_input_hook'], {0x128858, 0x12888c},
            {2: 0x110000, 17: 0x100000}, memory, calls,
        )[0]

    symbols = runner.symbols
    calls = {symbols['game_time_us']: game_time_us}
    row, state = symbols['menu_row'], symbols['menu_state']
    xp, money = symbols['p3p_xp_mode'], symbols['p3p_money_mode']
    affinity, shrine = symbols['p3p_community_affinity_zero'], symbols['p3p_shrine_wait_one_day']
    selection = symbols['p3p_shrine_card_selection']
    memory = {row: 2, state: 0, affinity: 0, shrine: 0, selection: 0, xp: 0, money: 0, 0x100018: 0}

    assert push(1) == 0x12888c and memory[row] == 0 and memory[state] == 1
    push(0x20)
    assert memory[xp] == 0
    memory[state] = 2
    push(0x10)
    assert memory[row] == 4
    push(0x20)
    assert (memory[selection], memory[shrine]) == (1, 0)
    push(0x10)
    assert memory[row] == 3
    push(0x20)
    assert memory[shrine] == 1
    push(0x10)
    assert memory[row] == 2
    push(0x20)
    assert (memory[affinity], memory[shrine], memory[xp], memory[money]) == (1, 1, 0, 0)
    push(0xa0)
    assert memory[affinity] == 1
    push(0x80)
    assert memory[affinity] == 0
    push(0x40)
    push(0x40)
    push(0x40)
    assert memory[row] == 0
    push(0x80)
    assert memory[xp] == 4
    push(0x40)
    assert memory[row] == 1
    push(0x20)
    assert (memory[money], memory[xp], memory[affinity], memory[shrine]) == (1, 4, 0, 1)
    push(0x50)
    assert memory[row] == 1
    push(0x4000 | 0x20)
    assert memory[state] == 3 and memory[money] == 1
    memory[0x100018] = 1
    assert push(1) == 0x128858


def check_shrine(runner):
    script, context, stack = 0x200000, 0x300000, 0x400000
    result, saved_count = context + 0x110, 0x500000
    start = HOOK_SITES['p3p_shrine_wait_hook']
    default_caller = runner.symbols['game_flow_command_return'] - runner.base
    # Only the signature words read by the guard need to be present in this fixture.
    signature = {
        163: 0x00040007, 351: 0x0009001d, 352: 0x00000008, 355: 0x002e0008,
        354: 0x005e001d, 359: 0x005e001d, 371: 0x005e001d,
    }

    def read_counter(regs, memory):
        assert regs[29] in (stack - 16, stack - 32)
        memory[result] = memory[saved_count]
        return_address = memory[regs[29]]
        regs[2], regs[29] = 1, regs[29] + 16
        return return_address

    def exercise(enabled, index, counter, *, count_id=94, valid_signature=True, caller=default_caller):
        memory = {
            context + 0x124: script, saved_count: counter,
            runner.symbols['p3p_shrine_wait_one_day']: enabled,
        }
        memory.update({script + index * 4: word for index, word in signature.items()})
        if not valid_signature:
            memory[script + 351 * 4] = 0
        if count_id != 94:
            memory[script + (index - 1) * 4] = (count_id << 16) | 29
        link = caller + runner.base
        stop, regs = runner.run(
            start, {caller}, {17: context, 19: index, 29: stack, 31: link}, memory,
            {runner.symbols['game_flow_get_variable_continue']: read_counter},
        )
        assert stop == caller and regs[29] == stack and regs[31] == link and regs[2] == 1
        assert memory[saved_count] == counter
        return memory[result]

    for index in (355, 360, 372):
        for counter in (0, 1, 2, 3, 4, 5, 6, 7, MASK):
            assert exercise(1, index, counter) == (6 if 2 <= counter <= 5 else counter)
            assert exercise(0, index, counter) == counter
        assert exercise(1, index, 2, count_id=93) == 2
        assert exercise(1, index, 2, valid_signature=False) == 2
        assert exercise(1, index, 2, caller=0x172820) == 2
    assert exercise(1, 601, 2) == 2


def check_shrine_selection(runner):
    context, code, stack = 0x300000, 0x200000, 0x400000
    start = HOOK_SITES['p3p_shrine_selection_hook']
    original = runner.symbols['game_flow_random_continue'] - runner.base
    choose = runner.symbols['p3p_shrine_choose_card'] - runner.base
    link = runner.symbols['game_flow_command_return']
    toggle = runner.symbols['p3p_shrine_card_selection']
    signature = {163: 0x00040007, 355: 0x002e0008, 423: 0x50040008, 444: 0x00ff001d}

    def exercise(enabled, index, return_address, words, selected):
        memory = {context + 0x124: code, toggle: enabled}
        memory.update({code + i * 4: word for i, word in words.items()})
        before = memory.copy()
        stop, regs = runner.run(
            start, {original, choose}, {17: context, 19: index, 29: stack, 31: return_address}, memory,
        )
        assert regs[31] == return_address
        if selected:
            assert stop == choose and regs[4] == context and regs[29] == stack
        else:
            assert stop == original and regs[29] == stack - 16
            before[stack - 12] = return_address
        assert memory == before

    exercise(1, 445, link, signature, True)
    exercise(0, 445, link, {}, False)
    exercise(1, 455, link, {}, False)
    exercise(1, 445, link + 4, {}, False)
    for index in signature:
        exercise(1, 445, link, {**signature, index: 0}, False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('iso', type=Path)
    parser.add_argument('--payload', type=Path, default=Path('build/rewards/fusion_hooks.elf'))
    args = parser.parse_args()
    require(__debug__, 'Run without Python optimization so assertions are checked')
    eboot = read_eboot(args.iso)
    payload = Elf(args.payload.read_bytes())
    elf = Elf(eboot)
    embedded = elf.by_name.get('.p3p_rewards')
    require(embedded is not None, 'ISO does not contain a P3P mod payload')
    require(elf.contents(embedded) == payload.contents(payload.by_name['.text']),
            'ISO payload differs from --payload')
    symbols = payload.symbols()
    for base in (0, 0x08804000):
        runner = HookRunner(eboot, symbols, base)
        for check in (check_affinity, check_menu, check_shrine, check_shrine_selection):
            try:
                check(runner)
            except AssertionError as error:
                raise AssertionError(f'{check.__name__} failed at load base {base:#x}') from error
        print(f'Passed affinity, menu input, and shrine checks at load base {base:#010x}')


if __name__ == '__main__':
    main()
