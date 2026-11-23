#!/usr/bin/env python3
"""Build a new UCKS45140 v1.00 modded ISO."""

import argparse
from hashlib import sha256
import os
from pathlib import Path
import struct
from tempfile import NamedTemporaryFile


ISO_SHA256 = '09868e5bd1e659cb2b1b5ed761bb17a1ef821bc95fc7be42ec4e4d5f5964645d'
ORIGINAL_BOOT_SHA256 = '54c45a0509c94b4109f5ed1f15954599c9a4b2581cb6791a82003f9613c3e5ae'
DECRYPTED_SHA256 = 'e28202ca7f42ee8a70d78a055cb1ec876f7d5a82a5c1636ab50de2d4c71ab005'
HOOKS = (
    (0x252B8C, (0x00A01021, 0x03E00008), 'p3p_base_xp_hook'),
    (0x252F70, (0x2A620063, 0x1440000A), 'p3p_persona_xp_hook'),
    (0x156614, (0x02111021, 0x3484967F), 'p3p_coin_hook'),
    (0x128850, (0x8C440004, 0x30832000), 'p3p_config_input_hook'),
    (0x126ECC, (0x27BDFF60, 0xAFB20048), 'p3p_config_draw_hook'),
    (0xC1B0, (0x94430010, 0x0223182B), 'p3p_community_affinity_hook'),
    (0x175AE8, (0x27BDFFF0, 0xAFBF0000), 'p3p_shrine_wait_hook'),
    (0x1757D4, (0x27BDFFF0, 0xAFBF0004), 'p3p_shrine_selection_hook'),
    (0x310F9C, (0xAFBF0014, 0xAFB40010), 'p3p_shrine_draw_hook'),
    (0x205C80, (0x00408821, 0x8FA30000), 'p3p_battle_drop_hook'),
    (0x2530AC, (0x27BDFFE0, 0xAFB00000), 'p3p_bench_xp_hook'),
    (0x254DE8, (0x27BDFF60, 0xAFB10094), 'p3p_split_xp_hook'),
)
FUSION_HOOKS = (
    (0x57608, (0x12E00042, 0x0040A021), 'p3p_fusion_pool_hook'),
    (0x4D0AC, (0x27BDFFC0, 0xAFB10034), 'p3p_fusion_input_hook'),
    (0x542A0, (0x02402021, 0x24050010), 'p3p_fusion_help_guide_hook'),
)
PAYLOAD_ADDRESS = 0x6BE000
PT_LOAD, PT_PSPREL1 = 1, 0x700000A0
R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16 = 4, 5, 6
ELF_HEADER = struct.Struct('<16sHHIIIIIHHHHHH')
PROGRAM_HEADER = struct.Struct('<8I')
SECTION_HEADER = struct.Struct('<10I')
SYMBOL = struct.Struct('<IIIBBH')
REL = struct.Struct('<II')
BLOCK = 2048


def require(condition, message):
    if not condition:
        raise ValueError(message)


def align(number, boundary):
    return (number + boundary - 1) & -boundary


def name_at(strings, offset):
    end = strings.index(0, offset)
    return strings[offset:end].decode('ascii')


class Elf:
    def __init__(self, data):
        self.data = data
        self.header = list(ELF_HEADER.unpack_from(data))
        ident, _, machine, _, _, phoff, shoff, _, ehsize, phsize, phnum, shsize, shnum, names = self.header
        require(ident[:7] == b'\x7fELF\x01\x01\x01' and machine == 8, 'Expected little-endian MIPS ELF32')
        require((ehsize, phsize, shsize) == (ELF_HEADER.size, PROGRAM_HEADER.size, SECTION_HEADER.size),
                'Unexpected ELF table entry sizes')
        self.programs = [list(PROGRAM_HEADER.unpack_from(data, phoff + index*phsize)) for index in range(phnum)]
        self.sections = [list(SECTION_HEADER.unpack_from(data, shoff + index*shsize)) for index in range(shnum)]
        self.names = self.contents(self.sections[names])
        self.by_name = {name_at(self.names, row[0]): row for row in self.sections}

    def contents(self, section):
        offset, size = section[4:6]
        require(offset + size <= len(self.data), 'Section extends beyond the ELF file')
        return self.data[offset:offset+size]

    def symbols(self):
        table = self.by_name['.symtab']
        strings = self.contents(self.sections[table[6]])
        return {
            name_at(strings, name): value
            for name, value, *_ in SYMBOL.iter_unpack(self.contents(table))
        }


def patch_eboot(decrypted, payload):
    require(sha256(decrypted).hexdigest() == DECRYPTED_SHA256, 'Unexpected decrypted EBOOT SHA-256')
    game, mod = Elf(decrypted), Elf(payload)
    require(game.header[1] == 0xFFA0, 'Expected a relocatable PSP EBOOT')
    require([row[0] for row in game.programs] == [PT_LOAD, PT_LOAD, PT_PSPREL1], 'Unexpected game segments')
    text, data, relocation_segment = game.programs
    require(text[2] == 0 and align(data[2] + data[5], 0x1000) == PAYLOAD_ADDRESS,
            'The payload would overlap existing game memory')
    require(text[3] == game.by_name['.rodata.sceModuleInfo'][4], 'Unexpected PSP module-info offset')

    old_rel_text = game.by_name['.rel.text']
    stop = old_rel_text[4] + old_rel_text[5]
    existing_relocs = set()
    rel_offset, rel_size = relocation_segment[1], relocation_segment[4]
    require(rel_offset <= old_rel_text[4] <= stop <= rel_offset + rel_size, 'Unexpected relocation section position')
    for offset, info in REL.iter_unpack(decrypted[rel_offset:rel_offset+rel_size]):
        write_segment, target_segment = info >> 8 & 255, info >> 16 & 255
        require(write_segment < 2 and target_segment < 2, 'Unexpected existing relocation segment index')
        if write_segment == 0:
            existing_relocs.add(offset)

    payload_section = mod.by_name['.text']
    payload_code = mod.contents(payload_section)
    require(payload_section[3] == PAYLOAD_ADDRESS and len(payload_code) % 4 == 0, 'Unexpected payload layout')
    symbols = mod.symbols()
    settings = symbols['p3p_reward_settings'] - PAYLOAD_ADDRESS
    setting_symbols = (
        'p3p_xp_mode', 'p3p_money_mode', 'p3p_community_affinity_zero',
        'p3p_shrine_wait_one_day', 'p3p_battle_drop_100',
        'p3p_bench_xp',
    )
    settings_end = settings + 8 + 4*len(setting_symbols)
    require(payload_code[settings:settings_end] == b'P3PRWD01' + bytes(4*len(setting_symbols)),
            'Runtime settings must start at AUTO and Off')
    for row, symbol in enumerate(setting_symbols):
        require(symbols[symbol] == PAYLOAD_ADDRESS + settings + 8 + row*4,
                f'{symbol} is out of menu row order')
    extra_relocs = bytearray()
    patches = []
    hooks = HOOKS + FUSION_HOOKS
    for address, expected, symbol in hooks:
        file_pos = text[1] + address
        require(address + 8 <= text[4] and struct.unpack_from('<2I', decrypted, file_pos) == expected,
                f'Original instructions mismatch at {address:#x}')
        require(address not in existing_relocs and address + 4 not in existing_relocs,
                f'An existing relocation overlaps the hook at {address:#x}')
        target = symbols[symbol]
        require(PAYLOAD_ADDRESS <= target < PAYLOAD_ADDRESS + len(payload_code) and target % 4 == 0,
                f'Invalid hook symbol {symbol}')
        patches.append((file_pos, 0x08000000 | (target >> 2)))
        extra_relocs += REL.pack(address, R_MIPS_26)

    high_relocs = {}
    for address, info in REL.iter_unpack(mod.contents(mod.by_name['.rel.text'])):
        kind = info & 255
        relative = address - PAYLOAD_ADDRESS
        require(kind in (R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16), f'Unsupported payload relocation {kind}')
        require(0 <= relative <= len(payload_code) - 4 and relative % 4 == 0,
                'Payload relocation points outside the payload')
        # Write to the new LOAD at index 2, relocate against the game base at index 0.
        relocation = REL.pack(relative, (2 << 8) | kind)
        symbol_index = info >> 8
        if kind == R_MIPS_HI16:
            high_relocs.setdefault(symbol_index, bytearray()).extend(relocation)
            continue
        if kind == R_MIPS_LO16:
            # PSP relocations have segment indices in place of ELF symbol indices.
            # Keep the highs next to their low before discarding the symbol index.
            extra_relocs += high_relocs.pop(symbol_index, b'')
        extra_relocs += relocation
    require(not high_relocs, 'A payload HI16 has no matching LO16')

    # Leave space for a fourth program header while preserving every load segment's alignment.
    shift = 0x80
    first_file_offset = text[1]
    require(game.header[5] + 4*PROGRAM_HEADER.size <= first_file_offset + shift,
            'No room for the expanded program header table')
    changed = bytearray(decrypted[:first_file_offset] + bytes(shift) + decrypted[first_file_offset:stop]
                        + extra_relocs + decrypted[stop:])
    for offset, jump in patches:
        struct.pack_into('<2I', changed, offset + shift, jump, 0)

    payload_offset = align(len(changed), 0x80)
    changed.extend(bytes(payload_offset - len(changed)))
    changed.extend(payload_code)
    strings_offset = len(changed)
    section_name = len(game.names)
    names = game.names + b'.p3p_rewards\x00'
    changed.extend(names)
    table_offset = align(len(changed), 4)
    changed.extend(bytes(table_offset - len(changed)))

    game.header[6] = table_offset
    game.header[10] += 1
    game.header[12] += 1
    ELF_HEADER.pack_into(changed, 0, *game.header)

    text[1] += shift
    text[3] += shift  # PSP stores the module-info file offset in the first p_paddr.
    data[1] += shift
    relocation_segment[1] += shift
    relocation_segment[4] += len(extra_relocs)
    new_segment = [PT_LOAD, payload_offset, PAYLOAD_ADDRESS, 0, len(payload_code), len(payload_code), 7, 0x80]
    for index, row in enumerate((text, data, new_segment, relocation_segment)):
        PROGRAM_HEADER.pack_into(changed, game.header[5] + index*PROGRAM_HEADER.size, *row)

    for index, row in enumerate(game.sections):
        if index == game.header[13]:
            row[4:6] = [strings_offset, len(names)]
        elif row[4] >= first_file_offset:
            row[4] += shift + (len(extra_relocs) if row[4] >= stop else 0)
        if row is old_rel_text:
            row[5] += len(extra_relocs)
        changed.extend(SECTION_HEADER.pack(*row))
    changed.extend(SECTION_HEADER.pack(section_name, 1, 7, PAYLOAD_ADDRESS, payload_offset, len(payload_code), 0, 0, 4, 0))

    built = Elf(changed)
    require(built.contents(built.by_name['.p3p_rewards']) == payload_code, 'Payload section verification failed')
    require(built.contents(built.by_name['.rel.text']).endswith(extra_relocs), 'Relocation verification failed')
    require(built.contents(built.by_name['.rodata.sceModuleInfo']) == game.data[text[3]-shift:text[3]-shift+52],
            'PSP module-info verification failed')
    for offset, jump in patches:
        require(struct.unpack_from('<2I', changed, offset + shift) == (jump, 0), 'Hook verification failed')
    print(f'Runtime settings at module offset {symbols["p3p_reward_settings"]:#010x}')
    return changed


def both_endian(value):
    return struct.pack('<I', value) + struct.pack('>I', value)


def iso_number(record, offset):
    value = struct.unpack_from('<I', record, offset)[0]
    require(record[offset:offset+8] == both_endian(value), 'Inconsistent ISO9660 number')
    return value


def iso_entry(iso, path):
    iso.seek(16*BLOCK)
    primary = iso.read(BLOCK)
    require(primary[:7] == b'\x01CD001\x01' and primary[128:132] == b'\x00\x08\x08\x00',
            'Unexpected ISO9660 primary descriptor')
    record = primary[156:190]
    for component in path:
        require(record[25] & 2, 'Expected an ISO9660 directory')
        base = iso_number(record, 2)*BLOCK
        size = iso_number(record, 10)
        iso.seek(base)
        directory = iso.read(size)
        require(len(directory) == size, 'Truncated ISO9660 directory')
        offset = 0
        while offset < size:
            length = directory[offset]
            if length == 0:
                offset = align(offset + 1, BLOCK)
                continue
            record = directory[offset:offset+length]
            require(len(record) >= 34 and len(record) >= 33+record[32], 'Truncated ISO9660 directory record')
            name = record[33:33+record[32]].split(b';', 1)[0]
            if name == component:
                break
            offset += length
        else:
            raise ValueError(f'ISO9660 entry not found: {component!r}')
    return base + offset, record, primary


def build_iso(source, target, eboot):
    target.parent.mkdir(parents=True, exist_ok=True)
    boot_path = (b'PSP_GAME', b'SYSDIR', b'EBOOT.BIN')
    with source.open('rb') as iso:
        entry_pos, entry, primary = iso_entry(iso, boot_path)
        require(entry[25] & 0x82 == 0, 'Expected a single-extent EBOOT.BIN')
        sectors = iso_number(primary, 80)
        require(source.stat().st_size == sectors*BLOCK, 'Unexpected original ISO size')
        iso.seek(17*BLOCK)
        require(iso.read(7) == b'\xffCD001\x01', 'Unexpected additional ISO9660 volume descriptor')
        iso.seek(iso_number(entry, 2)*BLOCK)
        require(sha256(iso.read(iso_number(entry, 10))).hexdigest() == ORIGINAL_BOOT_SHA256,
                'Unexpected encrypted EBOOT content')
        iso.seek(0)
        digest = sha256()
        temp = None
        try:
            with NamedTemporaryFile(dir=target.parent, suffix='.iso', delete=False) as output:
                temp = Path(output.name)
                while chunk := iso.read(8*1024*1024):
                    digest.update(chunk)
                    output.write(chunk)
                require(digest.hexdigest() == ISO_SHA256 and output.tell() == sectors*BLOCK,
                        'Unexpected original ISO SHA-256')
                output.write(eboot)
                extent_size = align(len(eboot), BLOCK)
                output.write(bytes(extent_size - len(eboot)))
                output.seek(entry_pos+2)
                output.write(both_endian(sectors) + both_endian(len(eboot)))
                output.seek(16*BLOCK+80)
                output.write(both_endian(sectors + extent_size//BLOCK))
            with temp.open('rb') as result:
                _, record, primary = iso_entry(result, boot_path)
                require(iso_number(record, 2) == sectors and iso_number(record, 10) == len(eboot),
                        'Patched EBOOT directory entry failed verification')
                require(iso_number(primary, 80)*BLOCK == temp.stat().st_size, 'Patched ISO size mismatch')
                result.seek(sectors*BLOCK)
                require(result.read(len(eboot)) == eboot, 'Patched EBOOT content mismatch')
            os.link(temp, target)  # Publish only the verified result and never overwrite.
        finally:
            if temp is not None:
                temp.unlink()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--decrypted', type=Path, required=True)
    parser.add_argument('--payload', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error(f'output already exists: {args.output}')
    try:
        payload = args.payload or Path('build/rewards/fusion_hooks.elf')
        eboot = patch_eboot(args.decrypted.read_bytes(), payload.read_bytes())
        build_iso(args.original, args.output, eboot)
    except (OSError, ValueError, KeyError, struct.error) as error:
        raise SystemExit(str(error)) from error
    print(f'Patched ISO: {args.output}')


if __name__ == '__main__':
    main()
