#!/bin/sh
set -eu

program=$1

"$program" --json | python3 -c 'import json, sys; data = json.load(sys.stdin); assert data["hardware_id"]["fingerprint_sha256"] == "<redacted>"'
"$program" --json --include-identifiers | python3 -c 'import json, sys; data = json.load(sys.stdin); assert len(data["hardware_id"]["fingerprint_sha256"]) == 64'
if "$program" --invalid-option >/dev/null 2>&1; then
    echo "unknown option was accepted" >&2
    exit 1
fi

echo "CLI and JSON: OK"
