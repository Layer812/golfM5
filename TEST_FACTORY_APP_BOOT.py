# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Layer812
"""Host-only regression tests for R13 boot APP preflight (never communicates with hardware)."""
import importlib.util
import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path
from unittest.mock import patch

p = Path(__file__).parent / 'flash_app_only.py'
spec = importlib.util.spec_from_file_location('flash_app_only', p)
f = importlib.util.module_from_spec(spec)
spec.loader.exec_module(f)


def table(entries):
    b = bytearray(b'\xff' * 0x1000)
    for i, (name, typ, sub, off, size) in enumerate(entries):
        struct.pack_into('<HBBII16sI', b, i*32, 0x50aa, typ, sub, off, size, name.encode('ascii').ljust(16, b'\x00'), 0)
    return bytes(b)


def data_ota(seqs=(), corrupt=False, state=2):
    b = bytearray(b'\xff' * 0x2000)
    for sector, seq in enumerate(seqs):
        o = sector * 4096
        struct.pack_into('<I', b, o, seq)
        struct.pack_into('<I', b, o + 24, state)
        c = zlib.crc32(b[o:o+4], 0xffffffff) & 0xffffffff
        struct.pack_into('<I', b, o + 28, c ^ int(corrupt))
    return bytes(b)


NV = ('nvs', 1, 2, 0x9000, 0x4000)
PHY = ('phy_init', 1, 1, 0xF000, 0x1000)
OD = ('otadata', 1, 0, 0xD000, 0x2000)
FA = ('app0', 0, 0, 0x10000, 0x300000)
OA = ('app1', 0, 0x10, 0x320000, 0x300000)
OB = ('second', 0, 0x11, 0x620000, 0x300000)


class BootSelectionTests(unittest.TestCase):
    def test_logged_device_factory_app0_no_ota_no_otadata(self):
        found = f.parse_partitions(table([NV, PHY, FA]))
        self.assertEqual(f.find_next_boot(None, found)[0], 'app0')
        self.assertEqual(found['app0'][2:], (0x10000, 0x300000))

    def test_factory_app0_and_erased_otadata(self):
        found = f.parse_partitions(table([NV, OD, PHY, FA]))
        self.assertEqual(f.find_next_boot(data_ota(), found)[0], 'app0')

    def test_factory_default_and_ota_available(self):
        found = f.parse_partitions(table([NV, OD, PHY, FA, OA]))
        self.assertEqual(f.find_next_boot(data_ota(), found)[0], 'app0')
        self.assertEqual(f.find_next_boot(data_ota([1]), found)[0], 'app1')
        self.assertEqual(f.find_next_boot(data_ota([2]), found)[0], 'app1')

    def test_ota_slots_by_subtype_not_label(self):
        found = f.parse_partitions(table([NV, OD, PHY, FA, OA, OB]))
        self.assertEqual(f.find_next_boot(data_ota([1]), found)[0], 'app1')
        self.assertEqual(f.find_next_boot(data_ota([2]), found)[0], 'second')
        self.assertEqual(f.find_next_boot(data_ota([3]), found)[0], 'app1')

    def test_ota_only_erased(self):
        entries = [NV, OD, PHY, ('ota_0',0,0x10,0x20000,0x300000)]
        # Both PHY @ 0xf000 and otadata @ d000 before OTA start.
        found = f.parse_partitions(table(entries))
        self.assertEqual(f.find_next_boot(data_ota(), found)[0], 'ota_0')
        self.assertEqual(f.find_next_boot(data_ota([1]), found)[0], 'ota_0')

    def test_factory_named_factory(self):
        entries = [NV, PHY, ('factory',0,0,0x10000,0x300000)]
        found = f.parse_partitions(table(entries))
        self.assertEqual(f.find_next_boot(None, found)[0], 'factory')

    def test_ota_no_otadata_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'otadata'):
            f.parse_partitions(table([NV, PHY, FA, OA]))

    def test_bad_ota_crc_rejected(self):
        found = f.parse_partitions(table([NV, OD, PHY, FA, OA]))
        with self.assertRaisesRegex(RuntimeError, 'CRC'):
            f.find_next_boot(data_ota([1], corrupt=True), found)

    def test_pending_ota_rejected(self):
        found = f.parse_partitions(table([NV, OD, PHY, FA, OA]))
        with self.assertRaisesRegex(RuntimeError, 'PENDING'):
            f.find_next_boot(data_ota([1], state=1), found)

    def test_no_valid_ota_rejected(self):
        found = f.parse_partitions(table([NV, OD, PHY, FA, OA]))
        b = bytearray(data_ota())
        b[1]=0x80
        with self.assertRaises(RuntimeError):
            f.find_next_boot(bytes(b), found)

    def test_duplicate_factory_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'factory'):
            f.parse_partitions(table([NV, PHY, FA, ('alt',0,0,0x400000,0x300000)]))

    def test_ota_label_subtype_mismatch_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'disagreement'):
            f.parse_partitions(table([NV, OD, PHY, FA, ('ota_1',0,0x10,0x320000,0x300000)]))

    def test_ota_subtype_gap_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'missing/noncontiguous'):
            f.parse_partitions(table([NV, OD, PHY, FA, ('app2',0,0x11,0x320000,0x300000)]))

    def test_app_overlap_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'Overlapping'):
            f.parse_partitions(table([NV, PHY, FA, ('another',1,0x81,0x10000,0x10000)]))

    def test_bad_otadata_type_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'otadata'):
            f.parse_partitions(table([NV, PHY, FA, ('otadata',1,1,0xD000,0x2000)]))

    def test_unknown_app_subtype_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'Unsupported APP'):
            f.parse_partitions(table([NV, PHY, FA, ('test_app',0,0x20,0x400000,0x10000)]))

    def test_unsupported_factory_label_spoof_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'factory label'):
            f.parse_partitions(table([NV, OD, PHY, ('factory', 0, 0x10, 0x20000, 0x300000)]))

    def test_no_app_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'No factory'):
            f.parse_partitions(table([NV, PHY]))

    def test_size_guard_and_image_magic(self):
        with tempfile.TemporaryDirectory() as td:
            path=Path(td)/'firmware.bin'
            h=bytearray(b'\x00'*4096)
            h[0], h[1], h[3]=0xE9, 2, 0x40
            struct.pack_into('<H',h,12,9)
            path.write_bytes(h)
            self.assertEqual(f.check_image(path), 4096)
            h[0]=0
            path.write_bytes(h)
            with self.assertRaisesRegex(RuntimeError,'magic'):
                f.check_image(path)

    def test_main_factory_only_flashes_selected_0x10000_only(self):
        t=table([NV, PHY, FA])
        header=bytes([0xE9])+bytes(31)
        seen=[]
        def dummy_esptool(port,*args):
            seen.append(args)
            if args[0]=='flash_id':
                return 'Chip is ESP32-S3\nDetected flash size: 16MB'
            return 'Hash of data verified.'
        def dummy_read(port,off,size):
            return { (0x8000,0x1000):t, (0x10000,0x20):header }[(off,size)]
        with tempfile.TemporaryDirectory() as td:
            image=Path(td)/'firmware.bin'
            h=bytearray(b'\x00'*614608)
            h[0],h[1],h[3]=0xE9,2,0x40
            struct.pack_into('<H',h,12,9)
            image.write_bytes(h)
            with patch.object(sys,'argv',['flash_app_only.py','COM13',str(image)]), \
                 patch.object(f,'run_esptool',side_effect=dummy_esptool), \
                 patch.object(f,'read',side_effect=dummy_read):
                f.main()
        writes=[args for args in seen if args[0]=='write_flash']
        self.assertEqual(len(writes),1)
        self.assertEqual(writes[0][-2],'0x10000')
        self.assertEqual(writes[0][-1],str(image))

    def test_main_never_flashes_on_missing_app_header(self):
        t=table([NV,PHY,FA])
        seen=[]
        def fake_esptool(port,*args):
            seen.append(args)
            return 'Chip is ESP32-S3\nDetected flash size: 16MB'
        with tempfile.TemporaryDirectory() as td:
            path=Path(td)/'firmware.bin'
            h=bytearray(b'\x00'*4096); h[0],h[1],h[3]=0xe9,2,0x40
            struct.pack_into('<H',h,12,9);path.write_bytes(h)
            def fake_read(port,off,size):
                return t if off==0x8000 else bytes(32)
            with patch.object(sys,'argv',['flash_app_only.py','COM13',str(path)]), \
                 patch.object(f,'run_esptool',side_effect=fake_esptool), \
                 patch.object(f,'read',side_effect=fake_read):
                with self.assertRaisesRegex(RuntimeError, 'image magic'):
                    f.main()
        self.assertFalse(any(x[0]=='write_flash' for x in seen))

if __name__ == '__main__':
    unittest.main(verbosity=2)
