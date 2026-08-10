#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_DIR="${DOORBELL_PI_OUTPUT_DIR:-$ROOT_DIR/dist/pi-production}"
PI_HOST="${DOORBELL_PI_HOST:-rpi}"
PI_ROOT="${DOORBELL_PI_ROOT:-/home/ethioprince/dev/smart-doorbell}"
PI_PLATFORM="${DOORBELL_PI_PLATFORM:-linux/arm64}"
BUILDER="${DOORBELL_PI_BUILDER:-smart-doorbell-pi-builder}"

build_gateway=true
build_dashboard=true
deploy=false
restart=false

usage() {
  cat <<'EOF'
Usage: ./scripts/build-pi-production.sh [options]

Build Raspberry Pi runtime artifacts on the Arch development machine.

Options:
  --gateway-only    Build only the Spring Boot gateway JAR
  --dashboard-only  Build only the ARM64 Next.js standalone server
  --deploy          Transfer built artifacts to the Pi (does not restart services)
  --restart         Transfer artifacts and restart only the services that were built
  -h, --help        Show this help

Environment overrides:
  DOORBELL_PI_HOST, DOORBELL_PI_ROOT, DOORBELL_PI_OUTPUT_DIR,
  DOORBELL_PI_PLATFORM, DOORBELL_PI_BUILDER
EOF
}

while (( $# > 0 )); do
  case "$1" in
    --gateway-only)
      build_gateway=true
      build_dashboard=false
      ;;
    --dashboard-only)
      build_gateway=false
      build_dashboard=true
      ;;
    --deploy)
      deploy=true
      ;;
    --restart)
      deploy=true
      restart=true
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

case "$PI_ROOT" in
  /*) ;;
  *)
    echo "ERROR: DOORBELL_PI_ROOT must be an absolute path." >&2
    exit 2
    ;;
esac

if [[ "$PI_ROOT" == *"'"* || "$OUTPUT_DIR" == "/" ]]; then
  echo "ERROR: unsafe deployment or output path." >&2
  exit 2
fi

mkdir -p "$OUTPUT_DIR"

if [[ "$build_gateway" == true ]]; then
  echo "[1/2] Packaging the architecture-neutral Spring Boot gateway on Arch..."
  (
    cd "$ROOT_DIR/gateway"
    ./mvnw -DskipTests package
  )
  mkdir -p "$OUTPUT_DIR/gateway"
  cp "$ROOT_DIR/gateway/target/gateway-0.0.1-SNAPSHOT.jar" "$OUTPUT_DIR/gateway/gateway.jar"
fi

if [[ "$build_dashboard" == true ]]; then
  for required_command in docker file rsync; do
    command -v "$required_command" >/dev/null 2>&1 || {
      echo "ERROR: $required_command is required for the Pi dashboard build." >&2
      exit 1
    }
  done
  docker info >/dev/null
  docker buildx version >/dev/null

  if ! docker buildx inspect "$BUILDER" >/dev/null 2>&1; then
    docker buildx create --name "$BUILDER" --driver docker-container --use >/dev/null
  else
    docker buildx use "$BUILDER"
  fi
  docker buildx inspect "$BUILDER" --bootstrap >/dev/null

  staging_dir="$(mktemp -d "${TMPDIR:-/tmp}/smart-doorbell-pi-build.XXXXXX")"
  cleanup() {
    rm -rf -- "$staging_dir"
  }
  trap cleanup EXIT

  echo "[2/2] Building the cached Pi-compatible Next.js standalone server..."
  docker buildx build \
    --builder "$BUILDER" \
    --platform "$PI_PLATFORM" \
    --file "$ROOT_DIR/dashboard/Dockerfile.pi" \
    --target artifact \
    --output "type=local,dest=$staging_dir/dashboard" \
    "$ROOT_DIR/dashboard"

  mkdir -p "$OUTPUT_DIR/dashboard"
  rsync --archive --delete "$staging_dir/dashboard/" "$OUTPUT_DIR/dashboard/"

  if find "$OUTPUT_DIR/dashboard" -type f -exec file {} + | grep -Eq 'ELF.*(x86-64|Intel 80386)'; then
    echo "ERROR: the standalone dashboard contains an x86 native runtime dependency." >&2
    echo "Build that dependency for ARM64 before deploying this artifact." >&2
    exit 1
  fi
fi

git_revision="$(git -C "$ROOT_DIR" rev-parse HEAD 2>/dev/null || printf unknown)"
if [[ -n "$(git -C "$ROOT_DIR" status --porcelain 2>/dev/null || true)" ]]; then
  git_revision="${git_revision}-dirty"
fi
printf 'revision=%s\nplatform=%s\nbuilt_at=%s\n' \
  "$git_revision" "$PI_PLATFORM" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" \
  > "$OUTPUT_DIR/manifest.txt"

echo
echo "Pi runtime artifacts are ready under $OUTPUT_DIR"

deploy_component() {
  local component="$1"
  local source_dir="$OUTPUT_DIR/$component"
  local component_root="$PI_ROOT/.pi-production/$component"
  local link_dest
  local release_id
  local release_dir

  [[ -d "$source_dir" ]] || {
    echo "ERROR: missing $component artifact directory: $source_dir" >&2
    exit 1
  }

  release_id="$(date -u +%Y%m%dT%H%M%S%NZ)-${git_revision:0:12}"
  release_id="${release_id//[^A-Za-z0-9._-]/-}"
  release_dir="$component_root/releases/$release_id"

  ssh "$PI_HOST" "mkdir -p '$release_dir'"
  link_dest="$component_root"
  if ssh "$PI_HOST" "test -e '$component_root/current'"; then
    link_dest="$component_root/current"
  fi
  rsync --archive --compress \
    --link-dest="$link_dest" \
    "$source_dir/" "$PI_HOST:$release_dir/"
  ssh "$PI_HOST" \
    "ln -sfn '$release_dir' '$component_root/current.next' && mv -Tf '$component_root/current.next' '$component_root/current'"
  echo "Deployed $component release $release_id to $PI_HOST."
}

if [[ "$deploy" == true ]]; then
  for required_command in ssh rsync; do
    command -v "$required_command" >/dev/null 2>&1 || {
      echo "ERROR: $required_command is required to deploy to the Pi." >&2
      exit 1
    }
  done
  echo
  echo "Deploying runtime artifacts to $PI_HOST:$PI_ROOT..."
  if [[ "$build_gateway" == true ]]; then
    deploy_component gateway
  fi
  if [[ "$build_dashboard" == true ]]; then
    deploy_component dashboard
  fi
fi

if [[ "$restart" == true ]]; then
  services=()
  if [[ "$build_gateway" == true ]]; then
    if ! ssh "$PI_HOST" \
      "systemctl cat smart-doorbell-gateway.service 2>/dev/null | grep -Fq '$PI_ROOT/.pi-production/gateway/current'"; then
      echo "ERROR: the installed gateway unit is not configured for Arch-built releases." >&2
      echo "Install scripts/systemd/smart-doorbell-gateway.service on the Pi and run systemctl daemon-reload." >&2
      exit 1
    fi
    services+=(smart-doorbell-gateway.service)
  fi
  if [[ "$build_dashboard" == true ]]; then
    if ! ssh "$PI_HOST" \
      "systemctl cat smart-doorbell-dashboard.service 2>/dev/null | grep -Fq '$PI_ROOT/.pi-production/dashboard/current'"; then
      echo "ERROR: the installed dashboard unit is not configured for Arch-built releases." >&2
      echo "Install scripts/systemd/smart-doorbell-dashboard.service on the Pi and run systemctl daemon-reload." >&2
      exit 1
    fi
    services+=(smart-doorbell-dashboard.service)
  fi
  ssh -t "$PI_HOST" sudo systemctl restart "${services[@]}"
  echo "Restarted ${services[*]} on $PI_HOST."
elif [[ "$deploy" == true ]]; then
  echo "Artifacts deployed. Restart the affected service when ready, or use --restart next time."
fi
