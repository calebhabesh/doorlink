#!/usr/bin/env bash
set -euo pipefail

LOG_DIR="${1:-/tmp/smart-doorbell-diagnostics}"
mkdir -p "$LOG_DIR"

OUT="$LOG_DIR/cpu-snapshot-$(date +%Y%m%d-%H%M%S).log"

{
  echo "=== timestamp ==="
  date -Is

  echo
  echo "=== uptime/load ==="
  uptime

  echo
  echo "=== memory ==="
  free -h

  echo
  echo "=== root filesystem ==="
  df -h /

  echo
  echo "=== hottest processes ==="
  ps -eo pid,ppid,ni,pcpu,pmem,comm,args --sort=-pcpu | head -n 30

  echo
  echo "=== smart doorbell systemd services ==="
  systemctl --no-pager --plain status smart-doorbell-gateway.service smart-doorbell-dashboard.service || true

  if command -v docker >/dev/null 2>&1; then
    echo
    echo "=== docker stats ==="
    docker stats --no-stream || true
  fi

  echo
  echo "=== recent gateway logs ==="
  journalctl --no-pager -u smart-doorbell-gateway.service --since "-10 minutes" -n 120 || true

  echo
  echo "=== recent dashboard logs ==="
  journalctl --no-pager -u smart-doorbell-dashboard.service --since "-10 minutes" -n 120 || true
} > "$OUT" 2>&1

echo "$OUT"
