// SPDX-FileCopyrightText: 2026 marceldale
// SPDX-License-Identifier: GPL-3.0-only
//
// Pure layout arithmetic of the XU316 boot-flash writer, without any ESP-IDF dependency, so that the
// same code is checked at compile time (static_assert below) and mirrored by the host test
// firmware/esphome/tests/test_xmos_flasher.py.
#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace xmos_flasher {
namespace plan {

// Winbond W25Q32JV (datasheet rev. H): 4 KiB sectors (20h), 256-byte pages (02h), 4 MiB.
constexpr uint32_t PAGE_SIZE = 256;
constexpr uint32_t SECTOR_SIZE = 4096;
constexpr uint32_t FLASH_SIZE = 4u * 1024u * 1024u;
// JEDEC ID (9Fh): manufacturer EFh (Winbond), memory type 40h, capacity 16h (32 Mbit).
constexpr uint8_t JEDEC_MANUFACTURER = 0xEF;
constexpr uint8_t JEDEC_MEMORY_TYPE = 0x40;
constexpr uint8_t JEDEC_CAPACITY = 0x16;
// Boot partition the factory image was built for (xflash --boot-partition-size 0x100000,
// firmware/xmos/README.md). Everything inside it is erased so that no stale upgrade image survives;
// the data partition above it is not touched.
constexpr uint32_t BOOT_PARTITION_SIZE = 0x100000;

constexpr uint32_t div_ceil(uint32_t a, uint32_t b) { return (a + b - 1) / b; }

// Number of pages that carry image bytes (the last one is padded with 0xFF).
constexpr uint32_t pages_for(uint32_t image_len) { return div_ceil(image_len, PAGE_SIZE); }
// Bytes of the last page that belong to the image (PAGE_SIZE if the image ends on a page boundary).
constexpr uint32_t last_page_fill(uint32_t image_len) {
  return image_len == 0 ? 0 : (image_len % PAGE_SIZE == 0 ? PAGE_SIZE : image_len % PAGE_SIZE);
}
// Sectors erased before writing: the whole boot partition.
constexpr uint32_t sectors_to_erase() { return BOOT_PARTITION_SIZE / SECTOR_SIZE; }
// The image must fit into the boot partition; the boot partition into the flash.
constexpr bool image_fits(uint32_t image_len) {
  return image_len > 0 && image_len <= BOOT_PARTITION_SIZE && BOOT_PARTITION_SIZE <= FLASH_SIZE;
}
// Address of page n; pages never cross a 256-byte boundary (page program wraps inside a page).
constexpr uint32_t page_address(uint32_t n) { return n * PAGE_SIZE; }
// Verification covers the whole boot partition: [0, image_len) must hash to the expected MD5,
// [image_len, BOOT_PARTITION_SIZE) must read 0xFF (erased).
constexpr uint32_t verify_length() { return BOOT_PARTITION_SIZE; }

// --- compile-time checks with the shipped image (ffva_v1.3.1_factory.bin, 286 720 bytes) ---------
constexpr uint32_t FFVA_131_LEN = 286720;
static_assert(SECTOR_SIZE % PAGE_SIZE == 0, "sector must hold whole pages");
static_assert(BOOT_PARTITION_SIZE % SECTOR_SIZE == 0, "boot partition must be whole sectors");
static_assert(sectors_to_erase() == 256, "1 MiB boot partition = 256 sectors");
static_assert(image_fits(FFVA_131_LEN), "factory image must fit the boot partition");
static_assert(pages_for(FFVA_131_LEN) == 1120, "286720 / 256 = 1120 pages");
static_assert(last_page_fill(FFVA_131_LEN) == 256, "image ends on a page boundary");
static_assert(pages_for(FFVA_131_LEN) * PAGE_SIZE == FFVA_131_LEN, "no padding for this image");
static_assert(div_ceil(FFVA_131_LEN, SECTOR_SIZE) == 70, "image occupies 70 sectors");
static_assert(page_address(1119) == 0x45F00, "last page address");
static_assert(pages_for(1) == 1 && last_page_fill(1) == 1, "one-byte image: one page, padded");
static_assert(pages_for(257) == 2 && last_page_fill(257) == 1, "257 bytes: two pages");
static_assert(!image_fits(0) && !image_fits(BOOT_PARTITION_SIZE + 1), "size limits");

}  // namespace plan
}  // namespace xmos_flasher
}  // namespace esphome
