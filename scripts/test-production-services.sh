#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GATEWAY_SERVICE="$ROOT_DIR/scripts/systemd/smart-doorbell-gateway.service"
DASHBOARD_SERVICE="$ROOT_DIR/scripts/systemd/smart-doorbell-dashboard.service"
INFRA_SERVICE="$ROOT_DIR/scripts/systemd/smart-doorbell-infra.service"
PI_BUILD_SCRIPT="$ROOT_DIR/scripts/build-pi-production.sh"
DASHBOARD_LAYOUT="$ROOT_DIR/dashboard/src/app/layout.tsx"
DASHBOARD_CONFIG="$ROOT_DIR/dashboard/next.config.mjs"
DASHBOARD_PI_DOCKERFILE="$ROOT_DIR/dashboard/Dockerfile.pi"

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

assert_contains "$INFRA_SERVICE" '^Type=oneshot$' "infra service is a oneshot compose bootstrap"
assert_contains "$INFRA_SERVICE" '^WorkingDirectory=/home/ethioprince/dev/smart-doorbell$' "infra service runs from the Pi checkout"
assert_contains "$INFRA_SERVICE" '^ExecStart=/usr/bin/docker compose up -d$' "infra service starts compose containers"
assert_contains "$INFRA_SERVICE" '^RemainAfterExit=yes$' "infra service stays active after compose bootstrap"
assert_contains "$INFRA_SERVICE" '^Requires=docker.service$' "infra service requires Docker"
assert_contains "$INFRA_SERVICE" '^After=.*docker\.service' "infra service starts after Docker"
assert_contains "$INFRA_SERVICE" '^WantedBy=multi-user\.target$' "infra service is enabled for normal headless boot"

assert_contains "$GATEWAY_SERVICE" '^WorkingDirectory=/home/ethioprince/dev/smart-doorbell/\.pi-production/gateway/current$' "gateway service uses the deployed release directory"
assert_contains "$GATEWAY_SERVICE" '^ExecStart=/usr/bin/java .*-jar /home/ethioprince/dev/smart-doorbell/\.pi-production/gateway/current/gateway\.jar$' "gateway service runs the Arch-built Spring Boot jar"
assert_contains "$GATEWAY_SERVICE" '^Requires=smart-doorbell-infra\.service$' "gateway service requires the compose infra bootstrap"
assert_contains "$GATEWAY_SERVICE" '^After=.*smart-doorbell-infra\.service' "gateway service starts after the compose infra bootstrap"
assert_not_contains "$GATEWAY_SERVICE" '^Requires=docker\.service$' "gateway service does not depend directly on raw Docker"
assert_contains "$GATEWAY_SERVICE" '^EnvironmentFile=-/home/ethioprince/dev/smart-doorbell/\.env$' "gateway service treats the shared environment file as optional"
assert_not_contains "$GATEWAY_SERVICE" 'spring-boot:run|/mvnw|maven' "gateway service does not run Maven or spring-boot:run"
assert_not_contains "$GATEWAY_SERVICE" '^ExecStartPre=' "gateway service avoids brittle external start-pre checks"
assert_contains "$GATEWAY_SERVICE" '^Nice=' "gateway service lowers scheduler priority"
assert_not_contains "$GATEWAY_SERVICE" '^(CPUQuota|CPUWeight|MemoryHigh|MemoryMax|ProtectSystem|ProtectHome|ReadWritePaths|PrivateTmp|NoNewPrivileges)=' "gateway service avoids Pi-incompatible resource/sandbox directives"

assert_contains "$DASHBOARD_SERVICE" '^WorkingDirectory=/home/ethioprince/dev/smart-doorbell/\.pi-production/dashboard/current$' "dashboard service uses the deployed release directory"
assert_contains "$DASHBOARD_SERVICE" '^ExecStart=/usr/bin/node server\.js$' "dashboard service runs the ARM64 standalone Next.js server"
assert_contains "$DASHBOARD_SERVICE" '^EnvironmentFile=-/home/ethioprince/dev/smart-doorbell/\.env$' "dashboard service treats the shared environment file as optional"
assert_contains "$DASHBOARD_SERVICE" '^Environment=NODE_ENV=production$' "dashboard service uses production NODE_ENV"
assert_contains "$DASHBOARD_SERVICE" '^Environment=NEXT_TELEMETRY_DISABLED=1$' "dashboard service disables Next.js telemetry"
assert_not_contains "$DASHBOARD_SERVICE" 'npm run dev|next dev|NODE_ENV=development' "dashboard service does not run the Next.js dev server"
assert_not_contains "$DASHBOARD_SERVICE" '^ExecStartPre=' "dashboard service avoids brittle external start-pre checks"
assert_contains "$DASHBOARD_SERVICE" '^Nice=' "dashboard service lowers scheduler priority"
assert_not_contains "$DASHBOARD_SERVICE" '^(CPUQuota|CPUWeight|MemoryHigh|MemoryMax|ProtectSystem|ProtectHome|ReadWritePaths|PrivateTmp|NoNewPrivileges)=' "dashboard service avoids Pi-incompatible resource/sandbox directives"

assert_contains "$PI_BUILD_SCRIPT" '^\s*./mvnw -DskipTests package$' "production build packages the portable gateway jar on Arch"
assert_contains "$PI_BUILD_SCRIPT" '^\s*docker buildx build \\' "production build uses Docker buildx for the dashboard"
assert_contains "$PI_BUILD_SCRIPT" '^\s*--platform "\$PI_PLATFORM" \\' "production build explicitly targets the Pi platform"
assert_contains "$DASHBOARD_PI_DOCKERFILE" '^FROM --platform=\$BUILDPLATFORM ' "dashboard compilation runs natively on the Arch builder"
assert_contains "$DASHBOARD_CONFIG" "^\s*output: 'standalone',$" "dashboard emits a traced standalone runtime"
assert_contains "$PI_BUILD_SCRIPT" "ELF\.\*\(x86-64\|Intel 80386\)" "production build rejects x86 runtime binaries"
assert_contains "$PI_BUILD_SCRIPT" '^\s*rsync --archive --compress ' "production deploy transfers runtime artifacts with rsync"
assert_contains "$PI_BUILD_SCRIPT" '^\s*--link-dest="\$link_dest" ' "production deploy reuses unchanged release files"
assert_contains "$PI_BUILD_SCRIPT" 'systemctl cat smart-doorbell-dashboard\.service ' "production restart verifies the installed Pi units"
assert_contains "$PI_BUILD_SCRIPT" '^\s*ssh -t "\$PI_HOST" sudo systemctl restart ' "production restart is explicit and remote"
assert_not_contains "$DASHBOARD_LAYOUT" 'next/font/google' "dashboard production build does not fetch Google fonts"

if (( failures > 0 )); then
  echo
  echo "$failures production service check(s) failed."
  exit 1
fi

echo
echo "Production service checks passed."
