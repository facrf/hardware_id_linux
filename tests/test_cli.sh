#!/bin/sh
set -eu

program=$1

# 1. Testes de JSON (Redacted e Unredacted)
"$program" --json | python3 -c '
import json, sys
data = json.load(sys.stdin)
assert data["hardware_id"]["fingerprint_sha256"] == "<redacted>"
assert "virtualization" in data["system"]
assert "temp_celsius" in data["cpu"]
'

"$program" --json --include-identifiers | python3 -c '
import json, sys
data = json.load(sys.stdin)
assert len(data["hardware_id"]["fingerprint_sha256"]) == 64
'

# 2. Testes de flags diretas (--only-id e --uuid)
id_out=$("$program" --only-id)
assert_len=$(echo "$id_out" | awk '{ print length($0) }')
if [ "$assert_len" -ne 64 ]; then
    echo "Expected 64 characters for --only-id, got $assert_len" >&2
    exit 1
fi

uuid_out=$("$program" --uuid)
echo "$uuid_out" | python3 -c '
import sys, re
val = sys.stdin.read().strip()
pattern = r"^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"
assert re.match(pattern, val), f"Invalid UUID: {val}"
'

# 3. Teste de exportação ENV (--env / -e)
eval "$("$program" --env --include-identifiers)"
if [ -z "${HW_ID:-}" ] || [ ${#HW_ID} -ne 64 ]; then
    echo "HW_ID missing or invalid from --env" >&2
    exit 1
fi
if [ -z "${HW_DISTRO_NAME:-}" ]; then
    echo "HW_DISTRO_NAME missing from --env" >&2
    exit 1
fi

# 4. Teste de modo rápido (--fast) e modo ASCII (--ascii)
"$program" --fast --ascii --no-color >/dev/null

# 5. Teste de opção desconhecida
if "$program" --invalid-option >/dev/null 2>&1; then
    echo "unknown option was accepted" >&2
    exit 1
fi

echo "CLI, Flags, ENV and JSON: OK"
