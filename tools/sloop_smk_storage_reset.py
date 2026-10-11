#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Explicit, backup-checked A-C legacy reset. Never updates firmware. No write retries."""
import argparse
import json
from pathlib import Path
import struct
import time
import zlib
from sloop_smk_storage_diag import MIDIReader, analyze, capture, classify

PREFIX = [125, 83, 77, 75, 2]


def checked_archive(directory):
    directory = Path(directory)
    manifest = analyze(directory)
    return {(r['slot'], r['copy']): (directory / r['files'][0]).read_bytes()
            for r in manifest['copies']}


def tombstone(slot):
    head = struct.pack('<IHH5I', 0x554c4546, slot + 1, 1, 1, 0, 0, 0xffffffff, 0xffffffff)
    return head + struct.pack('<I', zlib.crc32(head)) + b'\xff' * (4096 - 32)


def plan(archived, live, slots):
    if not slots or len(set(slots)) != len(slots) or any(s not in (0, 1, 2) for s in slots):
        raise ValueError('Only distinct slots A-C may be reset; D is excluded')
    # Check the whole archive, including D, before allowing the first mutation.
    if archived != live:
        raise ValueError('Device project data changed since the verified backup; no reset sent')
    for s in slots:
        if classify(live[s, 0], s, 0)['kind'] != 'Felucca SMK':
            raise ValueError('Expected a valid legacy Felucca copy A')
        if classify(live[s, 1], s, 1)['kind'] not in ('Felucca SMK', 'blank'):
            raise ValueError('Copy B contains other data; no reset sent')
    return [(s, zlib.crc32(live[s, 0]), zlib.crc32(live[s, 1])) for s in slots]


def reset_args(slot, a, b):
    if slot not in (0, 1, 2):
        raise ValueError('Slot D cannot be reset')
    return [slot] + [(v >> (7 * i)) & 127 for v in (a, b) for i in range(5)] + list(b'RESET')


class ResetLink:
    def __init__(self, inp, out, mido):
        self.inp, self.out, self.mido = inp, out, mido

    def request(self, op, args):
        if not (op == 0 and args == []) and not (
            op == 1 and len(args) == 16 and args[0] in (0, 1, 2) and
            args[5] < 16 and args[10] < 16 and args[11:] == list(b'RESET')):
            raise ValueError('Invalid explicit-reset request')
        prefix = PREFIX + [op]
        self.out.send(self.mido.Message('sysex', data=prefix + args))
        end = time.monotonic() + 8
        while time.monotonic() < end:
            msg = self.inp.poll()
            if msg and msg.type == 'sysex' and list(msg.data[:6]) == prefix:
                return list(msg.data[6:])
            time.sleep(.003)
        raise TimeoutError('Reset reply missing. Do not repeat: inspect with the read-only backup tool.')


def execute(steps, request, record):
    for slot, a, b in steps:
        record(dict(slot=slot, state='sending', copy_a_crc=a, copy_b_crc=b))
        response = request(1, reset_args(slot, a, b))  # Exactly one send; never retry a mutation.
        record(dict(slot=slot, state='reply', response=response))
        if response != [0, slot]:
            raise ValueError(f'Slot {chr(65 + slot)} reset refused: {response}; stopped')


def verify_after(before, after, slots):
    expected = dict(before)
    for slot in slots:
        expected[slot, 1] = tombstone(slot)
    if after != expected:
        raise ValueError('Post-reset data differs from the expected A-C tombstones; stop and inspect')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--backup', type=Path, required=True)
    parser.add_argument('--directory', type=Path, required=True, help='New private receipt/readback directory')
    parser.add_argument('--input', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--slots', nargs='+', choices=['A', 'B', 'C'], required=True)
    parser.add_argument('--confirm-replace-felucca-projects', action='store_true')
    args = parser.parse_args()
    if not args.confirm_replace_felucca_projects:
        parser.error('Explicit --confirm-replace-felucca-projects is required')
    archived = checked_archive(args.backup)
    slots = [ord(s) - 65 for s in args.slots]
    # Validate archive and selections even before opening MIDI ports.
    plan(archived, archived, slots)
    args.directory.mkdir(parents=True, exist_ok=False)
    def record(row):
        with (args.directory / 'receipt.jsonl').open('a', encoding='utf-8') as f:
            f.write(json.dumps(row) + '\n')
    import mido
    with mido.open_input(args.input) as inp, mido.open_output(args.output) as out:
        reader = MIDIReader(inp, out, mido)
        info = reader.exchange([125, 70, 76, 1], [])
        firmware = bytes(info[:info.index(0)]).decode('ascii')
        if firmware != 'SLOOP SMK 2.6.0-b5':
            raise ValueError('This repair tool requires the tested b5 firmware')
        writer = ResetLink(inp, out, mido)
        if writer.request(0, []) != [0, 1, 3]:
            raise ValueError('Explicit A-C reset capability unavailable')
        capture(reader.request, args.directory / 'before', firmware)
        before = checked_archive(args.directory / 'before')
        steps = plan(archived, before, slots)
        record(dict(firmware=firmware, slots=args.slots, verified_backup=str(args.backup.resolve())))
        execute(steps, writer.request, record)
        capture(reader.request, args.directory / 'after', firmware)
        verify_after(before, checked_archive(args.directory / 'after'), slots)
        record(dict(state='verified', preserved='All copy A sectors, slot D, all unselected sectors'))
    print('Verified: selected slots ready for Sloop. Copy A and D unchanged. Backups and receipt retained.')


if __name__ == '__main__':
    main()
