#!/usr/bin/env bash
# Run the hot-reloading dashboard against live, read-only Raspberry Pi data.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

DEV_GATEWAY_URL="${SMART_DOORBELL_DEV_GATEWAY_URL:-http://192.168.1.10:8080}"
DEV_HOST="${SMART_DOORBELL_DEV_HOST:-127.0.0.1}"
DEV_PORT_START="${SMART_DOORBELL_DEV_PORT:-3001}"

if ! [[ "$DEV_PORT_START" =~ ^[0-9]+$ ]] \
  || (( DEV_PORT_START < 1 || DEV_PORT_START > 65535 )); then
  echo "SMART_DOORBELL_DEV_PORT must be an integer between 1 and 65535." >&2
  exit 1
fi

port_is_available() {
  ! ss -H -ltn "sport = :$1" | grep -q .
}

DEV_PORT="$DEV_PORT_START"
while ! port_is_available "$DEV_PORT"; do
  if (( DEV_PORT == 65535 )); then
    echo "No available TCP port found at or above $DEV_PORT_START." >&2
    exit 1
  fi
  ((DEV_PORT += 1))
done

if ! curl --fail --silent --show-error --max-time 5 \
  "$DEV_GATEWAY_URL/api/events/sessions?size=1" >/dev/null; then
  echo "Could not reach the Raspberry Pi gateway at $DEV_GATEWAY_URL." >&2
  echo "Set SMART_DOORBELL_DEV_GATEWAY_URL to the correct gateway URL and retry." >&2
  exit 1
fi

export GATEWAY_URL="$DEV_GATEWAY_URL"
export GATEWAY_READ_ONLY=true
export NEXT_TELEMETRY_DISABLED=1

echo "Starting the hot-reloading dashboard at http://$DEV_HOST:$DEV_PORT"
echo "Using live Raspberry Pi data from $DEV_GATEWAY_URL (read-only proxy)"
if (( DEV_PORT != DEV_PORT_START )); then
  echo "Port $DEV_PORT_START was busy; selected the next available port: $DEV_PORT"
fi

cd "$PROJECT_DIR/dashboard"
exec npm run dev -- --hostname "$DEV_HOST" -p "$DEV_PORT"
