#!/bin/bash
# Restore most recent v2 web assets backup to the web server (requires SSH alias "webdeploy")
# Usage: undo_upload_v2_www.sh [output_dir]

set -euo pipefail

OUTPUT_DIR="${1:-../../_static/v2}"
REMOTE_DIR="webdeploy:/var/www/wordpress/v2/"
LATEST_LINK="$OUTPUT_DIR/backup-latest"

if [ -L "$LATEST_LINK" ]; then
  BACKUP_DIR="$OUTPUT_DIR/$(readlink "$LATEST_LINK")"
else
  # Fallback: pick most recent backup-* directory by name
  BACKUP_DIR="$(ls -1dt "$OUTPUT_DIR"/backup-* 2>/dev/null | head -n 1)"
  if [ -z "$BACKUP_DIR" ]; then
    echo "Error: No backups found in $OUTPUT_DIR"
    exit 1
  fi
fi

for file in "$BACKUP_DIR/www.js" "$BACKUP_DIR/www.js.br" "$BACKUP_DIR/www.js.gz"; do
  if [ ! -f "$file" ]; then
    echo "Error: Missing file in backup: $file"
    exit 1
  fi
done

echo "Restoring v2 web assets from $BACKUP_DIR to $REMOTE_DIR"
scp "$BACKUP_DIR/www.js" "$BACKUP_DIR/www.js.br" "$BACKUP_DIR/www.js.gz" "$REMOTE_DIR"
echo "Restore complete."
