# SPDX-FileCopyrightText: 2019 ESPHome
# SPDX-FileCopyrightText: 2024-2026 Mischa Siekmann (FutureProofHomes, Satellite1-ESPHome)
# SPDX-FileCopyrightText: 2026 marceldale
# SPDX-License-Identifier: MIT
#
# ESPHome configuration for the XU316 boot-flash writer. The image embedding and the compile-time
# MD5 check follow FutureProofHomes/Satellite1-ESPHome, esphome/components/memory_flasher/__init__.py
# (MIT part of the ESPHome licence used there); remote images (HTTP) are deliberately not supported.
import hashlib
import re
from pathlib import Path

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import sensor, text_sensor
from esphome.const import (
    CONF_ID,
    CONF_RAW_DATA_ID,
    CONF_TRIGGER_ID,
    CONF_FREQUENCY,
    CONF_RESET_PIN,
    CONF_CLK_PIN,
    CONF_MOSI_PIN,
    CONF_MISO_PIN,
    CONF_CS_PIN,
    CONF_STATUS,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_PERCENT,
)
from esphome.core import CORE, HexInt

CODEOWNERS = ["@marceldale"]
AUTO_LOAD = ["md5", "sensor", "text_sensor"]

CONF_IMAGE_FILE = "image_file"
CONF_MD5_FILE = "md5_file"
CONF_PROGRESS = "progress"
CONF_ON_SUCCESS = "on_success"
CONF_ON_FAILURE = "on_failure"

xmos_flasher_ns = cg.esphome_ns.namespace("xmos_flasher")
XmosFlasher = xmos_flasher_ns.class_("XmosFlasher", cg.Component)
WriteAction = xmos_flasher_ns.class_("WriteAction", automation.Action, cg.Parented.template(XmosFlasher))
InProgressCondition = xmos_flasher_ns.class_(
    "InProgressCondition", automation.Condition, cg.Parented.template(XmosFlasher)
)
SuccessTrigger = xmos_flasher_ns.class_("SuccessTrigger", automation.Trigger.template())
FailureTrigger = xmos_flasher_ns.class_("FailureTrigger", automation.Trigger.template())

BOOT_PARTITION_SIZE = 0x100000  # must match flash_plan.h


def _md5_from_file(md5_file: Path, image_name: str) -> str:
    """Read the MD5 for image_name from an md5sum-style file (`<hex> *<name>` lines)."""
    for line in md5_file.read_text(encoding="utf-8").splitlines():
        m = re.match(r"^([0-9a-fA-F]{32})\s+\*?(.+)$", line.strip())
        if m and Path(m.group(2)).name == image_name:
            return m.group(1).lower()
    raise cv.Invalid(f"{md5_file} has no 32-digit MD5 line for {image_name}")


def _validate_image(config):
    image = Path(CORE.relative_config_path(config[CONF_IMAGE_FILE]))
    md5_file = Path(CORE.relative_config_path(config[CONF_MD5_FILE]))
    expected = _md5_from_file(md5_file, image.name)
    data = image.read_bytes()
    actual = hashlib.md5(data).hexdigest()
    if actual != expected:
        raise cv.Invalid(f"{image.name}: MD5 {actual} does not match {expected} from {md5_file.name}")
    if not 0 < len(data) <= BOOT_PARTITION_SIZE:
        raise cv.Invalid(f"{image.name}: {len(data)} bytes do not fit the {BOOT_PARTITION_SIZE}-byte boot partition")
    return config


def _internal_gpio_number(value):
    return pins.internal_gpio_output_pin_number(value)


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(XmosFlasher),
            cv.Required(CONF_IMAGE_FILE): cv.file_,
            cv.Required(CONF_MD5_FILE): cv.file_,
            cv.GenerateID(CONF_RAW_DATA_ID): cv.declare_id(cg.uint8),
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_CLK_PIN, default=3): _internal_gpio_number,
            cv.Optional(CONF_MOSI_PIN, default=4): _internal_gpio_number,
            cv.Optional(CONF_MISO_PIN, default=40): pins.internal_gpio_input_pin_number,
            cv.Optional(CONF_CS_PIN, default=39): _internal_gpio_number,
            # W25Q32JV allows 50 MHz for 03h; start low because CLK/D0 share long tracks with the XU316.
            cv.Optional(CONF_FREQUENCY, default="4MHz"): cv.All(cv.frequency, cv.int_range(min=100000, max=8000000)),
            cv.Optional(CONF_PROGRESS): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:progress-upload",
            ),
            cv.Optional(CONF_STATUS): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:chip",
            ),
            cv.Optional(CONF_ON_SUCCESS): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SuccessTrigger)}
            ),
            cv.Optional(CONF_ON_FAILURE): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FailureTrigger)}
            ),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate_image,
    cv.only_on_esp32,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    image = Path(CORE.relative_config_path(config[CONF_IMAGE_FILE]))
    md5 = _md5_from_file(Path(CORE.relative_config_path(config[CONF_MD5_FILE])), image.name)
    data = image.read_bytes()
    arr = cg.progmem_array(config[CONF_RAW_DATA_ID], [HexInt(b) for b in data])
    cg.add(var.set_image(arr, len(data), md5))

    pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(pin))
    cg.add(
        var.set_spi_pins(config[CONF_CLK_PIN], config[CONF_MOSI_PIN], config[CONF_MISO_PIN], config[CONF_CS_PIN])
    )
    cg.add(var.set_frequency(int(config[CONF_FREQUENCY])))

    if conf := config.get(CONF_PROGRESS):
        s = await sensor.new_sensor(conf)
        cg.add(var.set_progress_sensor(s))
    if conf := config.get(CONF_STATUS):
        t = await text_sensor.new_text_sensor(conf)
        cg.add(var.set_status_text_sensor(t))

    for conf in config.get(CONF_ON_SUCCESS, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_FAILURE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)


FLASHER_ACTION_SCHEMA = automation.maybe_simple_id({cv.GenerateID(): cv.use_id(XmosFlasher)})


@automation.register_action("xmos_flasher.write", WriteAction, FLASHER_ACTION_SCHEMA, synchronous=True)
async def write_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_condition("xmos_flasher.in_progress", InProgressCondition, FLASHER_ACTION_SCHEMA)
async def in_progress_to_code(config, condition_id, template_arg, args):
    var = cg.new_Pvariable(condition_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
