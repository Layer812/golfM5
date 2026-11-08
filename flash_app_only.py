#!/usr/bin/env python3
"""Flashes only existing selected APP; checks actual 16MB partition/otadata/ESP32S3.

No bootloader, NVS, filesystem, otadata or partition table writes.
Compatible with esptool 4.9.0 and 5.2 (underscore CLI spelling).
"""
from pathlib import Path
import re
import os
import struct
import subprocess
import sys
import tempfile
import zlib

FLASH_BYTES = 16 * 1024 * 1024
# Do not assume a particular application's OTA layout.  Read the live table,
# then validate each range and select the actual boot APP from live otadata.
# R12 previously expected 7 partitions, while the logged device has 6.


def esptool_entry():
    """Prefer the exact PlatformIO esptool.py that succeeded with Clock Cat."""
    core = Path(os.environ.get('PLATFORMIO_CORE_DIR', str(Path.home() / '.platformio')))
    script = core / 'packages' / 'tool-esptoolpy' / 'esptool.py'
    if script.is_file():
        return [sys.executable, str(script)]
    # Fallback only selects the tool executable, NEVER a different COM port or APP slot.
    return [sys.executable, '-m', 'esptool']


def run_esptool(port, *args):
    # Successful Clock Cat path: device is ALREADY in ESP32-S3 ROM download mode.
    # Do not toggle native USB-JTAG RTS/DTR to reset or leave ROM during preflight.
    cmd = [*esptool_entry(), '--chip', 'esp32s3',
           '--port', port, '--baud', '460800', '--before', 'no_reset',
           '--after', 'no_reset', *args]
    print('> ', ' '.join(map(str, cmd)), flush=True)
    cp = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                        text=True, errors='replace')
    print(cp.stdout or '', flush=True)
    if cp.returncode:
        raise RuntimeError(f'esptool exited {cp.returncode}; NO further write attempted')
    return cp.stdout or ''


def check_image(path):
    if not path.is_file():
        raise RuntimeError('Missing BIN: ' + str(path))
    size = path.stat().st_size
    if not (4096 <= size <= FLASH_BYTES):
        raise RuntimeError('Invalid firmware image size')
    h = path.read_bytes()[:24]
    if len(h) != 24 or h[0] != 0xe9 or not 1 <= h[1] <= 16:
        raise RuntimeError('Invalid ESP32 firmware image magic/header')
    if struct.unpack_from('<H', h, 12)[0] != 9:
        raise RuntimeError('Firmware chip ID is not ESP32-S3')
    if ((h[3] >> 4) & 0xf) != 4:
        raise RuntimeError('BIN flash size header is not 16MB')
    print(f'PASS: ESP32-S3 image header, 16MB, {size} bytes', flush=True)
    return size


def parse_partitions(buf):
    """Parse live ESP-IDF partition table. Never rewrite or infer a layout."""
    if len(buf) != 0x1000:
        raise RuntimeError('Partition-table read is not 0x1000 bytes')
    found = {}
    ranges = [(0, 0x9000, 'bootloader/partition-table')]
    terminated = False
    for o in range(0, len(buf) - 31, 32):
        entry = buf[o:o + 32]
        magic = entry[:2]
        if magic in (b'\xff\xff', b'\xeb\xeb'):
            terminated = True
            break
        if magic != b'\xaa\x50':
            raise RuntimeError(f'Unexpected partition magic at 0x{o:X}')
        _, typ, sub, off, size, label, flags = struct.unpack('<HBBII16sI', entry)
        name_bytes = label.split(b'\x00', 1)[0]
        try:
            name = name_bytes.decode('ascii')
        except UnicodeDecodeError:
            raise RuntimeError('Invalid partition label')
        if (not name or name in found or off < 0x9000 or not size
                or off + size > FLASH_BYTES):
            raise RuntimeError('Duplicate/invalid partition ' + name)
        if off % 0x1000 or size % 0x1000:
            raise RuntimeError('Unaligned partition ' + name)
        if typ == 0 and (off % 0x10000 or size % 0x1000):
            raise RuntimeError('Unaligned APP ' + name)
        if flags & ~0x1:
            raise RuntimeError('Unexpected partition flags for ' + name)
        for lo, hi, other in ranges:
            if off < hi and lo < off + size:
                raise RuntimeError(f'Overlapping partitions: {name} / {other}')
        ranges.append((off, off + size, name))
        found[name] = (typ, sub, off, size)
    if not terminated or not found:
        raise RuntimeError('No partition table terminator or entries')
    # Check label/type relationships, not any fixed application/storage offset.
    for name, (typ, sub, off, size) in found.items():
        if typ == 0 and sub == 0x00 and name != 'factory':
            raise RuntimeError('Unexpected factory APP label ' + name)
        if typ == 0 and 0x10 <= sub <= 0x1F and name != f'ota_{sub - 0x10}':
            raise RuntimeError('OTA name/subtype disagreement: ' + name)
        if name.startswith('ota_') and name != 'otadata':
            if not (typ == 0 and 0x10 <= sub <= 0x1F and name == f'ota_{sub - 0x10}'):
                raise RuntimeError('Unexpected OTA partition definition: ' + name)
    if 'ota_0' not in found or found['ota_0'][:2] != (0, 0x10):
        raise RuntimeError('Cannot find a validated ota_0 APP')
    if 'otadata' not in found or found['otadata'][:2] != (1, 0) or found['otadata'][3] != 0x2000:
        raise RuntimeError('Cannot find two-sector OTA selection data')
    # If factory exists and otadata is empty, the bootloader may choose factory;
    # find_next_boot rejects that scenario.  Keep the factory APP untouched.
    slots = sorted(int(name[4:]) for name in found if re.fullmatch(r'ota_\d+', name))
    if not slots or slots != list(range(len(slots))):
        raise RuntimeError('OTA APP numbering has missing/noncontiguous slots')
    if len(slots) > 16:
        raise RuntimeError('Too many OTA APPs')
    return found


def find_next_boot(ota, found):
    """Select an existing APP from real otadata and real OTA slot count."""
    if len(ota) != 0x2000:
        raise RuntimeError('Bad otadata size')
    slots = sorted(int(name[4:]) for name in found if re.fullmatch(r'ota_\d+', name))
    count = len(slots)
    valid = []
    all_erased = True
    for sector in (0, 1):
        e = ota[sector * 4096: sector * 4096 + 32]
        seq, = struct.unpack_from('<I', e, 0)
        state, crc = struct.unpack_from('<II', e, 24)
        all_erased &= e == b'\xff' * 32
        if e == b'\xff' * 32:
            continue
        if seq == 0 or seq == 0xffffffff:
            raise RuntimeError(f'Invalid ota_seq in sector {sector}')
        calc = zlib.crc32(e[:4], 0xffffffff) & 0xffffffff
        if crc != calc:
            raise RuntimeError(f'CRC mismatch in otadata sector {sector}')
        if state in (0, 1):
            raise RuntimeError(f'OTA NEW/PENDING_VERIFY in sector {sector}; abort')
        if state not in (2, 0xffffffff):
            raise RuntimeError(f'Unaccepted OTA state {state} in sector {sector}')
        valid.append((seq, sector))
    if valid:
        seq, sector = max(valid)
        label = f'ota_{(seq - 1) % count}'
        if label not in found:
            raise RuntimeError('Selected APP is not in partition table')
        return label, f'otadata sector={sector} seq={seq} / {count} actual OTA APP(s)'
    if all_erased:
        if 'factory' in found:
            raise RuntimeError('Erased otadata + factory APP: refusing uncertain boot target')
        return 'ota_0', 'erased otadata without factory -> ota_0'
    raise RuntimeError('Cannot determine next boot APP')


def read(port, off, size):
    with tempfile.TemporaryDirectory(prefix='stopwatch_sample_read_') as td:
        f = Path(td) / 'read.bin'
        run_esptool(port, 'read_flash', hex(off), hex(size), str(f))
        b = f.read_bytes()
        if len(b) != size:
            raise RuntimeError('Flash read returned wrong length')
        return b


def main():
    if len(sys.argv) != 3:
        raise RuntimeError('Usage: flash_app_only.py COM16 sample_build\\.pio\\build\\m5stack-stopwatch\\firmware.bin')
    port = sys.argv[1].upper()
    if not re.fullmatch(r'COM[0-9]{1,3}', port):
        raise RuntimeError('Invalid COM port')
    bin_file = Path(sys.argv[2]).resolve()
    size = check_image(bin_file)
    print('READ-ONLY ESP32-S3 + 16MB FLASH CHECK', flush=True)
    info = run_esptool(port, 'flash_id')
    if not re.search(r'(Chip is ESP32-S3|Connected to ESP32-S3)', info):
        raise RuntimeError('ESP32-S3 identity not established')
    if not re.search(r'Detected flash size\s*:\s*16\s*MB', info, re.I):
        raise RuntimeError('Flash size 16MB not verified')
    print('READ-ONLY SECURITY STATUS', flush=True)
    run_esptool(port, 'get_security_info')
    print('READ-ONLY PARTITION TABLE at 0x8000', flush=True)
    found = parse_partitions(read(port, 0x8000, 0x1000))
    print(f'PASS: validated live partition table ({len(found)} entries)', flush=True)
    for name, (typ, sub, off, cap) in found.items():
        print(f'  {name}: type={typ} subtype=0x{sub:02X} offset=0x{off:X} size=0x{cap:X}', flush=True)
    print('READ-ONLY OTA BOOT SELECTION', flush=True)
    _, _, ota_off, ota_size = found['otadata']
    label, detail = find_next_boot(read(port, ota_off, ota_size), found)
    _, _, off, cap = found[label]
    if size > cap:
        raise RuntimeError(f'APP size {size} exceeds live boot APP capacity {cap} ({label})')
    print(f'PASS: next boot {label} at 0x{off:X} size 0x{cap:X} ({detail})', flush=True)
    print('READ-ONLY selected APP header (0x20 bytes)', flush=True)
    current_header = read(port, off, 0x20)
    if current_header[0] != 0xE9:
        raise RuntimeError('Selected APP does not have ESP image magic 0xE9; refuse to overwrite')
    print('PASS: selected APP is already bootable (ESP image magic)', flush=True)
    print('FLASHING ONLY SELECTED APP; no otadata/bootloader/partition/NVS/storage/coredump changes', flush=True)
    # esptool prints its *real* Wrote/Hash verified lines; no silent success reporting.
    run_esptool(port, 'write_flash', '--flash_mode', 'keep', '--flash_freq', 'keep', '--flash_size', 'keep', hex(off), str(bin_file))
    print(f'PASS: APP-only flash command completed to {label} at 0x{off:X}', flush=True)
    print('DEVICE LEFT IN ROM MODE: close monitors, SHORT-PRESS the power button once.', flush=True)
    print('NOTE: Windows may re-enumerate as COM17 on app boot. Do not open a serial monitor to check the screen.', flush=True)


if __name__ == '__main__':
    try:
        main()
    except (Exception,) as exc:
        print('SAFE STOP:', exc, file=sys.stderr, flush=True)
        print('If connection failed, ensure COM16 is in DOWNLOAD mode and no serial monitor is using it.', file=sys.stderr, flush=True)
        sys.exit(1)
