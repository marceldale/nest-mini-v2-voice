// SPDX-FileCopyrightText: 2024-2026 Mischa Siekmann (FutureProofHomes, Satellite1-ESPHome)
// SPDX-FileCopyrightText: 2026 marceldale
// SPDX-License-Identifier: GPL-3.0-only
//
// Writes the XU316 factory image into its boot flash U11 (W25Q32JVSSIQ) from the ESP32-S3.
// Derived from FutureProofHomes/Satellite1-ESPHome, esphome/components/memory_flasher and
// esphome/components/satellite1/memory_flasher/xmos_flashing.* (state machine, page write with
// read-back, MD5 over the written image). Adapted to this board: one bus switch (U16) instead of a
// switched bus, CLK/D0 permanently wired to the XU316, reset and switch on the same GPIO.
// See README.md in this directory for every deviation.
#pragma once

#include <string>

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/md5/md5.h"
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

#include "flash_plan.h"

namespace esphome {
namespace xmos_flasher {

enum class Stage : uint8_t {
  IDLE,
  CONNECT,   // XU316 into reset, bus to the ESP32, SPI up
  IDENTIFY,  // JEDEC ID, status register 2 (QE)
  ERASE,     // boot partition, one 4 KiB sector per loop
  WRITE,     // one 256-byte page per loop, read back and compared
  VERIFY,    // whole boot partition: image MD5 + erased tail
  DONE,
  FAILED,
};

const char *stage_name(Stage stage);

class XmosFlasher : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_image(const uint8_t *data, size_t length, const char *md5_hex) {
    this->image_ = data;
    this->image_length_ = length;
    this->image_md5_ = md5_hex;
  }
  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }
  void set_spi_pins(uint8_t clk, uint8_t mosi, uint8_t miso, uint8_t cs) {
    this->clk_pin_ = clk;
    this->mosi_pin_ = mosi;
    this->miso_pin_ = miso;
    this->cs_pin_ = cs;
  }
  void set_frequency(uint32_t hz) { this->frequency_ = hz; }
#ifdef USE_SENSOR
  void set_progress_sensor(sensor::Sensor *s) { this->progress_sensor_ = s; }
#endif
#ifdef USE_TEXT_SENSOR
  void set_status_text_sensor(text_sensor::TextSensor *s) { this->status_sensor_ = s; }
#endif

  // Starts a write of the embedded image. Returns false (and does nothing) if busy or no image.
  bool start();
  bool in_progress() const { return this->stage_ != Stage::IDLE && this->stage_ != Stage::DONE && this->stage_ != Stage::FAILED; }
  Stage stage() const { return this->stage_; }
  uint8_t progress() const { return this->progress_; }
  const std::string &last_error() const { return this->error_; }

  void add_on_success_callback(std::function<void()> &&cb) { this->success_callback_.add(std::move(cb)); }
  void add_on_failure_callback(std::function<void()> &&cb) { this->failure_callback_.add(std::move(cb)); }

 protected:
  // bus and flash access
  bool bus_acquire_();
  void bus_release_();
  void pins_to_input_();
  bool xfer_(const uint8_t *tx, uint8_t *rx, size_t len);
  bool command_(uint8_t cmd);
  bool read_status_(uint8_t cmd, uint8_t *value);
  bool write_enable_();
  bool wait_ready_(uint32_t timeout_ms);
  bool read_(uint32_t addr, uint8_t *dst, size_t len);
  bool program_page_(uint32_t addr, const uint8_t *src);
  bool erase_sector_(uint32_t addr);

  void step_();
  void fail_(const char *what, uint32_t addr);
  void finish_();
  void set_stage_(Stage stage);
  void publish_progress_(bool force);

  const uint8_t *image_{nullptr};
  size_t image_length_{0};
  const char *image_md5_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  uint8_t clk_pin_{3}, mosi_pin_{4}, miso_pin_{40}, cs_pin_{39};
  uint32_t frequency_{4000000};

  Stage stage_{Stage::IDLE};
  uint8_t progress_{0};
  std::string error_{};
  uint32_t sector_{0};
  uint32_t page_{0};
  uint32_t verify_pos_{0};
  bool erase_pending_{false};
  uint32_t erase_started_{0};
  uint32_t last_publish_{0};
  uint32_t started_ms_{0};
  bool tail_erased_{true};
  md5::MD5Digest md5_{};

  void *spi_device_{nullptr};  // spi_device_handle_t
  bool bus_initialised_{false};
  uint8_t *tx_buf_{nullptr};
  uint8_t *rx_buf_{nullptr};

#ifdef USE_SENSOR
  sensor::Sensor *progress_sensor_{nullptr};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *status_sensor_{nullptr};
#endif
  CallbackManager<void()> success_callback_{};
  CallbackManager<void()> failure_callback_{};
};

template<typename... Ts> class WriteAction : public Action<Ts...>, public Parented<XmosFlasher> {
 public:
  void play(const Ts &...x) override { this->parent_->start(); }
};

template<typename... Ts> class InProgressCondition : public Condition<Ts...>, public Parented<XmosFlasher> {
 public:
  bool check(const Ts &...x) override { return this->parent_->in_progress(); }
};

class SuccessTrigger : public Trigger<> {
 public:
  explicit SuccessTrigger(XmosFlasher *parent) {
    parent->add_on_success_callback([this]() { this->trigger(); });
  }
};

class FailureTrigger : public Trigger<> {
 public:
  explicit FailureTrigger(XmosFlasher *parent) {
    parent->add_on_failure_callback([this]() { this->trigger(); });
  }
};

}  // namespace xmos_flasher
}  // namespace esphome
