# Deprecated Substitutions

Some legacy substitutions were removed in favor of ESPHome package overrides with `!extend` and `!remove`.

If your build fails with a deprecation error, move the old substitution value to an override block.

## Why this changed

`!extend` and `!remove` are clearer than behavioral substitutions and make upstream package updates easier.
They also use ESPHome's native option names and values directly, instead of requiring a second layer of device-specific substitution names.

## Migration pattern

Generic old style:

```yaml
substitutions:
  sub_some_option: some_value
```

Generic new style:

```yaml
component:
  - id: !extend some_id
    some_option: some_value
```

## Usage Tips

If you are overriding multiple fields on the same entity, combine them into a single `!extend` block, e.g.:

```yaml
light:
  - id: !extend kauf_light
    restore_mode: RESTORE_DEFAULT_ON
    default_transition_length: 500ms
    warm_white_color_temperature: 3000 Kelvin
    cold_white_color_temperature: 6500 Kelvin
```

## All Devices

### disable_webserver

Old style:

```yaml
substitutions:
  disable_webserver: "true"
```

New style:

```yaml
web_server: !remove
```

## Plugs (PLF10 and PLF12)

### default_button_config

Old style:

```yaml
substitutions:
  default_button_config: "Toggle on Release"
```

New style:

```yaml
select:
  - id: !extend select_button
    initial_option: Toggle on Release
```

### sub_reboot_timeout

Old style:

```yaml
substitutions:
  sub_reboot_timeout: 30s
```

New style:

```yaml
api:
  reboot_timeout: 30s
```

## RGBWW Bulb

### light_restore_mode

Old style:

```yaml
substitutions:
  light_restore_mode: RESTORE_DEFAULT_OFF
```

New style:

```yaml
light:
  - id: !extend kauf_light
    restore_mode: RESTORE_DEFAULT_OFF
```

### sub_api_reboot_timeout

Old style:

```yaml
substitutions:
  sub_api_reboot_timeout: 30s
```

New style:

```yaml
api:
  reboot_timeout: 30s
```

### sub_ota_num_attempts

Old style:

```yaml
substitutions:
  sub_ota_num_attempts: "20"
```

New style:

```yaml
safe_mode:
  num_attempts: 20
```

### sub_default_transition_length

Old style:

```yaml
substitutions:
  sub_default_transition_length: 500ms
```

New style:

```yaml
light:
  - id: !extend kauf_light
    default_transition_length: 500ms
```

### sub_warm_white_temp

Old style:

```yaml
substitutions:
  sub_warm_white_temp: 3000 Kelvin
```

New style:

```yaml
light:
  - id: !extend kauf_light
    warm_white_color_temperature: 3000 Kelvin
```

### sub_cold_white_temp

Old style:

```yaml
substitutions:
  sub_cold_white_temp: 6500 Kelvin
```

New style:

```yaml
light:
  - id: !extend kauf_light
    cold_white_color_temperature: 6500 Kelvin
```

### sub_cw_freq / sub_ww_freq

Old style:

```yaml
substitutions:
  sub_cw_freq: 125 hz
  sub_ww_freq: 125 hz
```

New style:

```yaml
substitutions:
  sub_cwww_freq: 125 hz
```

## RGB Switch

Device-specific deprecation entries will be added here as substitutions are retired.

## Notes

- Keep your override in your local device YAML (the one that imports the package).
- Only override what you need.
- Use `!remove` when you want to remove an entire component from a package.

Example:

```yaml
web_server: !remove
```
