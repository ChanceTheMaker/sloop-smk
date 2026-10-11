# SPDX-License-Identifier: GPL-3.0-only
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

spec = importlib.util.spec_from_file_location('diag', Path(__file__).parents[1] / 'tools/sloop_smk_storage_diag.py')
diag = importlib.util.module_from_spec(spec)
spec.loader.exec_module(diag)


def pack(data):
    out = []
    for off in range(0, len(data), 7):
        group = data[off:off + 7]
        out.append(sum((v >> 7) << i for i, v in enumerate(group)))
        out.extend(v & 127 for v in group)
    return out


def legacy(slot):
    data = bytearray(b'\xff' * 4096)
    payload = bytes(range(256)) * 15
    data[256:] = payload
    struct.pack_into('<7I', data, 0, 0x534d4b39, 1, slot, 7, len(payload), zlib.crc32(payload), 0)
    struct.pack_into('<I', data, 28, zlib.crc32(data[:28]))
    return bytes(data)


class BackupTest(unittest.TestCase):
    def request(self, op, args):
        self.assertIn(op, (0, 1))
        if op == 0:
            return [0, 1, 4, 2, 0, 32, 0, 2]
        slot, copy, low, high, nlow, nhigh = args
        self.assertIn(slot, range(4)); self.assertIn(copy, range(2))
        self.assertEqual((nlow, nhigh), (0, 2))
        data = legacy(slot) if slot < 3 else b'\xff' * 4096
        block = data[low + (high << 7):low + (high << 7) + 256]
        crc = zlib.crc32(block)
        return [0] + args + [(crc >> (7 * i)) & 127 for i in range(5)] + pack(block)

    def test_capture_and_recheck(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'backup'
            result = diag.capture(self.request, path, 'SLOOP SMK test')
            self.assertEqual(len(result['copies']), 8)
            self.assertEqual(diag.analyze(path), result)
            self.assertEqual(result['copies'][0]['classification']['kind'], 'Felucca SMK')
            self.assertEqual(result['copies'][7]['classification']['kind'], 'blank')
            # A damaged local backup is never accepted for later work.
            (path / result['copies'][0]['files'][0]).write_bytes(b'bad')
            with self.assertRaises(ValueError): diag.analyze(path)

    def test_negotiation_and_corruption(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'backup'
            with self.assertRaises(ValueError): diag.capture(lambda *_: [1], path, 'test')
            self.assertFalse(path.exists())
            def damaged(op, args):
                r = self.request(op, args)
                if op: r[-1] ^= 1
                return r
            with self.assertRaises(ValueError): diag.capture(damaged, path, 'test')
            self.assertFalse((path / 'manifest.json').exists())

    def test_different_passes(self):
        calls = 0
        def changing(op, args):
            nonlocal calls
            if not op: return self.request(op, args)
            calls += 1
            if calls <= 16: return self.request(op, args)
            block = bytes(256); crc = zlib.crc32(block)
            return [0] + args + [(crc >> (7 * i)) & 127 for i in range(5)] + pack(block)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'backup'
            with self.assertRaises(ValueError): diag.capture(changing, path, 'test')
            self.assertFalse((path / 'manifest.json').exists())

    def test_classification(self):
        self.assertEqual(diag.classify(legacy(0), 1, 0)['kind'], 'invalid Felucca-like record')
        data = bytearray(legacy(0)); data[300] ^= 1
        self.assertEqual(diag.classify(data, 0, 0)['kind'], 'invalid Felucca-like record')
        data = bytearray(b'\xff' * 4096); payload = b'project'
        data[256:256 + len(payload)] = payload
        struct.pack_into('<IHH5I', data, 0, 0x554c4546, 4, 1, 2, len(payload), zlib.crc32(payload), 0xffffffff, 0xffffffff)
        struct.pack_into('<I', data, 28, zlib.crc32(data[:28]))
        self.assertEqual(diag.classify(data, 3, 1)['kind'], 'Sloop storage record')
        self.assertEqual(diag.classify(bytes(4096), 0, 0)['kind'], 'unrecognized data')

    def test_no_write_commands(self):
        reader = diag.MIDIReader(None, None, None)
        for prefix in ([125, 70, 76, 36], diag.PREFIX + [2], [0x22, 0x24, 0x35, 0x7f]):
            with self.assertRaises(ValueError): reader.exchange(prefix, [])


if __name__ == '__main__':
    unittest.main()
