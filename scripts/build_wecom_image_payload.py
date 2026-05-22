#!/usr/bin/env python3

import base64
import hashlib
import json
import pathlib
import sys


def main() -> int:
    if len(sys.argv) != 3:
        return 2

    image_path = pathlib.Path(sys.argv[1])
    payload_path = pathlib.Path(sys.argv[2])

    image_bytes = image_path.read_bytes()
    payload = {
        "msgtype": "image",
        "image": {
            "base64": base64.b64encode(image_bytes).decode("ascii"),
            "md5": hashlib.md5(image_bytes).hexdigest(),
        },
    }
    payload_path.write_text(json.dumps(payload, separators=(",", ":")), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
