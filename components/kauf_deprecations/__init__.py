import esphome.config_validation as cv

CONF_DISABLE_WEBSERVER = "disable_webserver"
CONF_LIGHT_RESTORE_MODE = "light_restore_mode"
CONF_DEFAULT_BUTTON_CONFIG = "default_button_config"
CONF_SUB_REBOOT_TIMEOUT = "sub_reboot_timeout"
CONF_SUB_API_REBOOT_TIMEOUT = "sub_api_reboot_timeout"
CONF_SUB_OTA_NUM_ATTEMPTS = "sub_ota_num_attempts"
CONF_SUB_DEFAULT_TRANSITION_LENGTH = "sub_default_transition_length"
CONF_SUB_WARM_WHITE_TEMP = "sub_warm_white_temp"
CONF_SUB_COLD_WHITE_TEMP = "sub_cold_white_temp"
SENTINEL = "__KAUF_DEPRECATED_SENTINEL__"


def _any_value(value):
    return value


# Added: 2026-02-18. Keep deprecation guard until at least 2027-02-18.
def _validate_disable_webserver(value):
    if value != SENTINEL:
        raise cv.Invalid(
            f"'{CONF_DISABLE_WEBSERVER}' has been removed. Use '!remove' instead by removing the substitution and adding the following YAML.\n"
            "\n"
            "web_server: !remove\n"
            "\n"
            "More details: https://github.com/KaufHA/common/DEPRECATED_SUBSTITUTIONS.md#disable_webserver"
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
            "More details: https://github.com/KaufHA/common/DEPRECATED_SUBSTITUTIONS.md#light_restore_mode"
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
            "More details: https://github.com/KaufHA/common/DEPRECATED_SUBSTITUTIONS.md#default_button_config"
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
            "More details: https://github.com/KaufHA/common/DEPRECATED_SUBSTITUTIONS.md#sub_reboot_timeout"
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
            "More details: https://github.com/KaufHA/common/DEPRECATED_SUBSTITUTIONS.md#sub_api_reboot_timeout"
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
            "More details: https://github.com/KaufHA/common/DEPRECATED_SUBSTITUTIONS.md#sub_ota_num_attempts"
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
            "More details: https://github.com/KaufHA/common/DEPRECATED_SUBSTITUTIONS.md#sub_default_transition_length"
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
            "More details: https://github.com/KaufHA/common/DEPRECATED_SUBSTITUTIONS.md#sub_warm_white_temp"
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
            "More details: https://github.com/KaufHA/common/DEPRECATED_SUBSTITUTIONS.md#sub_cold_white_temp"
        )
    return value


CONFIG_SCHEMA = cv.Schema(
    {
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
    },
    extra=cv.ALLOW_EXTRA,
)


async def to_code(config):
    return
