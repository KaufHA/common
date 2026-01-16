import os
from pathlib import Path

import esphome
import esphome.codegen as cg

# Stock ESPHome template component path
_esphome_root = Path(esphome.__file__).parent
_stock_template_path = str(_esphome_root / "components" / "template")

# Custom template folder (this file's directory)
_custom_path = os.path.dirname(__file__)

# Search custom first, then fall back to stock for submodules
__path__ = [_custom_path, _stock_template_path]

# Same namespace as stock template component
template_ns = cg.esphome_ns.namespace("template_")

CODEOWNERS = ["@esphome/core"]
