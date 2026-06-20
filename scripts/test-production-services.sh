#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GATEWAY_SERVICE="$ROOT_DIR/scripts/systemd/smart-doorbell-gateway.service"
DASHBOARD_SERVICE="$ROOT_DIR/scripts/systemd/smart-doorbell-dashboard.service"
PI_BUILD_SCRIPT="$ROOT_DIR/scripts/build-pi-production.sh"
DASHBOARD_LAYOUT="$ROOT_DIR/dashboard/src/app/layout.tsx"

failures=0

assert_contains() {
  local file="$1"
  local pattern="$2"
  local description="$3"

  if [[ ! -f "$file" ]]; then
    echo "FAIL: $description ($file is missing)"
    failures=$((failures + 1))
    return
  fi

  if grep -Eq "$pattern" "$file"; then
    echo "OK: $description"
  else
    echo "FAIL: $description"
    failures=$((failures + 1))
  fi
}

assert_not_contains() {
  local file="$1"
  local pattern="$2"
  local description="$3"

  if [[ ! -f "$file" ]]; then
    echo "FAIL: $description ($file is missing)"
    failures=$((failures + 1))
    return
  fi

  if grep -Eq "$pattern" "$file"; then
    echo "FAIL: $description"
    failures=$((failures + 1))
  else
    echo "OK: $description"
  fi
}

assert_contains "$GATEWAY_SERVICE" '^ExecStart=/usr/bin/java .*-jar /home/ethioprince/dev/smart-doorbell/gateway/target/gateway-[^/]+\.jar$' "gateway service runs the packaged Spring Boot jar"
assert_not_contains "$GATEWAY_SERVICE" 'spring-boot:run|/mvnw|maven' "gateway service does not run Maven or spring-boot:run"
assert_contains "$GATEWAY_SERVICE" '^Nice=' "gateway service lowers scheduler priority"
assert_not_contains "$GATEWAY_SERVICE" '^(CPUQuota|CPUWeight|MemoryHigh|MemoryMax|ProtectSystem|ProtectHome|ReadWritePaths|PrivateTmp|NoNewPrivileges)=' "gateway service avoids Pi-incompatible resource/sandbox directives"

assert_contains "$DASHBOARD_SERVICE" '^ExecStart=/usr/bin/npm run start -- -p 3000$' "dashboard service runs Next.js production start"
assert_contains "$DASHBOARD_SERVICE" '^Environment=NODE_ENV=production$' "dashboard service uses production NODE_ENV"
assert_contains "$DASHBOARD_SERVICE" '^Environment=NEXT_TELEMETRY_DISABLED=1$' "dashboard service disables Next.js telemetry"
assert_not_contains "$DASHBOARD_SERVICE" 'npm run dev|next dev|NODE_ENV=development' "dashboard service does not run the Next.js dev server"
assert_contains "$DASHBOARD_SERVICE" '^Nice=' "dashboard service lowers scheduler priority"
assert_not_contains "$DASHBOARD_SERVICE" '^(CPUQuota|CPUWeight|MemoryHigh|MemoryMax|ProtectSystem|ProtectHome|ReadWritePaths|PrivateTmp|NoNewPrivileges)=' "dashboard service avoids Pi-incompatible resource/sandbox directives"

assert_contains "$PI_BUILD_SCRIPT" '^\s*run_low_priority npm ci$' "Pi build script installs dashboard dependencies reproducibly"
assert_contains "$PI_BUILD_SCRIPT" '^\s*NEXT_TELEMETRY_DISABLED=1 NODE_OPTIONS=--max-old-space-size=768 run_low_priority npm run build$' "Pi build script creates the Next.js production build at reduced priority"
assert_contains "$PI_BUILD_SCRIPT" '^\s*MAVEN_OPTS="-Xmx768m -XX:ActiveProcessorCount=2" run_low_priority ./mvnw -DskipTests package$' "Pi build script packages the gateway jar at reduced priority"
assert_not_contains "$DASHBOARD_LAYOUT" 'next/font/google' "dashboard production build does not fetch Google fonts"

if (( failures > 0 )); then
  echo
  echo "$failures production service check(s) failed."
  exit 1
fi

echo
echo "Production service checks passed."
