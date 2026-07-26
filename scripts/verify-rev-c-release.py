#!/usr/bin/env python3
"""Fail-closed checks for the ordered Smart Doorbell Rev C JLCPCB release."""

from __future__ import annotations

import csv
import hashlib
import re
import struct
import subprocess
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path
from xml.etree import ElementTree


ROOT = Path(__file__).resolve().parents[1]
PCB_DIR = ROOT / "pcb" / "smart-doorbell"
JLCPCB_DIR = PCB_DIR / "jlcpcb"
RELEASE_DIR = JLCPCB_DIR / "rev-c"
GERBER_DIR = RELEASE_DIR / "gerber"
PRODUCTION_DIR = RELEASE_DIR / "production_files"
PROTECTED_BOM = (
    PCB_DIR / "production" / "Smart_Doorbell_Project_B_bom-JLCPCB_FINAL.csv"
)
REV_C_BOM = PRODUCTION_DIR / "BOM-smart-doorbell.csv"
REV_C_CPL = PRODUCTION_DIR / "CPL-smart-doorbell.csv"
REV_C_ZIP = PRODUCTION_DIR / "GERBER-smart-doorbell.zip"
REV_C_HASHES = RELEASE_DIR / "ORDERED_RELEASE.sha256"
ORDER_EVIDENCE_HASHES = RELEASE_DIR / "ORDER_EVIDENCE.sha256"
ORDER_RECORD = RELEASE_DIR / "ORDERED_RELEASE.md"
REV_B_HASHES = JLCPCB_DIR / "ORDERED_RELEASE.sha256"
SCHEMATIC = PCB_DIR / "smart-doorbell.kicad_sch"
BOARD = PCB_DIR / "smart-doorbell.kicad_pcb"
CAMERA_SYMBOL = PCB_DIR / "ov5640_pinout.kicad_sym"

EXPECTED_REV_C_REFS = 85
EXPECTED_BOARD_FOOTPRINTS = 108
EXPECTED_ADDED_REFS = {"C35", "R37", "U9"}
EXPECTED_REMOVED_REFS = {"R26", "R36"}
EXPECTED_NEW_PARTS = {
    "C35": ("1u", "C15849"),
    "R37": ("1k", "C21190"),
    "U9": ("TPS22919DCK", "C2149796"),
}
EXPECTED_UPLOAD_FILES = {
    "production_files/GERBER-smart-doorbell.zip",
    "production_files/BOM-smart-doorbell.csv",
    "production_files/CPL-smart-doorbell.csv",
}
EXPECTED_EVIDENCE_FILES = {
    "evidence/JLCPCB-assembly-preview-top.png",
    "evidence/JLCPCB-BOM-matching-2026-07-26.xlsx",
}
EXPECTED_ORDER_IDENTIFIERS = {
    "2026-07-26 06:01:45",
    "Y9-6841583A",
    "SMT026072660566-6841583A",
    "W2026072618014602",
    "pcb-rev-c-ordered-2026-07-26",
}
PACKAGE_SUFFIXES = {".gbr", ".drl", ".pdf"}
EXPECTED_PACKAGE_MEMBERS = 13
U9_FLASHES = {
    "X125912500Y-112350000D03*",
    "X125912500Y-113000000D03*",
    "X125912500Y-113650000D03*",
    "X127587500Y-113650000D03*",
    "X127587500Y-113000000D03*",
    "X127587500Y-112350000D03*",
}
MASK_FLASH_APERTURES = {
    "X110950000Y-90152000D03*": "D19",
    "X111050000Y-71425000D03*": "D30",
    "X162450000Y-71425000D03*": "D30",
    "X111050000Y-128825000D03*": "D30",
    "X162450000Y-128825000D03*": "D30",
}


@dataclass(frozen=True)
class Part:
    value: str
    lcsc: str


class Checks:
    def __init__(self) -> None:
        self.failures: list[str] = []

    def require(self, condition: bool, message: str) -> None:
        if not condition:
            self.failures.append(message)


def split_refs(raw: str) -> list[str]:
    return [ref.strip() for ref in raw.split(",") if ref.strip()]


def read_bom(
    path: Path, *, value_column: str, lcsc_column: str, checks: Checks
) -> dict[str, Part]:
    parts: dict[str, Part] = {}
    with path.open(newline="", encoding="utf-8-sig") as handle:
        for line_number, row in enumerate(csv.DictReader(handle), start=2):
            refs = split_refs(row["Designator"])
            try:
                quantity = int(row["Quantity"])
            except (TypeError, ValueError):
                checks.failures.append(f"{path}: invalid quantity on line {line_number}")
                continue
            checks.require(
                quantity == len(refs),
                f"{path}: line {line_number} quantity {quantity} != {len(refs)} refs",
            )
            part = Part(row[value_column].strip(), row[lcsc_column].strip())
            for ref in refs:
                if ref in parts:
                    checks.failures.append(f"{path}: duplicate reference {ref}")
                parts[ref] = part
    return parts


def read_cpl(path: Path, checks: Checks) -> dict[str, dict[str, str]]:
    placements: dict[str, dict[str, str]] = {}
    with path.open(newline="", encoding="utf-8-sig") as handle:
        for line_number, row in enumerate(csv.DictReader(handle), start=2):
            ref = row["Designator"].strip()
            if not ref:
                checks.failures.append(f"{path}: blank designator on line {line_number}")
            elif ref in placements:
                checks.failures.append(f"{path}: duplicate placement {ref}")
            else:
                placements[ref] = row
    return placements


def parse_hash_manifest(path: Path, checks: Checks) -> dict[str, str]:
    hashes: dict[str, str] = {}
    for line_number, raw_line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), start=1
    ):
        if not raw_line.strip():
            continue
        match = re.fullmatch(r"([0-9a-f]{64})  (.+)", raw_line)
        if not match:
            checks.failures.append(f"{path}: malformed line {line_number}")
            continue
        digest, relative_path = match.groups()
        hashes[relative_path] = digest
    return hashes


def check_hashes(base: Path, manifest: Path, checks: Checks) -> set[str]:
    if not manifest.is_file():
        checks.failures.append(f"missing hash manifest: {manifest}")
        return set()

    recorded = parse_hash_manifest(manifest, checks)
    for relative_path, expected in recorded.items():
        artifact = base / relative_path
        if not artifact.is_file():
            checks.failures.append(f"{manifest}: missing artifact {relative_path}")
            continue
        actual = hashlib.sha256(artifact.read_bytes()).hexdigest()
        checks.require(
            actual == expected,
            f"{manifest}: {relative_path} hash {actual} != {expected}",
        )
    return set(recorded)


def check_git_blob_hashes(base: Path, manifest: Path, checks: Checks) -> None:
    """Ensure a clean checkout will contain the exact hashed upload bytes."""
    recorded = parse_hash_manifest(manifest, checks)
    for relative_path, expected in recorded.items():
        repo_path = (base / relative_path).relative_to(ROOT)
        staged = subprocess.run(
            ["git", "show", f":{repo_path}"],
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        if staged.returncode != 0:
            checks.failures.append(f"{repo_path}: upload artifact is not tracked by Git")
            continue
        actual = hashlib.sha256(staged.stdout).hexdigest()
        checks.require(
            actual == expected,
            f"{repo_path}: Git blob hash {actual} != release hash {expected}",
        )


def title_block_has_rev_c(path: Path) -> bool:
    source = path.read_text(encoding="utf-8")
    start = source.find("(title_block")
    if start < 0:
        return False
    end = source.find("\n\t)", start)
    if end < 0:
        return False
    return '(rev "C")' in source[start:end]


def gerber_apertures(path: Path) -> dict[str, str | None]:
    aperture: str | None = None
    flashes: dict[str, str | None] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        select = re.fullmatch(r"D([0-9]+)\*", line)
        if select:
            aperture = f"D{select.group(1)}"
        if line.endswith("D03*"):
            flashes[line] = aperture
    return flashes


def spreadsheet_column(cell_reference: str) -> int:
    match = re.match(r"[A-Z]+", cell_reference)
    if match is None:
        raise ValueError(f"invalid spreadsheet cell reference: {cell_reference}")
    result = 0
    for character in match.group():
        result = result * 26 + ord(character) - ord("A") + 1
    return result


def check_order_evidence(checks: Checks) -> None:
    preview = RELEASE_DIR / "evidence" / "JLCPCB-assembly-preview-top.png"
    try:
        preview_bytes = preview.read_bytes()
    except OSError as exc:
        checks.failures.append(f"cannot read JLCPCB assembly preview: {exc}")
        preview_bytes = b""
    checks.require(
        preview_bytes.startswith(b"\x89PNG\r\n\x1a\n"),
        "JLCPCB assembly preview is not a PNG",
    )
    if len(preview_bytes) >= 24:
        width, height = struct.unpack(">II", preview_bytes[16:24])
        checks.require(
            (width, height) == (780, 1171),
            f"JLCPCB assembly preview is {width}x{height}, expected 780x1171",
        )

    expected: dict[str, tuple[str, str, str]] = {}
    with REV_C_BOM.open(newline="", encoding="utf-8-sig") as handle:
        for row in csv.DictReader(handle):
            details = (
                row["Comment"].strip(),
                row["Footprint"].strip(),
                row["LCSC"].strip(),
            )
            for ref in split_refs(row["Designator"]):
                expected[ref] = details

    evidence = RELEASE_DIR / "evidence" / "JLCPCB-BOM-matching-2026-07-26.xlsx"
    namespace = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    matched: dict[str, tuple[str, str, str]] = {}
    statuses: list[tuple[set[str], str, str]] = []
    try:
        with zipfile.ZipFile(evidence) as archive:
            shared_root = ElementTree.fromstring(archive.read("xl/sharedStrings.xml"))
            shared_strings = [
                "".join(
                    text.text or ""
                    for text in item.findall(".//m:t", namespace)
                )
                for item in shared_root.findall("m:si", namespace)
            ]
            sheet_root = ElementTree.fromstring(
                archive.read("xl/worksheets/sheet1.xml")
            )
            for row in sheet_root.findall(".//m:sheetData/m:row", namespace):
                if int(row.attrib["r"]) < 6:
                    continue
                values: dict[int, str] = {}
                for cell in row.findall("m:c", namespace):
                    value_node = cell.find("m:v", namespace)
                    value = "" if value_node is None else value_node.text or ""
                    if cell.attrib.get("t") == "s" and value:
                        value = shared_strings[int(value)]
                    values[spreadsheet_column(cell.attrib["r"])] = value

                refs = set(split_refs(values.get(1, "")))
                checks.require(
                    not values.get(2, ""),
                    f"JLCPCB evidence has bottom designators: {values.get(2)}",
                )
                details = (
                    values.get(3, ""),
                    values.get(4, ""),
                    values.get(13, ""),
                )
                for ref in refs:
                    checks.require(
                        ref not in matched,
                        f"JLCPCB BOM evidence duplicates {ref}",
                    )
                    matched[ref] = details
                statuses.append((refs, values.get(6, ""), values.get(12, "")))
    except (
        ElementTree.ParseError,
        OSError,
        KeyError,
        ValueError,
        zipfile.BadZipFile,
    ) as exc:
        checks.failures.append(f"cannot read JLCPCB BOM evidence: {exc}")
        return

    checks.require(
        len(statuses) == 38,
        f"JLCPCB BOM evidence has {len(statuses)} rows, expected 38",
    )
    checks.require(
        set(matched) == set(expected),
        "JLCPCB BOM evidence reference set differs from ordered BOM",
    )
    for ref in sorted(set(matched) & set(expected)):
        checks.require(
            matched[ref] == expected[ref],
            f"{ref}: JLCPCB BOM evidence {matched[ref]} != ordered {expected[ref]}",
        )

    unconfirmed = [
        (refs, warning)
        for refs, status, warning in statuses
        if status == "Unconfirmed"
    ]
    checks.require(
        unconfirmed
        == [
            (
                {"BOOT1", "RESET1"},
                "1.There may be multiple types of parts, but one type of part has been matched. Please check.;",
            )
        ],
        f"unexpected JLCPCB unconfirmed rows: {unconfirmed}",
    )
    checks.require(
        all(status in {"Select by System", "Unconfirmed"} for _, status, _ in statuses),
        "JLCPCB BOM evidence contains an unexpected match status",
    )
    unexpected_warnings = [
        (refs, warning)
        for refs, status, warning in statuses
        if status == "Select by System" and warning
    ]
    checks.require(
        not unexpected_warnings,
        f"unexpected warnings on matched JLCPCB rows: {unexpected_warnings}",
    )


def check_release_contents(checks: Checks) -> None:
    protected = read_bom(
        PROTECTED_BOM,
        value_column="Value",
        lcsc_column="LCSC Part #",
        checks=checks,
    )
    rev_c = read_bom(
        REV_C_BOM,
        value_column="Comment",
        lcsc_column="LCSC",
        checks=checks,
    )
    placements = read_cpl(REV_C_CPL, checks)

    checks.require(
        len(rev_c) == EXPECTED_REV_C_REFS,
        f"Rev C BOM has {len(rev_c)} refs, expected {EXPECTED_REV_C_REFS}",
    )
    checks.require(
        len(placements) == EXPECTED_REV_C_REFS,
        f"Rev C CPL has {len(placements)} refs, expected {EXPECTED_REV_C_REFS}",
    )
    checks.require(
        set(rev_c) == set(placements),
        "Rev C BOM and CPL reference sets differ",
    )
    checks.require(
        set(rev_c) - set(protected) == EXPECTED_ADDED_REFS,
        "Rev C additions differ from C35/R37/U9",
    )
    checks.require(
        set(protected) - set(rev_c) == EXPECTED_REMOVED_REFS,
        "Rev C removals differ from R26/R36",
    )

    for ref in sorted(set(rev_c) & set(protected)):
        checks.require(
            rev_c[ref] == protected[ref],
            f"{ref}: Rev C value/LCSC {rev_c[ref]} != protected {protected[ref]}",
        )
    for ref, expected in EXPECTED_NEW_PARTS.items():
        actual = rev_c.get(ref)
        checks.require(
            actual == Part(*expected),
            f"{ref}: expected value/LCSC {Part(*expected)}, got {actual}",
        )

    u9 = placements.get("U9", {})
    try:
        u9_rotation = float(u9.get("Rotation", "nan")) % 360
    except ValueError:
        u9_rotation = float("nan")
    checks.require(u9_rotation == 180.0, f"U9 CPL rotation is {u9_rotation}, expected 180")
    checks.require(u9.get("Layer") == "top", "U9 must be a top-side placement")

    package_files = sorted(
        path for path in GERBER_DIR.iterdir() if path.suffix in PACKAGE_SUFFIXES
    )
    checks.require(
        len(package_files) == EXPECTED_PACKAGE_MEMBERS,
        f"Gerber directory has {len(package_files)} package files, expected 13",
    )

    try:
        with zipfile.ZipFile(REV_C_ZIP) as archive:
            bad_member = archive.testzip()
            checks.require(bad_member is None, f"Gerber ZIP CRC failure: {bad_member}")
            archive_names = sorted(
                info.filename for info in archive.infolist() if not info.is_dir()
            )
            expected_names = sorted(path.name for path in package_files)
            checks.require(
                archive_names == expected_names,
                "Gerber ZIP members differ from the generated Gerber directory",
            )
            for path in package_files:
                if path.name in archive_names:
                    checks.require(
                        archive.read(path.name) == path.read_bytes(),
                        f"Gerber ZIP member differs from directory: {path.name}",
                    )
    except (OSError, zipfile.BadZipFile) as exc:
        checks.failures.append(f"cannot read Gerber ZIP: {exc}")

    for gerber in GERBER_DIR.glob("*.gbr"):
        checks.require(
            re.search(
                r"%TF\.ProjectId,smart-doorbell,[^,]+,C\*%",
                gerber.read_text(encoding="utf-8"),
            )
            is not None,
            f"{gerber.name}: Gerber project metadata is not Rev C",
        )

    mask = GERBER_DIR / "smart-doorbell-MaskTop.gbr"
    mask_source = mask.read_text(encoding="utf-8")
    checks.require(
        "%ADD19C,0.500000*%" in mask_source,
        "top mask lacks the 0.5 mm MK1 aperture definition",
    )
    checks.require(
        "%ADD30C,2.700000*%" in mask_source,
        "top mask lacks the 2.7 mm mounting-hole aperture definition",
    )
    mask_flashes = gerber_apertures(mask)
    for flash, expected_aperture in MASK_FLASH_APERTURES.items():
        checks.require(
            mask_flashes.get(flash) == expected_aperture,
            f"top mask {flash} uses {mask_flashes.get(flash)}, expected {expected_aperture}",
        )

    for filename in ("smart-doorbell-CuTop.gbr", "smart-doorbell-PasteTop.gbr"):
        source = (GERBER_DIR / filename).read_text(encoding="utf-8")
        present = {flash for flash in U9_FLASHES if source.count(flash) == 1}
        checks.require(
            present == U9_FLASHES,
            f"{filename}: U9 does not have exactly six expected apertures",
        )


def main() -> int:
    checks = Checks()

    check_release_contents(checks)
    check_order_evidence(checks)

    rev_c_paths = check_hashes(RELEASE_DIR, REV_C_HASHES, checks)
    check_git_blob_hashes(RELEASE_DIR, REV_C_HASHES, checks)
    checks.require(
        rev_c_paths == EXPECTED_UPLOAD_FILES,
        f"Rev C hash manifest paths are {sorted(rev_c_paths)}, expected exactly three uploads",
    )
    evidence_paths = check_hashes(RELEASE_DIR, ORDER_EVIDENCE_HASHES, checks)
    check_git_blob_hashes(RELEASE_DIR, ORDER_EVIDENCE_HASHES, checks)
    checks.require(
        evidence_paths == EXPECTED_EVIDENCE_FILES,
        f"order evidence paths are {sorted(evidence_paths)}, expected {sorted(EXPECTED_EVIDENCE_FILES)}",
    )
    rev_b_paths = check_hashes(JLCPCB_DIR, REV_B_HASHES, checks)
    check_git_blob_hashes(JLCPCB_DIR, REV_B_HASHES, checks)
    checks.require(
        rev_b_paths == EXPECTED_UPLOAD_FILES,
        "protected Rev B hash manifest no longer names its exact three uploads",
    )

    checks.require(title_block_has_rev_c(SCHEMATIC), "schematic title block is not Rev C")
    checks.require(title_block_has_rev_c(BOARD), "PCB title block is not Rev C")
    board_source = BOARD.read_text(encoding="utf-8")
    board_lines = board_source.splitlines()
    checks.require(
        'gr_text "CAM1 / J3-24"' in board_source,
        "PCB lacks the CAM1 / J3-24 silkscreen label",
    )
    checks.require(
        'gr_text "CAM24 / J3-1"' in board_source,
        "PCB lacks the CAM24 / J3-1 silkscreen label",
    )
    footprint_starts = [
        index
        for index, line in enumerate(board_lines)
        if line.startswith('\t(footprint "')
    ]
    locked_footprints = sum(
        "\t\t(locked yes)" in board_lines[index + 1 : index + 5]
        for index in footprint_starts
    )
    checks.require(
        len(footprint_starts) == EXPECTED_BOARD_FOOTPRINTS,
        f"PCB has {len(footprint_starts)} footprints, expected {EXPECTED_BOARD_FOOTPRINTS}",
    )
    checks.require(
        locked_footprints == len(footprint_starts),
        f"PCB has {locked_footprints}/{len(footprint_starts)} locked footprints",
    )

    checks.require(CAMERA_SYMBOL.is_file(), "project-local OV5640 symbol is missing")
    sym_table = (PCB_DIR / "sym-lib-table").read_text(encoding="utf-8")
    checks.require(
        "${KIPRJMOD}/ov5640_pinout.kicad_sym" in sym_table,
        "sym-lib-table does not reference the project-local OV5640 symbol",
    )
    tracked = subprocess.run(
        ["git", "ls-files", "--error-unmatch", str(CAMERA_SYMBOL.relative_to(ROOT))],
        cwd=ROOT,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    checks.require(
        tracked.returncode == 0,
        "ov5640_pinout.kicad_sym exists but is not tracked by Git",
    )
    try:
        order_record = ORDER_RECORD.read_text(encoding="utf-8")
    except OSError as exc:
        checks.failures.append(f"cannot read Rev C ordered-release record: {exc}")
        order_record = ""
    for identifier in EXPECTED_ORDER_IDENTIFIERS:
        checks.require(
            identifier in order_record,
            f"Rev C ordered release is missing order identifier {identifier}",
        )
    checks.require(
        "Status: **signed off before order**." in order_record,
        "Rev C ordered release does not record signed-off JLCPCB preview status",
    )

    if checks.failures:
        print("Rev C release verification FAILED:")
        for failure in checks.failures:
            print(f"  - {failure}")
        return 1

    print("Ordered Rev C release verification passed:")
    print("  - immutable Rev B upload hashes pass")
    print("  - exact committed Rev C upload bytes match their ordered hashes")
    print("  - JLCPCB BOM and top-preview evidence match their ordered hashes")
    print("  - Rev C BOM/CPL contain the same 85 references")
    print("  - protected assignments and intended population delta match")
    print("  - U9 is top-side at 180 degrees with six copper/paste apertures")
    print("  - 13-file Rev C Gerber ZIP exactly matches its directory")
    print("  - all 108 footprints are locked")
    print("  - Rev C metadata, mask openings, silkscreen labels, and symbol pass")
    return 0


if __name__ == "__main__":
    sys.exit(main())
