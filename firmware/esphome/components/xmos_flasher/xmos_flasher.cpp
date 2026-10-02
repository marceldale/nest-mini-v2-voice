// SPDX-FileCopyrightText: 2024-2026 Mischa Siekmann (FutureProofHomes, Satellite1-ESPHome)
// SPDX-FileCopyrightText: 2026 marceldale
// SPDX-License-Identifier: GPL-3.0-only
//
// See xmos_flasher.h and README.md. Command set and timing: Winbond W25Q32JV datasheet rev. H —
// 9Fh JEDEC ID, 05h/35h status register 1/2, 06h write enable, 20h sector erase (tSE 45 ms typ /
// 400 ms max), 02h page program (tPP 0.4 ms typ / 3 ms max), 03h read data (fR up to 50 MHz),
// 66h/99h software reset (tRST 30 us). The status registers are only read, never written.
#include "xmos_flasher.h"

#include <algorithm>
#include <cstring>

#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"

namespace esphome {
namespace xmos_flasher {

static const char *const TAG = "xmos_flasher";

static constexpr spi_host_device_t HOST = SPI2_HOST;
static constexpr size_t READ_CHUNK = plan::SECTOR_SIZE;           // verify reads 4 KiB per loop
static constexpr size_t BUF_SIZE = 4 + READ_CHUNK;                // command + address + data
static constexpr uint32_t RESET_SETTLE_MS = 10;                   // C63 1 uF is discharged by Q2 in < 0.1 ms
static constexpr uint32_t PAGE_TIMEOUT_MS = 10;                   // tPP max 3 ms
static constexpr uint32_t SECTOR_TIMEOUT_MS = 1000;               // tSE max 400 ms
static constexpr uint8_t SR1_BUSY = 0x01, SR1_WEL = 0x02, SR2_QE = 0x02;
static constexpr uint8_t SR1_PROTECT = 0x7C;  // BP0, BP1, BP2, TB, SEC
static constexpr uint8_t SR2_CMP = 0x40;
static constexpr uint32_t LOOP_BUDGET_MS = 20;  // work per loop() call (ESPHome loop interval 16 ms)

const char *stage_name(Stage stage) {
  switch (stage) {
    case Stage::IDLE: return "idle";
    case Stage::CONNECT: return "connect";
    case Stage::IDENTIFY: return "identify";
    case Stage::ERASE: return "erase";
    case Stage::WRITE: return "write";
    case Stage::VERIFY: return "verify";
    case Stage::DONE: return "done";
    case Stage::FAILED: return "failed";
  }
  return "?";
}

void XmosFlasher::setup() {
  // CLK and D0 are wired straight to the XU316's QSPI pins; outside a write they must stay
  // high-impedance inputs. CS_N and MISO only reach the flash through U16 (switched by XU316_RST).
  // The reset pin is shared with voice_kit (allow_other_uses); setup() only sets the pin mode and
  // does not change its level.
  if (this->reset_pin_ != nullptr)
    this->reset_pin_->setup();
  this->pins_to_input_();
  this->set_stage_(Stage::IDLE);
}

void XmosFlasher::dump_config() {
  ESP_LOGCONFIG(TAG, "XMOS boot-flash writer:");
  ESP_LOGCONFIG(TAG, "  Image: %u bytes, MD5 %s", (unsigned) this->image_length_,
                this->image_md5_ != nullptr ? this->image_md5_ : "-");
  ESP_LOGCONFIG(TAG, "  SPI: CLK GPIO%u, MOSI GPIO%u, MISO GPIO%u, CS GPIO%u, %u Hz", this->clk_pin_,
                this->mosi_pin_, this->miso_pin_, this->cs_pin_, (unsigned) this->frequency_);
  LOG_PIN("  Reset/switch pin: ", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "  Boot partition: %u bytes erased and verified", (unsigned) plan::BOOT_PARTITION_SIZE);
}

void XmosFlasher::pins_to_input_() {
  // CLK and D0 share the XU316's QSPI lines: input without any pull, so nothing loads the XU316's
  // flash access. CS_N and MISO end on U16's open 1D side while S is low: weak pull-down so they do
  // not float.
  for (uint8_t pin : {this->clk_pin_, this->mosi_pin_, this->miso_pin_, this->cs_pin_}) {
    gpio_reset_pin((gpio_num_t) pin);  // detaches the pin from the SPI matrix (leaves a pull-up on)
    gpio_set_direction((gpio_num_t) pin, GPIO_MODE_INPUT);
    const bool shared = pin == this->clk_pin_ || pin == this->mosi_pin_;
    gpio_set_pull_mode((gpio_num_t) pin, shared ? GPIO_FLOATING : GPIO_PULLDOWN_ONLY);
  }
}

void XmosFlasher::set_stage_(Stage stage) {
  this->stage_ = stage;
#ifdef USE_TEXT_SENSOR
  if (this->status_sensor_ != nullptr) {
    if (stage == Stage::FAILED) {
      this->status_sensor_->publish_state(std::string("failed: ") + this->error_);
    } else {
      this->status_sensor_->publish_state(stage_name(stage));
    }
  }
#endif
}

void XmosFlasher::publish_progress_(bool force) {
  const uint32_t pages = plan::pages_for(this->image_length_);
  const uint32_t sectors = plan::sectors_to_erase();
  const uint32_t verify = plan::verify_length() / READ_CHUNK;
  // Weighted by typical time: erase 256 x 45 ms = 11.5 s, write 1120 x ~1.5 ms = 1.7 s, verify
  // 256 x 8.2 ms = 2.1 s (4 MHz) -> 75 / 11 / 14 %.
  uint32_t pct = 0;
  switch (this->stage_) {
    case Stage::ERASE: pct = 75 * this->sector_ / sectors; break;
    case Stage::WRITE: pct = 75 + 11 * this->page_ / std::max<uint32_t>(pages, 1); break;
    case Stage::VERIFY: pct = 86 + 14 * (this->verify_pos_ / READ_CHUNK) / verify; break;
    case Stage::DONE: pct = 100; break;
    default: pct = this->progress_; break;
  }
  this->progress_ = std::min<uint32_t>(pct, 100);
  const uint32_t now = millis();
  if (!force && now - this->last_publish_ < 1000)
    return;
  this->last_publish_ = now;
  ESP_LOGD(TAG, "%s: %u %%", stage_name(this->stage_), this->progress_);
#ifdef USE_SENSOR
  if (this->progress_sensor_ != nullptr)
    this->progress_sensor_->publish_state(this->progress_);
#endif
}

bool XmosFlasher::start() {
  if (this->in_progress()) {
    ESP_LOGW(TAG, "Write already in progress");
    return false;
  }
  if (this->image_ == nullptr || !plan::image_fits(this->image_length_) || this->reset_pin_ == nullptr) {
    ESP_LOGE(TAG, "No usable embedded image or reset pin");
    return false;
  }
  ESP_LOGW(TAG, "Writing the XU316 factory image (%u bytes) into U11", (unsigned) this->image_length_);
  this->error_.clear();
  this->sector_ = this->page_ = this->verify_pos_ = 0;
  this->erase_pending_ = false;
  this->tail_erased_ = true;
  this->progress_ = 0;
  this->started_ms_ = millis();
  this->set_stage_(Stage::CONNECT);
  this->publish_progress_(true);
  return true;
}

void XmosFlasher::fail_(const char *what, uint32_t addr) {
  char buf[96];
  snprintf(buf, sizeof(buf), "%s %s at 0x%06X", stage_name(this->stage_), what, (unsigned) addr);
  this->error_ = buf;
  ESP_LOGE(TAG, "Aborted: %s - XU316 released; press the button again or restart", buf);
  this->bus_release_();
  this->set_stage_(Stage::FAILED);
  this->publish_progress_(true);
  this->failure_callback_.call();
}

void XmosFlasher::finish_() {
  this->bus_release_();
  this->set_stage_(Stage::DONE);
  this->publish_progress_(true);
  ESP_LOGI(TAG, "XU316 image written and verified in %u ms; XU316 released from reset",
           (unsigned) (millis() - this->started_ms_));
  this->success_callback_.call();
}

// --- bus ------------------------------------------------------------------------------------------

bool XmosFlasher::bus_acquire_() {
  // 1. XU316 into reset and U16 to the 2D side: both follow XU316_RST (GPIO46 -> R47 -> Q2 pulls
  //    RST_N low; the same net drives U16 pin 9 S). In reset the XU316's GPIOs, including the QSPI
  //    pins, are inputs with weak pull-downs (XU316 datasheet section 8).
  this->reset_pin_->digital_write(true);
  delay(RESET_SETTLE_MS);

  this->tx_buf_ = (uint8_t *) heap_caps_malloc(BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  this->rx_buf_ = (uint8_t *) heap_caps_malloc(BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (this->tx_buf_ == nullptr || this->rx_buf_ == nullptr)
    return false;

  spi_bus_config_t bus{};
  bus.mosi_io_num = this->mosi_pin_;
  bus.miso_io_num = this->miso_pin_;
  bus.sclk_io_num = this->clk_pin_;
  bus.quadwp_io_num = -1;
  bus.quadhd_io_num = -1;
  bus.max_transfer_sz = BUF_SIZE;
  if (spi_bus_initialize(HOST, &bus, SPI_DMA_CH_AUTO) != ESP_OK)
    return false;
  this->bus_initialised_ = true;

  spi_device_interface_config_t dev{};
  dev.mode = 0;  // W25Q32JV supports SPI modes 0 and 3
  dev.clock_speed_hz = (int) this->frequency_;
  dev.spics_io_num = this->cs_pin_;
  dev.queue_size = 1;
  spi_device_handle_t handle = nullptr;
  if (spi_bus_add_device(HOST, &dev, &handle) != ESP_OK)
    return false;
  this->spi_device_ = handle;
  return true;
}

void XmosFlasher::bus_release_() {
  if (this->spi_device_ != nullptr) {
    spi_bus_remove_device((spi_device_handle_t) this->spi_device_);
    this->spi_device_ = nullptr;
  }
  if (this->bus_initialised_) {
    spi_bus_free(HOST);
    this->bus_initialised_ = false;
  }
  heap_caps_free(this->tx_buf_);
  heap_caps_free(this->rx_buf_);
  this->tx_buf_ = this->rx_buf_ = nullptr;
  // CLK/D0 back to high-impedance BEFORE the XU316 leaves reset, then release reset (U16 isolates
  // CS_N/MISO at the same moment).
  this->pins_to_input_();
  if (this->reset_pin_ != nullptr)
    this->reset_pin_->digital_write(false);
}

bool XmosFlasher::xfer_(const uint8_t *tx, uint8_t *rx, size_t len) {
  if (this->spi_device_ == nullptr || len > BUF_SIZE)
    return false;
  memcpy(this->tx_buf_, tx, len);
  spi_transaction_t t{};
  t.length = len * 8;
  t.tx_buffer = this->tx_buf_;
  t.rx_buffer = this->rx_buf_;
  if (spi_device_polling_transmit((spi_device_handle_t) this->spi_device_, &t) != ESP_OK)
    return false;
  if (rx != nullptr)
    memcpy(rx, this->rx_buf_, len);
  return true;
}

bool XmosFlasher::command_(uint8_t cmd) { return this->xfer_(&cmd, nullptr, 1); }

bool XmosFlasher::read_status_(uint8_t cmd, uint8_t *value) {
  uint8_t tx[2] = {cmd, 0}, rx[2] = {0, 0};
  if (!this->xfer_(tx, rx, 2))
    return false;
  *value = rx[1];
  return true;
}

bool XmosFlasher::write_enable_() {
  uint8_t sr1 = 0;
  return this->command_(0x06) && this->read_status_(0x05, &sr1) && (sr1 & SR1_WEL);
}

bool XmosFlasher::wait_ready_(uint32_t timeout_ms) {
  const uint32_t t0 = millis();
  uint8_t sr1 = SR1_BUSY;
  while (millis() - t0 < timeout_ms) {
    if (!this->read_status_(0x05, &sr1))
      return false;
    if ((sr1 & SR1_BUSY) == 0)
      return true;
  }
  return false;
}

bool XmosFlasher::read_(uint32_t addr, uint8_t *dst, size_t len) {
  if (len + 4 > BUF_SIZE)
    return false;
  memset(this->tx_buf_, 0, len + 4);
  this->tx_buf_[0] = 0x03;
  this->tx_buf_[1] = (addr >> 16) & 0xFF;
  this->tx_buf_[2] = (addr >> 8) & 0xFF;
  this->tx_buf_[3] = addr & 0xFF;
  spi_transaction_t t{};
  t.length = (len + 4) * 8;
  t.tx_buffer = this->tx_buf_;
  t.rx_buffer = this->rx_buf_;
  if (spi_device_polling_transmit((spi_device_handle_t) this->spi_device_, &t) != ESP_OK)
    return false;
  if (dst != nullptr)
    memcpy(dst, this->rx_buf_ + 4, len);
  return true;
}

bool XmosFlasher::program_page_(uint32_t addr, const uint8_t *src) {
  if ((addr % plan::PAGE_SIZE) != 0 || !this->write_enable_())
    return false;
  uint8_t tx[4 + plan::PAGE_SIZE];
  tx[0] = 0x02;
  tx[1] = (addr >> 16) & 0xFF;
  tx[2] = (addr >> 8) & 0xFF;
  tx[3] = addr & 0xFF;
  memcpy(tx + 4, src, plan::PAGE_SIZE);
  if (!this->xfer_(tx, nullptr, sizeof(tx)))
    return false;
  return this->wait_ready_(PAGE_TIMEOUT_MS);
}

bool XmosFlasher::erase_sector_(uint32_t addr) {
  if (!this->write_enable_())
    return false;
  uint8_t tx[4] = {0x20, (uint8_t) ((addr >> 16) & 0xFF), (uint8_t) ((addr >> 8) & 0xFF), (uint8_t) (addr & 0xFF)};
  return this->xfer_(tx, nullptr, 4);
}

// --- state machine (one bounded step per loop() call) ---------------------------------------------

void XmosFlasher::loop() {
  // Several bounded steps per call (each <= ~11 ms), so the write is not paced by the 16 ms loop.
  const uint32_t t0 = millis();
  do {
    this->step_();
  } while (this->in_progress() && millis() - t0 < LOOP_BUDGET_MS);
}

void XmosFlasher::step_() {
  switch (this->stage_) {
    case Stage::IDLE:
    case Stage::DONE:
    case Stage::FAILED:
      return;

    case Stage::CONNECT:
      if (!this->bus_acquire_()) {
        this->fail_("SPI bus could not be set up", 0);
        return;
      }
      this->set_stage_(Stage::IDENTIFY);
      return;

    case Stage::IDENTIFY: {
      // The XU316 may have left the flash in continuous-read mode or in the middle of an erase/program
      // (e.g. an interrupted DFU): 16 clocks with IO0 high end continuous-read mode, then wait while
      // BUSY (a reset during erase/program can corrupt data), then 66h + 99h to return to the
      // power-on state (WEL, volatile bits, read mode).
      const uint8_t ff[2] = {0xFF, 0xFF};
      if (!this->xfer_(ff, nullptr, 2) || !this->wait_ready_(SECTOR_TIMEOUT_MS)) {
        this->fail_("flash stays busy or does not answer", 0);
        return;
      }
      if (!this->command_(0x66) || !this->command_(0x99)) {
        this->fail_("software reset failed", 0);
        return;
      }
      delay(1);  // tRST 30 us
      uint8_t tx[4] = {0x9F, 0, 0, 0}, rx[4] = {0, 0, 0, 0};
      if (!this->xfer_(tx, rx, 4)) {
        this->fail_("JEDEC read failed", 0);
        return;
      }
      ESP_LOGI(TAG, "JEDEC ID %02X %02X %02X", rx[1], rx[2], rx[3]);
      if (rx[1] != plan::JEDEC_MANUFACTURER || rx[2] != plan::JEDEC_MEMORY_TYPE || rx[3] != plan::JEDEC_CAPACITY) {
        this->fail_("JEDEC ID is not EF 40 16 (W25Q32JV)", 0);
        return;
      }
      uint8_t sr2 = 0;
      if (!this->read_status_(0x35, &sr2)) {
        this->fail_("status register 2 read failed", 0);
        return;
      }
      ESP_LOGI(TAG, "Status register 2: 0x%02X (QE=%u)", sr2, (sr2 & SR2_QE) ? 1 : 0);
      if ((sr2 & SR2_QE) == 0) {
        // QE=0 would enable /HOLD, which the XU316 pulls low in reset; the IQ part has QE=1 fixed.
        this->fail_("QE=0: not a W25Q32JV-IQ, status register left unchanged", 0);
        return;
      }
      uint8_t sr1 = 0;
      if (!this->read_status_(0x05, &sr1)) {
        this->fail_("status register 1 read failed", 0);
        return;
      }
      if ((sr1 & SR1_PROTECT) != 0 || (sr2 & SR2_CMP) != 0) {
        // Erase/program would be ignored silently in protected areas; never clear protection here.
        ESP_LOGE(TAG, "SR1 0x%02X, SR2 0x%02X", sr1, sr2);
        this->fail_("block protection set (BP/TB/SEC/CMP), status register left unchanged", 0);
        return;
      }
      this->set_stage_(Stage::ERASE);
      return;
    }

    case Stage::ERASE: {
      if (this->erase_pending_) {
        uint8_t sr1 = 0;
        if (!this->read_status_(0x05, &sr1)) {
          this->fail_("status read failed", this->sector_ * plan::SECTOR_SIZE);
          return;
        }
        if (sr1 & SR1_BUSY) {
          if (millis() - this->erase_started_ > SECTOR_TIMEOUT_MS)
            this->fail_("sector erase timeout", this->sector_ * plan::SECTOR_SIZE);
          return;
        }
        this->erase_pending_ = false;
        this->sector_++;
        this->publish_progress_(false);
      }
      if (this->sector_ >= plan::sectors_to_erase()) {
        ESP_LOGI(TAG, "Boot partition erased (%u sectors)", (unsigned) plan::sectors_to_erase());
        this->set_stage_(Stage::WRITE);
        return;
      }
      if (!this->erase_sector_(this->sector_ * plan::SECTOR_SIZE)) {
        this->fail_("sector erase command failed", this->sector_ * plan::SECTOR_SIZE);
        return;
      }
      this->erase_pending_ = true;
      this->erase_started_ = millis();
      return;
    }

    case Stage::WRITE: {
      const uint32_t pages = plan::pages_for(this->image_length_);
      if (this->page_ >= pages) {
        this->set_stage_(Stage::VERIFY);
        this->md5_.init();
        this->verify_pos_ = 0;
        return;
      }
      const uint32_t addr = plan::page_address(this->page_);
      uint8_t page[plan::PAGE_SIZE];
      const uint32_t n = std::min<uint32_t>(plan::PAGE_SIZE, this->image_length_ - addr);
      memcpy(page, this->image_ + addr, n);
      memset(page + n, 0xFF, plan::PAGE_SIZE - n);  // pad the last page with the erased value
      if (!this->program_page_(addr, page)) {
        this->fail_("page program failed", addr);
        return;
      }
      uint8_t back[plan::PAGE_SIZE];
      if (!this->read_(addr, back, plan::PAGE_SIZE) || memcmp(page, back, plan::PAGE_SIZE) != 0) {
        this->fail_("page read-back mismatch", addr);
        return;
      }
      this->page_++;
      this->publish_progress_(false);
      return;
    }

    case Stage::VERIFY: {
      if (this->verify_pos_ >= plan::verify_length()) {
        char hex[33];
        this->md5_.calculate();
        this->md5_.get_hex(hex);
        hex[32] = 0;
        if (strncasecmp(hex, this->image_md5_, 32) != 0) {
          ESP_LOGE(TAG, "MD5 read back %s, expected %s", hex, this->image_md5_);
          this->fail_("MD5 mismatch", 0);
          return;
        }
        if (!this->tail_erased_) {
          this->fail_("boot partition tail not erased", (uint32_t) this->image_length_);
          return;
        }
        ESP_LOGI(TAG, "MD5 %s matches; rest of the boot partition is 0xFF", hex);
        this->finish_();
        return;
      }
      if (!this->read_(this->verify_pos_, nullptr, READ_CHUNK)) {
        this->fail_("read failed", this->verify_pos_);
        return;
      }
      const uint8_t *buf = this->rx_buf_ + 4;  // data stays in the DMA buffer
      const uint32_t end = this->verify_pos_ + READ_CHUNK;
      if (this->verify_pos_ < this->image_length_) {
        const uint32_t n = std::min<uint32_t>(end, this->image_length_) - this->verify_pos_;
        this->md5_.add(buf, n);
      }
      for (uint32_t a = std::max<uint32_t>(this->verify_pos_, this->image_length_); a < end; a++) {
        if (buf[a - this->verify_pos_] != 0xFF) {
          this->tail_erased_ = false;
          break;
        }
      }
      this->verify_pos_ = end;
      this->publish_progress_(false);
      return;
    }
  }
}

}  // namespace xmos_flasher
}  // namespace esphome
