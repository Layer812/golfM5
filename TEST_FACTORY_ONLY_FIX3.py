# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Layer812
"""Regression tests: R13 FIX3 factory-only layout and OTA branch safety."""
import importlib.util
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

spec=importlib.util.spec_from_file_location('fl', Path(__file__).parent/'flash_app_only.py')
f=importlib.util.module_from_spec(spec);spec.loader.exec_module(f)

def partition_table(entries):
    data=bytearray(b'\xff'*4096)
    for i,(name,typ,sub,off,cap) in enumerate(entries):
        struct.pack_into('<HBBII16sI',data,i*32,0x50aa,typ,sub,off,cap,
                         name.encode().ljust(16,b'\0'),0)
    return bytes(data)

NVS=('nvs',1,2,0x9000,0x5000)
OTAD=('otadata',1,0,0xE000,0x2000)
FACTORY=('app0',0,0,0x10000,0x300000)
SPIFFS=('spiffs',1,0x82,0x310000,0xCF0000)
# Separate non-overlapping layout when OTA slots exist.
OTA0=('ota_0',0,0x10,0x320000,0x300000)

class Fix3Tests(unittest.TestCase):
    def setUp(self):
        self.actual=f.parse_partitions(partition_table([NVS,OTAD,FACTORY,SPIFFS]))

    def test_actual_partition_layout(self):
        self.assertEqual(self.actual['app0'],(0,0,0x10000,0x300000))
        self.assertEqual(f.find_next_boot(None,self.actual)[0],'app0')

    def test_factory_only_ignores_erased_stale_otadata(self):
        blank=b'\xff'*0x2000
        bad=bytearray(blank)
        bad[4096:4100]=b'\x00\x00\x00\x00'
        invalid=bytearray(blank)
        invalid[4096+24:4096+28]=b'\x02\x00\x00\x00'
        for blob in [blank,bytes(bad),bytes(invalid),b'\x00'*0x2000]:
            self.assertEqual(f.find_next_boot(blob,self.actual)[0],'app0')

    def test_real_main_skips_unused_ota_read_and_writes_only_factory(self):
        seen=[]; reads=[]
        def fake_esptool(port,*args):
            seen.append(args)
            if args[0]=='flash_id': return 'Chip is ESP32-S3\nDetected flash size: 16MB'
            return 'Hash of data verified.'
        def fake_read(port,off,size):
            reads.append((off,size))
            if (off,size)==(0x8000,0x1000):
                return partition_table([NVS,OTAD,FACTORY,SPIFFS])
            if (off,size)==(0x10000,0x20):
                return b'\xe9'+bytes(31)
            raise AssertionError('Unexpected flash read '+str((off,size)))
        with tempfile.TemporaryDirectory() as td:
            image=Path(td)/'firmware.bin'
            header=bytearray(b'\x00'*614608);header[0]=0xe9;header[1]=2;header[3]=0x40
            struct.pack_into('<H',header,12,9)
            image.write_bytes(header)
            with patch.object(sys,'argv',['flash_app_only.py','COM13',str(image)]), \
                 patch.object(f,'run_esptool',side_effect=fake_esptool), \
                 patch.object(f,'read',side_effect=fake_read):
                f.main()
        self.assertEqual(reads,[(0x8000,0x1000),(0x10000,0x20)])
        writes=[args for args in seen if args[0]=='write_flash']
        self.assertEqual(len(writes),1)
        self.assertEqual(writes[0][-2],'0x10000')
        self.assertEqual(writes[0][1:7],('--flash_mode','keep','--flash_freq','keep','--flash_size','keep'))

    def test_ota_layout_still_requires_ota_data(self):
        found=f.parse_partitions(partition_table([NVS,OTAD,FACTORY,OTA0]))
        with self.assertRaisesRegex(RuntimeError,'without otadata'):
            f.find_next_boot(None,found)
        with self.assertRaisesRegex(RuntimeError,'CRC'):
            bad=bytearray(b'\xff'*8192)
            struct.pack_into('<I',bad,0,1)
            f.find_next_boot(bytes(bad),found)

    def test_factory_does_not_overwrite_without_esp_image_header(self):
        seen=[]
        def fake_esptool(port,*args):
            seen.append(args)
            if args[0]=='flash_id':return 'Chip is ESP32-S3\nDetected flash size: 16MB'
            return ''
        def fake_read(port,off,size):
            if off==0x8000:return partition_table([NVS,OTAD,FACTORY,SPIFFS])
            if off==0x10000:return bytes(0x20)
            raise AssertionError('Unexpected read')
        with tempfile.TemporaryDirectory() as td:
            image=Path(td)/'firmware.bin'
            h=bytearray(b'\x00'*4096);h[0]=0xe9;h[1]=2;h[3]=0x40
            struct.pack_into('<H',h,12,9)
            image.write_bytes(h)
            with patch.object(sys,'argv',['flash_app_only.py','COM13',str(image)]), \
                 patch.object(f,'run_esptool',side_effect=fake_esptool), \
                 patch.object(f,'read',side_effect=fake_read):
                with self.assertRaisesRegex(RuntimeError,'image magic'):
                    f.main()
        self.assertFalse(any(args[0]=='write_flash' for args in seen))

if __name__=='__main__':unittest.main(verbosity=2)
