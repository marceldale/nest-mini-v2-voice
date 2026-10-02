// Modified 2026-10-02 by marceldale for nest-mini-v2-voice: the start-up register table now
// ends in Hi-Z and the control state is initialised, so PLAY follows PBTL and the analog gain;
// the class-D loop bandwidth is 175 kHz (register 0x53) as the datasheet asks for Fsw = 768 kHz.
// Original: https://github.com/mrtoy-me/esphome-tas58xx @ 89f29cf. See firmware/esphome/PATCH.md.
#pragma once

#include "tas58xx.h"

namespace esphome::tas58xx {

struct Tas58xxConfiguration {
    uint8_t addr;
    uint8_t value;
  }__attribute__((packed));

// Startup sequence flag
static constexpr uint8_t TAS58XX_CFG_META_DELAY = 254;

static constexpr Tas58xxConfiguration TAS58XX_CONFIG[] = {
// RESET
    { 0x00, 0x00 }, //
    { 0x7f, 0x00 },
    { 0x03, 0x02 },
    { 0x01, 0x11 },
    { 0x03, 0x02 },
    { TAS58XX_CFG_META_DELAY, 5 },
    { 0x03, 0x00 },
    { 0x46, 0x01 }, // undocumented SR 96KHz; for 48KHz use 0x11
    { 0x03, 0x02 },
    { 0x61, 0x0b },
    { 0x60, 0x01 },
    { 0x7d, 0x11 },
    { 0x7e, 0xff },
    { 0x00, 0x01 },
    { 0x51, 0x05 },
// Register Tuning
    { 0x00, 0x00 },
    { 0x7f, 0x00 },
    { 0x02, 0x00 },
    { 0x30, 0x00 },
    { 0x4c, 0x30 },
    { 0x53, 0x60 }, // PATCH nest-mini-v2-voice (U14-B2): ANA_CTRL loop bandwidth 175 kHz; SLASEH5D
                    // table 7-27: "With Fsw=768kHz, 175kHz bandwidth should be selected" (0x02 FSW_SEL
                    // = 000 = 768 kHz here); BD rule Fsw >= 3 x BW (table 7-3): 768 >= 525 kHz
    { 0x54, 0x00 }, // analog gain 0db
    { 0x03, 0x02 }, // PATCH nest-mini-v2-voice (K-B4): stay in Hi-Z here; PLAY is set in
                    // configure_registers_() only after dac_mode (PBTL) and analog gain
    { 0x78, 0x80 },
};

}  // namespace esphome::tas58xx
