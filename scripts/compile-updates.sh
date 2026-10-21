#!/usr/bin/env bash
set -uo pipefail

shopt -s nullglob

# TODO: This gzip 4-byte padding is a workaround for an ESPHome OTA bug that
# has since been fixed upstream. Safe to remove around 2028-03 (~18mo from
# 2026-09-20, when this was added).
pad_gzip_to_4byte() {
    python3 - "$1" <<'PYEOF'
import sys

filename = sys.argv[1]
with open(filename, 'rb') as f:
    data = bytearray(f.read())

if data[0] != 0x1f or data[1] != 0x8b:
    print(f"ERROR: {filename} is not a gzip file", file=sys.stderr)
    sys.exit(1)

current_size = len(data)
if current_size % 4 == 0:
    print(f"{filename}: already aligned ({current_size} bytes)")
    sys.exit(0)

flg = data[3]
if flg & 0x04:  # FEXTRA already set — extend existing extra field
    xlen = data[10] | (data[11] << 8)
    n = (4 - current_size % 4) % 4
    new_xlen = xlen + n
    data = (data[:10]
            + bytes([new_xlen & 0xff, (new_xlen >> 8) & 0xff])
            + data[12:12 + xlen]
            + bytes(n)
            + data[12 + xlen:])
else:           # No extra field yet — insert one
    n = (4 - (current_size + 2) % 4) % 4
    data[3] = flg | 0x04  # set FEXTRA bit
    data = (data[:10]
            + bytes([n, 0])   # XLEN = n
            + bytes(n)        # n zero-padding bytes
            + data[10:])

assert len(data) % 4 == 0
with open(filename, 'wb') as f:
    f.write(data)
print(f"{filename}: {current_size} -> {len(data)} bytes")
PYEOF
}

errors=0

if [ $# -gt 0 ]; then
    files=("$@")
else
    files=(./*.yaml)
fi

for yaml in "${files[@]}"; do
    base="$(basename "$yaml" .yaml)"
    echo "========================================"
    echo "Compiling: $base"
    echo "========================================"

    if ! esphome compile "$yaml"; then
        echo "ERROR: esphome compile failed for $base — skipping" >&2
        (( errors++ )) || true
        continue
    fi

    # Ask platformio (via esphome idedata) for the actual build directory, since it
    # differs by chip/framework: .pioenvs/<name>/ for ESP8266 and ESP32+Arduino,
    # build/ for ESP32+ESP-IDF (e.g. C2/C3).
    prog_path=$(esphome idedata "$yaml" 2>/dev/null | python3 -c "import json,sys; print(json.load(sys.stdin)['prog_path'])" 2>/dev/null)
    if [ -z "$prog_path" ]; then
        echo "ERROR: Could not determine build directory from idedata for $base — skipping" >&2
        (( errors++ )) || true
        continue
    fi
    build_dir="$(dirname "$prog_path")"

    # Both ESP8266 and ESP32 produce firmware.factory.bin (for first/factory flash)
    # and firmware.ota.bin (for OTA updates) alongside plain firmware.bin. On ESP32
    # factory.bin is a real merge of bootloader+partition table+app; on ESP8266 and
    # for ESP32's ota.bin, they're identical copies of firmware.bin.
    if [[ "$base" == *factory* ]] && [ -f "$build_dir/firmware.factory.bin" ]; then
        firmware_bin="$build_dir/firmware.factory.bin"
        bin_note="used firmware.factory.bin"
    elif [ -f "$build_dir/firmware.ota.bin" ]; then
        firmware_bin="$build_dir/firmware.ota.bin"
        bin_note="used firmware.ota.bin"
        if [[ "$base" == *factory* ]]; then
            bin_note+=" (name has 'factory' but no firmware.factory.bin was found)"
        fi
    elif [ -f "$build_dir/firmware.bin" ]; then
        firmware_bin="$build_dir/firmware.bin"
        bin_note="used firmware.bin"
        if [[ "$base" == *factory* ]]; then
            bin_note+=" (name has 'factory' but no firmware.factory.bin was found)"
        fi
    else
        echo "ERROR: No firmware bin found in $build_dir for $base — skipping" >&2
        (( errors++ )) || true
        continue
    fi

    dest_bin="./${base}.bin"
    cp "$firmware_bin" "$dest_bin"
    gzip -kf "$dest_bin"
    pad_gzip_to_4byte "${dest_bin}.gz"
    echo "Created: ${base}.bin ($bin_note from $build_dir)"
    echo "Created: ${base}.bin.gz"
done

echo "========================================"
if [ "$errors" -eq 0 ]; then
    echo "All done."
else
    echo "Done with $errors error(s)."
    exit 1
fi
