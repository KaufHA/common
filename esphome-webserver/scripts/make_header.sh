#!/bin/bash
cat <<EOT > ./$1/$2
#pragma once
// Generated from https://github.com/KaufHA/common/tree/main/esphome-webserver
#include "esphome/core/hal.h"
namespace esphome {

namespace $3 {

EOT
echo "const uint8_t INDEX_GZ[] PROGMEM = {" >> ./$1/$2
xxd -cols 19 -i $1/index.html.gz | sed -e '2,$!d' -e 's/^/  /' -e '$d' | sed  -e '$d' | sed -e '$s/$/};/' >> ./$1/$2
cat <<EOT >> ./$1/$2

}  // namespace $3
}  // namespace esphome
EOT
ls -l ./$1/$2

if [ "web_server" = "$3" ]; then
    cp ./_static/v2/server_index.h ../components/web_server/
fi

if [ "captive_portal" = "$3" ]; then
    cp ./dist/captive_index.h ../../components/captive_portal/
fi
