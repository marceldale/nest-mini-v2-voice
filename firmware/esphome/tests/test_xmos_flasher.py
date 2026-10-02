# SPDX-FileCopyrightText: 2026 marceldale
# SPDX-License-Identifier: MIT
"""Host test of the xmos_flasher image logic (no hardware, no compiler needed).

It runs the same sequence as XmosFlasher::loop() — identify, erase the boot partition sector by
sector, program page by page with read-back, verify the whole partition (MD5 of the image + erased
tail) — against a simulated W25Q32JV, with the constants read from flash_plan.h so that the test
and the C++ code cannot drift apart. The C++ arithmetic itself is additionally checked by the
static_asserts in flash_plan.h on every firmware build.

    python firmware/esphome/tests/test_xmos_flasher.py
"""
import hashlib
import random
import re
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
COMP = HERE.parent / "components" / "xmos_flasher"
FIRMWARE = HERE.parent.parent / "xmos"


def _plan_constants():
    text = (COMP / "flash_plan.h").read_text(encoding="utf-8")
    c = {}
    for name in ("PAGE_SIZE", "SECTOR_SIZE", "BOOT_PARTITION_SIZE", "FFVA_131_LEN"):
        m = re.search(r"constexpr uint32_t %s = ([0-9xA-Fa-f]+);" % name, text)
        c[name] = int(m.group(1), 0)
    m = re.search(r"constexpr uint32_t FLASH_SIZE = 4u \* 1024u \* 1024u;", text)
    c["FLASH_SIZE"] = 4 * 1024 * 1024 if m else None
    for name in ("JEDEC_MANUFACTURER", "JEDEC_MEMORY_TYPE", "JEDEC_CAPACITY"):
        c[name] = int(re.search(r"constexpr uint8_t %s = (0x[0-9A-Fa-f]+);" % name, text).group(1), 16)
    return c


P = _plan_constants()


class FlashError(Exception):
    pass


class W25Q32JV:
    """Simulated flash: NOR semantics (program only clears bits), 4 KiB erase, 256-byte page wrap."""

    def __init__(self, jedec=(0xEF, 0x40, 0x16), qe=True, fill=None, sr1=0x00, cmp_bit=False, busy_ms=0):
        self.mem = bytearray(fill if fill is not None else b"\xff" * P["FLASH_SIZE"])
        self.jedec = jedec
        self.sr2 = (0x02 if qe else 0x00) | (0x40 if cmp_bit else 0x00)
        self.sr1 = sr1          # BP/TB/SEC bits as left by a previous owner
        self.busy_ms = busy_ms  # an erase/program the XU316 left running (interrupted DFU)
        self.reset_while_busy = False
        self.wel = False
        self.stuck_bit = None  # (address, bit) that never programs, for fault injection

    def read_jedec(self):
        return self.jedec

    def wait_ready(self, timeout_ms):
        if self.busy_ms > timeout_ms:
            return False
        self.busy_ms = 0
        return True

    def software_reset(self):
        if self.busy_ms:
            self.reset_while_busy = True
        self.wel = False

    def write_enable(self):
        self.wel = True

    def erase_sector(self, addr):
        if not self.wel:
            raise FlashError("erase without WREN")
        base = addr - addr % P["SECTOR_SIZE"]
        self.mem[base:base + P["SECTOR_SIZE"]] = b"\xff" * P["SECTOR_SIZE"]
        self.wel = False

    def program_page(self, addr, data):
        if not self.wel:
            raise FlashError("program without WREN")
        base = addr - addr % P["PAGE_SIZE"]
        for i, b in enumerate(data):
            a = base + ((addr - base + i) % P["PAGE_SIZE"])  # wraps inside the page like the device
            v = self.mem[a] & b
            if self.stuck_bit and self.stuck_bit[0] == a:
                v |= 1 << self.stuck_bit[1]
            self.mem[a] = v
        self.wel = False

    def read(self, addr, n):
        return bytes(self.mem[addr:addr + n])


def pages_for(n):
    return -(-n // P["PAGE_SIZE"])


def write_image(flash, image, md5_hex):
    """Mirror of XmosFlasher::loop(): returns None on success, else the abort reason."""
    if not flash.wait_ready(1000):  # after 16 clocks of FFh (ends continuous-read mode)
        return "identify: flash stays busy"
    flash.software_reset()          # 66h + 99h, only after BUSY has cleared
    if flash.read_jedec() != (P["JEDEC_MANUFACTURER"], P["JEDEC_MEMORY_TYPE"], P["JEDEC_CAPACITY"]):
        return "identify: JEDEC ID is not EF 40 16"
    if not flash.sr2 & 0x02:
        return "identify: QE=0"
    if flash.sr1 & 0x7C or flash.sr2 & 0x40:
        return "identify: block protection set"
    if not 0 < len(image) <= P["BOOT_PARTITION_SIZE"]:
        return "image does not fit"
    for sector in range(P["BOOT_PARTITION_SIZE"] // P["SECTOR_SIZE"]):
        flash.write_enable()
        flash.erase_sector(sector * P["SECTOR_SIZE"])
    for page in range(pages_for(len(image))):
        addr = page * P["PAGE_SIZE"]
        chunk = image[addr:addr + P["PAGE_SIZE"]]
        chunk = chunk + b"\xff" * (P["PAGE_SIZE"] - len(chunk))
        flash.write_enable()
        flash.program_page(addr, chunk)
        if flash.read(addr, P["PAGE_SIZE"]) != chunk:
            return "write: page read-back mismatch at 0x%06X" % addr
    md5 = hashlib.md5()
    tail_ok = True
    for pos in range(0, P["BOOT_PARTITION_SIZE"], P["SECTOR_SIZE"]):
        buf = flash.read(pos, P["SECTOR_SIZE"])
        end = pos + P["SECTOR_SIZE"]
        if pos < len(image):
            md5.update(buf[: min(end, len(image)) - pos])
        start = max(pos, len(image))
        if any(b != 0xFF for b in buf[start - pos:]):
            tail_ok = False
    if md5.hexdigest() != md5_hex.lower():
        return "verify: MD5 mismatch"
    if not tail_ok:
        return "verify: boot partition tail not erased"
    return None


class Tests(unittest.TestCase):
    def test_constants_match_datasheet_and_readme(self):
        self.assertEqual(P["PAGE_SIZE"], 256)
        self.assertEqual(P["SECTOR_SIZE"], 4096)
        self.assertEqual(P["BOOT_PARTITION_SIZE"], 0x100000)
        self.assertEqual(P["FLASH_SIZE"], 4 * 1024 * 1024)

    def test_shipped_image_md5_and_size(self):
        image = (FIRMWARE / "ffva_v1.3.1_factory.bin").read_bytes()
        md5sums = (FIRMWARE / "MD5SUMS").read_text(encoding="utf-8")
        expected = re.search(r"^([0-9a-f]{32}) \*ffva_v1\.3\.1_factory\.bin$", md5sums, re.M).group(1)
        self.assertEqual(len(image), P["FFVA_131_LEN"])
        self.assertEqual(hashlib.md5(image).hexdigest(), expected)
        self.assertEqual(pages_for(len(image)), 1120)

    def test_write_shipped_image_over_stale_content(self):
        image = (FIRMWARE / "ffva_v1.3.1_factory.bin").read_bytes()
        rnd = random.Random(1)
        old = bytearray(rnd.getrandbits(8) for _ in range(P["FLASH_SIZE"]))  # damaged/old content
        flash = W25Q32JV(fill=old)
        data_partition_before = flash.read(P["BOOT_PARTITION_SIZE"], 4096)
        self.assertIsNone(write_image(flash, image, hashlib.md5(image).hexdigest()))
        self.assertEqual(flash.read(0, len(image)), image)
        self.assertEqual(flash.read(len(image), P["BOOT_PARTITION_SIZE"] - len(image)),
                         b"\xff" * (P["BOOT_PARTITION_SIZE"] - len(image)))  # no stale upgrade header
        self.assertEqual(flash.read(P["BOOT_PARTITION_SIZE"], 4096), data_partition_before)  # data untouched

    def test_odd_lengths_are_padded(self):
        for n in (1, 255, 256, 257, 4095, 4097, 300001):
            image = bytes(random.Random(n).getrandbits(8) for _ in range(n))
            flash = W25Q32JV()
            self.assertIsNone(write_image(flash, image, hashlib.md5(image).hexdigest()), n)
            self.assertEqual(flash.read(n, pages_for(n) * 256 - n), b"\xff" * (pages_for(n) * 256 - n))

    def test_aborts(self):
        image = b"\x00" * 1000
        md5 = hashlib.md5(image).hexdigest()
        self.assertIn("JEDEC", write_image(W25Q32JV(jedec=(0xEF, 0x40, 0x17)), image, md5))
        self.assertIn("QE=0", write_image(W25Q32JV(qe=False), image, md5))
        bad = W25Q32JV()
        bad.stuck_bit = (512 + 7, 3)
        self.assertIn("0x000200", write_image(bad, image, md5))
        self.assertIn("MD5", write_image(W25Q32JV(), image, "0" * 32))
        self.assertIn("fit", write_image(W25Q32JV(), b"\x00" * (P["BOOT_PARTITION_SIZE"] + 1), md5))
        self.assertIn("protection", write_image(W25Q32JV(sr1=0x1C), image, md5))
        self.assertIn("protection", write_image(W25Q32JV(cmp_bit=True), image, md5))
        self.assertIn("busy", write_image(W25Q32JV(busy_ms=5000), image, md5))

    def test_reset_only_after_busy_cleared(self):
        image = b"\x5a" * 5000
        flash = W25Q32JV(busy_ms=300)  # interrupted sector erase, finishes within tSE max
        self.assertIsNone(write_image(flash, image, hashlib.md5(image).hexdigest()))
        self.assertFalse(flash.reset_while_busy)


if __name__ == "__main__":
    unittest.main(verbosity=2)
