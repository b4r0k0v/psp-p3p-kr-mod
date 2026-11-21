#!/usr/bin/env python3
"""Find UCKS45140 font-encoded text in a local binary."""

import argparse
from itertools import islice
from pathlib import Path
import mmap
import re


ASCII_LIMIT = 0x7f
EXTENDED_OFFSET = 32


class Charset:
    def __init__(self, path):
        self.characters = [
            character
            for row in path.read_text(encoding='utf-8').splitlines()
            for character in row.split('\t')
        ]
        self.indices = {}
        for index, character in enumerate(self.characters):
            self.indices.setdefault(character, index)
        self.indices[' '] = 32

    def encode(self, text):
        data = bytearray()
        for character in text:
            index = self.indices.get(character)
            if index is None:
                raise ValueError(f'Character missing from the table: {character!r}')
            if index < ASCII_LIMIT:
                data.append(index)
            else:
                # The extended codes omit the 32 control slots in the TSV.
                high, low = divmod(index - EXTENDED_OFFSET, 128)
                data.extend((128 + high, 128 + low))
        return bytes(data)

    def decode(self, data):
        characters = []
        source = iter(data)
        for first in source:
            if first < 32:
                characters.append(f'<{first:02x}>')
                continue
            index = first
            if first >= 128:
                second = next(source, None)
                if second is None or second < 128:
                    characters.append('�')
                    continue
                index = (first - 128) * 128 + second - 128 + EXTENDED_OFFSET
                if index < ASCII_LIMIT:
                    characters.append('�')
                    continue
            elif first == ASCII_LIMIT:
                characters.append('<7f>')
                continue
            if index == 32:
                characters.append(' ')
            else:
                characters.append(self.characters[index] if index < len(self.characters) else '�')
        return ''.join(characters)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--charset', required=True, type=Path, help='tab-separated local P3P Korean font table')
    parser.add_argument('binary', type=Path)
    parser.add_argument('text', nargs='+')
    args = parser.parse_args()
    try:
        charset = Charset(args.charset)
        queries = [(text, charset.encode(text)) for text in args.text]
        if any(not encoded for _, encoded in queries):
            raise ValueError('Search text must not be empty')
        with args.binary.open('rb') as source, mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
            for text, encoded in queries:
                matches = re.finditer(re.escape(encoded), data)
                first_matches = list(islice(matches, 25))
                count = len(first_matches) + sum(1 for _ in matches)
                print(f'{text} [{encoded.hex(" ")}] {count} match(es)')
                for match in first_matches:
                    offset = match.start()
                    context = data[offset:offset + 160].split(b'\0', 1)[0]
                    print(f'{offset:08x} {charset.decode(context)}')
    except (OSError, ValueError) as error:
        parser.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
