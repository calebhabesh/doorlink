#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

run_low_priority() {
  if command -v ionice >/dev/null 2>&1; then
    ionice -c2 -n7 nice -n 10 "$@"
  else
    nice -n 10 "$@"
  fi
}

echo "Building Smart Doorbell production artifacts under reduced CPU/IO priority."

echo
echo "[1/2] Packaging Spring Boot gateway..."
cd "$ROOT_DIR/gateway"
MAVEN_OPTS="-Xmx768m -XX:ActiveProcessorCount=2" run_low_priority ./mvnw -DskipTests package

echo
echo "[2/2] Building Next.js dashboard..."
cd "$ROOT_DIR/dashboard"
run_low_priority npm ci
NEXT_TELEMETRY_DISABLED=1 NODE_OPTIONS=--max-old-space-size=768 run_low_priority npm run build

echo
echo "Production artifacts are ready:"
echo "  gateway/target/gateway-0.0.1-SNAPSHOT.jar"
echo "  dashboard/.next"
