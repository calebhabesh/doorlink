#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
test_binary="$(mktemp /tmp/smart-doorbell-intercom-test.XXXXXX)"
trap 'rm -f "$test_binary"' EXIT

${CXX:-c++} -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  -I"$repo_dir/main" \
  "$repo_dir/main/services/intercom_protocol.cpp" \
  "$repo_dir/tests/firmware/intercom_protocol_test.cpp" \
  -o "$test_binary"

"$test_binary"
