#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only SMK project-pair backup and offline classification. Never writes the device."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import zlib

PREFIX = [0x7D, 0x53, 0x4D, 0x4B, 1]


def u7(data):
    return sum(v << (7 * i) for i, v in enumerate(data))


def unpack(data):
    result = bytearray()
    for pos in range(0, len(data), 8):
        group = data[pos:pos + 8]
        if len(group) < 2 or any(not 0 <= v < 128 for v in group):
            raise ValueError('Malformed packed data')
        result.extend(v | (((group[0] >> i) & 1) << 7) for i, v in enumerate(group[1:]))
    return bytes(result)


def classify(data, slot, copy):
    if len(data) != 4096:
        raise ValueError('Expected one full project sector')
    if data == b'\xff' * 4096:
        return dict(kind='blank')
    magic = struct.unpack_from('<I', data)[0]
    if magic == 0x534D4B39:
        _, version, stored_slot, seq, size, crc, reserved, hcrc = struct.unpack_from('<8I', data)
        off = 256 if version == 1 else 128
        valid = (version in (1, 2) and stored_slot == slot and reserved == 0 and
                 0 < size <= 4096 - off and zlib.crc32(data[:28]) == hcrc and
                 zlib.crc32(data[off:off + size]) == crc)
        return dict(kind='Felucca SMK' if valid else 'invalid Felucca-like record', sequence=seq, length=size)
    if magic == 0x554C4546:
        _, obj, stored_copy, seq, size, crc, _, _, hcrc = struct.unpack_from('<IHH6I', data)
        valid = (obj == slot + 1 and stored_copy == copy and size <= 3840 and
                 zlib.crc32(data[:28]) == hcrc and zlib.crc32(data[256:256 + size]) == crc)
        return dict(kind='Sloop storage record' if valid else 'invalid Sloop-like record', sequence=seq, length=size)
    return dict(kind='unrecognized data', header_hex=data[:32].hex())


def capture(request, directory, firmware):
    # Negotiate the separate namespace before requesting any data.
    caps = request(0, [])
    if caps != [0, 1, 4, 2, 0, 32, 0, 2]:
        raise ValueError('Unsupported or unavailable read-only diagnostic capability')
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=False)
    manifest = dict(format='sloop-smk-project-pairs-v1', firmware=firmware,
                    read_only=True, copies=[])
    for slot in range(4):
        for copy in range(2):
            passes = []
            for attempt in range(2):
                data = bytearray()
                for off in range(0, 4096, 256):
                    args = [slot, copy, off & 127, off >> 7, 0, 2]
                    r = request(1, args)
                    if len(r) < 12 or r[0] != 0 or r[1:7] != args:
                        raise ValueError(f'Read refused/mismatched: slot {slot + 1}, copy {copy}, offset {off}: {r[:7]}')
                    block = unpack(r[12:])
                    if len(block) != 256 or zlib.crc32(block) != u7(r[7:12]):
                        raise ValueError('Read checksum or length mismatch')
                    data.extend(block)
                name = f'slot-{slot + 1}-copy-{copy}-pass-{attempt + 1}.bin'
                with (directory / name).open('xb') as output:
                    output.write(data)
                passes.append(bytes(data))
            if passes[0] != passes[1]:
                raise ValueError('Flash changed between reads; partial captures kept, no verified manifest written')
            manifest['copies'].append(dict(slot=slot, copy=copy,
                files=[f'slot-{slot + 1}-copy-{copy}-pass-{i}.bin' for i in (1, 2)],
                sha256=hashlib.sha256(passes[0]).hexdigest(), classification=classify(passes[0], slot, copy)))
    with (directory / 'manifest.json').open('x', encoding='utf-8') as output:
        json.dump(manifest, output, indent=2)
    return manifest


def analyze(directory):
    directory = Path(directory)
    manifest = json.loads((directory / 'manifest.json').read_text(encoding='utf-8'))
    if manifest.get('format') != 'sloop-smk-project-pairs-v1' or manifest.get('read_only') is not True:
        raise ValueError('Not a verified project-pair archive')
    if len(manifest['copies']) != 8:
        raise ValueError('Incomplete archive')
    seen = set()
    for row in manifest['copies']:
        slot, copy = row['slot'], row['copy']
        if slot not in range(4) or copy not in range(2) or (slot, copy) in seen:
            raise ValueError('Invalid/duplicate slot pair')
        seen.add((slot, copy))
        expected = [f'slot-{slot + 1}-copy-{copy}-pass-{i}.bin' for i in (1, 2)]
        if row['files'] != expected:
            raise ValueError('Unexpected archive path')
        passes = [(directory / name).read_bytes() for name in expected]
        if passes[0] != passes[1] or hashlib.sha256(passes[0]).hexdigest() != row['sha256']:
            raise ValueError('Archive mismatch')
        row['classification'] = classify(passes[0], slot, copy)
    return manifest


class MIDIReader:
    def __init__(self, inp, out, mido):
        self.inp, self.out, self.mido = inp, out, mido

    def exchange(self, prefix, args):
        # Only INFO and diagnostic discovery/read requests are permitted.
        if prefix != [125, 70, 76, 1] and prefix not in [PREFIX + [0], PREFIX + [1]]:
            raise ValueError('Command is not read-only')
        for _ in range(2):
            self.out.send(self.mido.Message('sysex', data=prefix + args))
            end = time.monotonic() + 3
            while time.monotonic() < end:
                msg = self.inp.poll()
                if msg and msg.type == 'sysex' and list(msg.data[:len(prefix)]) == prefix:
                    return list(msg.data[len(prefix):])
                time.sleep(.003)
        raise TimeoutError('No diagnostic reply; check firmware and close other MIDI apps')

    def request(self, op, args):
        return self.exchange(PREFIX + [op], args)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    cap = sub.add_parser('capture')
    cap.add_argument('--input', required=True)
    cap.add_argument('--output', required=True)
    cap.add_argument('--directory', type=Path, required=True)
    check = sub.add_parser('analyze')
    check.add_argument('directory', type=Path)
    args = parser.parse_args()
    if args.command == 'analyze':
        manifest = analyze(args.directory)
    else:
        import mido
        with mido.open_input(args.input) as inp, mido.open_output(args.output) as out:
            client = MIDIReader(inp, out, mido)
            info = client.exchange([125, 70, 76, 1], [])
            firmware = bytes(info[:info.index(0)]).decode('ascii')
            if not firmware.startswith('SLOOP SMK '):
                raise ValueError('Unexpected device firmware')
            manifest = capture(client.request, args.directory, firmware)
    for row in manifest['copies']:
        print(f"Slot {row['slot'] + 1}, copy {row['copy']}: {row['classification']['kind']}")
    print('Verified two matching reads per sector. No device writes were sent.')


if __name__ == '__main__':
    main()
