import logging

import esphome.config_validation as cv

LOGGER = logging.getLogger(__name__)

CONF_YAML_FILE = "yaml_file"
YAML_FILE_TRIGGER_RGBS_1M     = "__KAUF_RGBS_LEGACY_YAML_1M__"
YAML_FILE_TRIGGER_RGBS_4M     = "__KAUF_RGBS_LEGACY_YAML_4M__"
YAML_FILE_TRIGGER_RGBS_1M_LITE = "__KAUF_RGBS_LEGACY_YAML_1M_LITE__"
YAML_FILE_TRIGGER_RGBS_4M_LITE = "__KAUF_RGBS_LEGACY_YAML_4M_LITE__"
YAML_FILE_TRIGGER_RGBS_1M_MIN  = "__KAUF_RGBS_LEGACY_YAML_1M_MIN__"
YAML_FILE_TRIGGER_RGBS_4M_MIN  = "__KAUF_RGBS_LEGACY_YAML_4M_MIN__"

CONF_DISABLE_WEBSERVER = "disable_webserver"
CONF_LIGHT_RESTORE_MODE = "light_restore_mode"
CONF_DEFAULT_BUTTON_CONFIG = "default_button_config"
CONF_SUB_REBOOT_TIMEOUT = "sub_reboot_timeout"
CONF_SUB_API_REBOOT_TIMEOUT = "sub_api_reboot_timeout"
CONF_SUB_OTA_NUM_ATTEMPTS = "sub_ota_num_attempts"
CONF_SUB_DEFAULT_TRANSITION_LENGTH = "sub_default_transition_length"
CONF_SUB_WARM_WHITE_TEMP = "sub_warm_white_temp"
CONF_SUB_COLD_WHITE_TEMP = "sub_cold_white_temp"
CONF_SUB_CW_FREQ = "sub_cw_freq"
CONF_SUB_WW_FREQ = "sub_ww_freq"
CONF_HASS_CT_MIREDS_ENTITY = "hass_ct_mireds_entity"
CONF_HASS_CT_MIREDS_ATTRIBUTE = "hass_ct_mireds_attribute"
CONF_HASS_CT_MIREDS_FIXED = "hass_ct_mireds_fixed"
SENTINEL = "__KAUF_DEPRECATED_SENTINEL__"
ERROR_SENTINEL = "__KAUF_ERROR_SENTINEL__"


def _any_value(value):
    return value


# Added: 2026-03-10. Switch to cv.Invalid after 2026-09-10. Delete file after 2027-03-10.
def _validate_yaml_file(value):
    if value == YAML_FILE_TRIGGER_RGBS_1M:
        LOGGER.warning(
            "\n"
            "============================================================\n"
            "kauf-rgbs.yaml is deprecated. Please migrate by updating\n"
            "your yaml file:\n"
            "\n"
            "---- CHANGE THIS: ----\n"
            "packages:\n"
            "  Kauf.RGBSw: github://KaufHA/kauf-rgb-switch/kauf-rgbs.yaml\n"
            "\n"
            "---- TO THIS: ----\n"
            "packages:\n"
            "  Kauf.RGBSw: github://KaufHA/kauf-rgb-switch/packages/kauf-srf10-1m.yaml\n"
            "\n"
            "See: https://github.com/KaufHA/kauf-rgb-switch#yaml-migration\n"
            "============================================================"
        )
    elif value == YAML_FILE_TRIGGER_RGBS_4M:
        LOGGER.warning(
            "\n"
            "============================================================\n"
            "kauf-rgbs-4m.yaml is deprecated. Please migrate by updating\n"
            "your yaml file:\n"
            "\n"
            "---- CHANGE THIS: ----\n"
            "packages:\n"
            "  Kauf.RGBSw: github://KaufHA/kauf-rgb-switch/kauf-rgbs-4m.yaml\n"
            "\n"
            "---- TO THIS: ----\n"
            "packages:\n"
            "  Kauf.RGBSw: github://KaufHA/kauf-rgb-switch/packages/kauf-srf10-4m.yaml\n"
            "\n"
            "See: https://github.com/KaufHA/kauf-rgb-switch#yaml-migration\n"
            "============================================================"
        )
    elif value == YAML_FILE_TRIGGER_RGBS_1M_LITE:
        LOGGER.warning(
            "\n"
            "============================================================\n"
            "kauf-rgbs-lite.yaml is deprecated. Please migrate by updating\n"
            "your yaml file:\n"
            "\n"
            "---- CHANGE THIS: ----\n"
            "packages:\n"
            "  Kauf.RGBSw: github://KaufHA/kauf-rgb-switch/config/kauf-rgbs-lite.yaml\n"
            "\n"
            "---- TO THIS: ----\n"
            "packages:\n"
            "  profile: github://KaufHA/kauf-rgb-switch/packages/profiles/stock-default.yaml\n"
            "  target:  github://KaufHA/kauf-rgb-switch/packages/targets/stock-srf10-1m-target.yaml\n"
            "\n"
            "See: https://github.com/KaufHA/kauf-rgb-switch#yaml-migration\n"
            "============================================================"
        )
    elif value == YAML_FILE_TRIGGER_RGBS_4M_LITE:
        LOGGER.warning(
            "\n"
            "============================================================\n"
            "kauf-rgbs-4m-lite.yaml is deprecated. Please migrate by updating\n"
            "your yaml file:\n"
            "\n"
            "---- CHANGE THIS: ----\n"
            "packages:\n"
            "  Kauf.RGBSw: github://KaufHA/kauf-rgb-switch/config/kauf-rgbs-4m-lite.yaml\n"
            "\n"
            "---- TO THIS: ----\n"
            "packages:\n"
            "  profile: github://KaufHA/kauf-rgb-switch/packages/profiles/stock-default.yaml\n"
            "  target:  github://KaufHA/kauf-rgb-switch/packages/targets/stock-srf10-4m-target.yaml\n"
            "\n"
            "See: https://github.com/KaufHA/kauf-rgb-switch#yaml-migration\n"
            "============================================================"
        )
    elif value == YAML_FILE_TRIGGER_RGBS_1M_MIN:
        LOGGER.warning(
            "\n"
            "============================================================\n"
            "kauf-rgbs-minimal.yaml is deprecated. Please migrate by updating\n"
            "your yaml file:\n"
            "\n"
            "---- CHANGE THIS: ----\n"
            "packages:\n"
            "  Kauf.RGBSw: github://KaufHA/kauf-rgb-switch/config/kauf-rgbs-minimal.yaml\n"
            "\n"
            "---- TO THIS: ----\n"
            "packages:\n"
            "  profile: github://KaufHA/kauf-rgb-switch/packages/profiles/stock-minimal.yaml\n"
            "  target:  github://KaufHA/kauf-rgb-switch/packages/targets/stock-srf10-1m-target.yaml\n"
            "\n"
            "See: https://github.com/KaufHA/kauf-rgb-switch#yaml-migration\n"
            "============================================================"
        )
    elif value == YAML_FILE_TRIGGER_RGBS_4M_MIN:
        LOGGER.warning(
            "\n"
            "============================================================\n"
            "kauf-rgbs-4m-minimal.yaml is deprecated. Please migrate by updating\n"
            "your yaml file:\n"
            "\n"
            "---- CHANGE THIS: ----\n"
            "packages:\n"
            "  Kauf.RGBSw: github://KaufHA/kauf-rgb-switch/config/kauf-rgbs-4m-minimal.yaml\n"
            "\n"
            "---- TO THIS: ----\n"
            "packages:\n"
            "  profile: github://KaufHA/kauf-rgb-switch/packages/profiles/stock-minimal.yaml\n"
            "  target:  github://KaufHA/kauf-rgb-switch/packages/targets/stock-srf10-4m-target.yaml\n"
            "\n"
            "See: https://github.com/KaufHA/kauf-rgb-switch#yaml-migration\n"
            "============================================================"
        )
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_disable_webserver(value):
    if value != SENTINEL:
        raise cv.Invalid(
            f"'{CONF_DISABLE_WEBSERVER}' has been removed. Use '!remove' instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "web_server: !remove\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#disable_webserver"
        )
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_light_restore_mode(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_LIGHT_RESTORE_MODE}' has been removed. Use '!extend' instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "light:\n"
            "  - id: !extend kauf_light\n"
            f"    restore_mode: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#light_restore_mode"
        )
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_default_button_config(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_DEFAULT_BUTTON_CONFIG}' has been removed. Use '!extend' instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "select:\n"
            "  - id: !extend select_button\n"
            f"    initial_option: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#default_button_config"
        )
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_sub_reboot_timeout(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_SUB_REBOOT_TIMEOUT}' has been removed. Use native ESPHome config instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "api:\n"
            f"  reboot_timeout: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#sub_reboot_timeout"
        )
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_sub_api_reboot_timeout(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_SUB_API_REBOOT_TIMEOUT}' has been removed. Use native ESPHome config instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "api:\n"
            f"  reboot_timeout: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#sub_api_reboot_timeout"
        )
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_sub_ota_num_attempts(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_SUB_OTA_NUM_ATTEMPTS}' has been removed. Use native ESPHome config instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "safe_mode:\n"
            f"  num_attempts: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#sub_ota_num_attempts"
        )
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_sub_default_transition_length(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_SUB_DEFAULT_TRANSITION_LENGTH}' has been removed. Use '!extend' instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "light:\n"
            "  - id: !extend kauf_light\n"
            f"    default_transition_length: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#sub_default_transition_length"
        )
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_sub_warm_white_temp(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_SUB_WARM_WHITE_TEMP}' has been removed. Use '!extend' instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "light:\n"
            "  - id: !extend kauf_light\n"
            f"    warm_white_color_temperature: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#sub_warm_white_temp"
        )
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_sub_cold_white_temp(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_SUB_COLD_WHITE_TEMP}' has been removed. Use '!extend' instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "light:\n"
            "  - id: !extend kauf_light\n"
            f"    cold_white_color_temperature: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#sub_cold_white_temp"
        )
    return value

# added 2026-03-08
def _validate_sub_cw_freq(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_SUB_CW_FREQ}' and '{CONF_SUB_WW_FREQ}' have been removed and combined into 'sub_cwww_freq'. "
            "Remove old substitutions and add the following YAML.\n"
            "\n"
            "substitutions:\n"
            f"  sub_cwww_freq: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#sub_cw_freq--sub_ww_freq"
        )
    return value


# added 2026-03-08
def _validate_sub_ww_freq(value):
    if value != SENTINEL:
        provided_value = str(value)
        raise cv.Invalid(
            f"'{CONF_SUB_CW_FREQ}' and '{CONF_SUB_WW_FREQ}' have been removed and combined into 'sub_cwww_freq'. "
            "Remove old substitutions and add the following YAML.\n"
            "\n"
            "substitutions:\n"
            f"  sub_cwww_freq: {provided_value}\n"
            "\n"
            "More details: https://github.com/KaufHA/common/blob/main/DEPRECATED_SUBSTITUTIONS.md#sub_cw_freq--sub_ww_freq"
        )
    return value


# added 2026-03-08
def _validate_hass_ct_mireds_entity(value):
    if value != SENTINEL:
        raise cv.Invalid(
            "Home Assistant no longer supports mired color temperature options. Use Kelvin instead."
        )
    return value


# added 2026-03-08
def _validate_hass_ct_mireds_attribute(value):
    if value != SENTINEL:
        raise cv.Invalid(
            "Home Assistant no longer supports mired color temperature options. Use Kelvin instead."
        )
    return value


# added 2026-03-08
def _validate_hass_ct_mireds_fixed(value):
    if value != SENTINEL:
        raise cv.Invalid(
            "Home Assistant no longer supports mired color temperature options. Use Kelvin instead."
        )
    return value


CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_YAML_FILE, default=SENTINEL): cv.All(
            _any_value, _validate_yaml_file
        ),
        cv.Optional(CONF_DISABLE_WEBSERVER, default=SENTINEL): cv.All(
            _any_value, _validate_disable_webserver
        ),
        cv.Optional(CONF_LIGHT_RESTORE_MODE, default=SENTINEL): cv.All(
            _any_value, _validate_light_restore_mode
        ),
        cv.Optional(CONF_DEFAULT_BUTTON_CONFIG, default=SENTINEL): cv.All(
            _any_value, _validate_default_button_config
        ),
        cv.Optional(CONF_SUB_REBOOT_TIMEOUT, default=SENTINEL): cv.All(
            _any_value, _validate_sub_reboot_timeout
        ),
        cv.Optional(CONF_SUB_API_REBOOT_TIMEOUT, default=SENTINEL): cv.All(
            _any_value, _validate_sub_api_reboot_timeout
        ),
        cv.Optional(CONF_SUB_OTA_NUM_ATTEMPTS, default=SENTINEL): cv.All(
            _any_value, _validate_sub_ota_num_attempts
        ),
        cv.Optional(CONF_SUB_DEFAULT_TRANSITION_LENGTH, default=SENTINEL): cv.All(
            _any_value, _validate_sub_default_transition_length
        ),
        cv.Optional(CONF_SUB_WARM_WHITE_TEMP, default=SENTINEL): cv.All(
            _any_value, _validate_sub_warm_white_temp
        ),
        cv.Optional(CONF_SUB_COLD_WHITE_TEMP, default=SENTINEL): cv.All(
            _any_value, _validate_sub_cold_white_temp
        ),
        cv.Optional(CONF_SUB_CW_FREQ, default=SENTINEL): cv.All(
            _any_value, _validate_sub_cw_freq
        ),
        cv.Optional(CONF_SUB_WW_FREQ, default=SENTINEL): cv.All(
            _any_value, _validate_sub_ww_freq
        ),
        cv.Optional(CONF_HASS_CT_MIREDS_ENTITY, default=SENTINEL): cv.All(
            _any_value, _validate_hass_ct_mireds_entity
        ),
        cv.Optional(CONF_HASS_CT_MIREDS_ATTRIBUTE, default=SENTINEL): cv.All(
            _any_value, _validate_hass_ct_mireds_attribute
        ),
        cv.Optional(CONF_HASS_CT_MIREDS_FIXED, default=SENTINEL): cv.All(
            _any_value, _validate_hass_ct_mireds_fixed
        ),
    },
    extra=cv.ALLOW_EXTRA,
)


async def to_code(config):
    return
