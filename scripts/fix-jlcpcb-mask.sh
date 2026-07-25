#!/usr/bin/env bash

set -euo pipefail

# Work around kicad-jlcpcb-tools omitting top-mask apertures for NPTH pads.
# The plugin supplies the JLCPCB_* paths when this script is configured as its
# post-generation hook. Direct invocation defaults to the separate Rev C
# release candidate so it cannot silently replace the immutable Rev B upload.

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "$script_dir/.." && pwd)"

board_path="${JLCPCB_BOARD_PATH:-$repo_root/pcb/smart-doorbell/smart-doorbell.kicad_pcb}"
release_dir="${JLCPCB_RELEASE_DIR:-$repo_root/pcb/smart-doorbell/jlcpcb/rev-c}"
gerber_dir="${JLCPCB_GERBER_DIR:-$release_dir/gerber}"
zip_path="${JLCPCB_ARTIFACT_GERBER_ZIP:-$release_dir/production_files/GERBER-smart-doorbell.zip}"

project_name="$(basename -- "$board_path" .kicad_pcb)"
mask_name="$project_name-MaskTop.gbr"
native_mask_name="$project_name-F_Mask.gbr"

for command in kicad-cli rg zip unzip sha256sum; do
    if ! command -v "$command" >/dev/null 2>&1; then
        echo "ERROR: required command is unavailable: $command" >&2
        exit 1
    fi
done

if [[ ! -f "$board_path" ]]; then
    echo "ERROR: PCB source does not exist: $board_path" >&2
    exit 1
fi

if [[ ! -d "$gerber_dir" ]]; then
    echo "ERROR: plugin Gerber directory does not exist: $gerber_dir" >&2
    exit 1
fi

mkdir -p -- "$(dirname -- "$zip_path")"

work_dir="$(mktemp -d "${TMPDIR:-/tmp}/jlcpcb-mask-fix.XXXXXX")"
zip_tmp="${zip_path%.zip}.tmp.zip"
mask_tmp="$gerber_dir/$mask_name.tmp"

cleanup() {
    rm -rf -- "$work_dir"
    rm -f -- "$zip_tmp" "$mask_tmp"
}
trap cleanup EXIT

native_dir="$work_dir/native"
mkdir -p -- "$native_dir"

echo "Generating source-correct native F.Mask Gerber..."
kicad-cli pcb export gerbers \
    --output "$native_dir" \
    --layers F.Mask \
    --use-drill-file-origin \
    --no-protel-ext \
    --precision 6 \
    "$board_path"

native_mask="$native_dir/$native_mask_name"
if [[ ! -s "$native_mask" ]]; then
    echo "ERROR: native F.Mask export was not created: $native_mask" >&2
    exit 1
fi

verify_mask() {
    local mask_file="$1"
    local aperture
    local coordinate
    local current_aperture=""
    local line

    # Current project-specific aperture definitions.
    if ! rg -Fq '%ADD19C,0.500000*%' "$mask_file"; then
        echo "ERROR: missing 0.5 mm D19 aperture in $mask_file" >&2
        return 1
    fi
    if ! rg -Fq '%ADD30C,2.700000*%' "$mask_file"; then
        echo "ERROR: missing 2.7 mm D30 aperture in $mask_file" >&2
        return 1
    fi

    # MK1 0.5 mm acoustic opening and H1-H4 2.7 mm openings.
    while IFS=" " read -r aperture coordinate; do
        current_aperture=""
        while IFS= read -r line; do
            if [[ "$line" =~ ^D([0-9]+)\*$ ]]; then
                current_aperture="D${BASH_REMATCH[1]}"
            fi
            if [[ "$line" == "$coordinate" ]]; then
                if [[ "$current_aperture" != "$aperture" ]]; then
                    echo "ERROR: $coordinate uses ${current_aperture:-no aperture}, expected $aperture in $mask_file" >&2
                    return 1
                fi
                break
            fi
        done <"$mask_file"

        if [[ "$line" != "$coordinate" ]]; then
            echo "ERROR: missing required flash $coordinate in $mask_file" >&2
            return 1
        fi
    done <<'EOF'
D19 X110950000Y-90152000D03*
D30 X111050000Y-71425000D03*
D30 X162450000Y-71425000D03*
D30 X111050000Y-128825000D03*
D30 X162450000Y-128825000D03*
EOF

    return 0
}

if ! verify_mask "$native_mask"; then
    echo "ERROR: native F.Mask lacks a required MK1 or H1-H4 aperture." >&2
    exit 1
fi

install -m 0644 -- "$native_mask" "$mask_tmp"
mv -f -- "$mask_tmp" "$gerber_dir/$mask_name"

mapfile -d '' package_files < <(
    find "$gerber_dir" -maxdepth 1 -type f \
        \( -name '*.gbr' -o -name '*.drl' -o -name '*.pdf' \) \
        -print0 | sort -z
)

if [[ "${#package_files[@]}" -ne 13 ]]; then
    echo "ERROR: expected 13 Gerber/drill package files, found ${#package_files[@]}." >&2
    exit 1
fi

rm -f -- "$zip_tmp"
zip -j -X -9 -q "$zip_tmp" "${package_files[@]}"
unzip -tq "$zip_tmp" >/dev/null

embedded_mask="$work_dir/$mask_name"
unzip -p "$zip_tmp" "$mask_name" >"$embedded_mask"
if ! verify_mask "$embedded_mask"; then
    echo "ERROR: rebuilt ZIP does not contain the corrected top mask." >&2
    exit 1
fi

mv -f -- "$zip_tmp" "$zip_path"

echo "Corrected top mask and rebuilt 13-file fabrication ZIP."
echo "ZIP: $zip_path"
sha256sum "$zip_path"
