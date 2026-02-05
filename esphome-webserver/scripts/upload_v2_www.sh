#!/bin/bash
# Upload built v2 web assets to the web server (requires SSH alias "webdeploy")
# Usage: upload_v2_www.sh [output_dir]

set -euo pipefail

OUTPUT_DIR="${1:-../../_static/v2}"
REMOTE_DIR="webdeploy:/var/www/wordpress/v2/"

for file in "$OUTPUT_DIR/www.js" "$OUTPUT_DIR/www.js.br" "$OUTPUT_DIR/www.js.gz"; do
  if [ ! -f "$file" ]; then
    echo "Error: Missing file: $file"
    exit 1
  fi
done

echo "Uploading v2 web assets from $OUTPUT_DIR to $REMOTE_DIR"
scp "$OUTPUT_DIR/www.js" "$OUTPUT_DIR/www.js.br" "$OUTPUT_DIR/www.js.gz" "$REMOTE_DIR"
echo "Upload complete."
