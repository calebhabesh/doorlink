#!/usr/bin/env python3
"""Verify the OV5640 camera connector nets in the KiCad schematic.

This check is intentionally narrow: it guards the Rev B camera connector
against the Rev A DOVDD/SCCB rail mistake and verifies the approved
DCXYX-LZTKQJ-5M-339-V1 24-pin OV5640 module pin table.
"""

from __future__ import annotations

import re
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCHEMATIC = ROOT / "pcb" / "smart-doorbell" / "smart-doorbell.kicad_sch"

EXPECTED_J3_NETS = {
    "1": "unconnected-(J3-Pin_1-Pad1)",  # STROBE unused
    "2": "GND",  # AGND
    "3": "/CAM_SDA",  # SIOD
    "4": "+2V8",  # AVDD
    "5": "/CAM_SCL",  # SIOC
    "6": "/CAM_RST_2V8",  # RESET
    "7": "/CAM_VSYNC",  # VSYNC
    "8": "/CAM_PWDN_2V8",  # PWDN
    "9": "/CAM_HREF",  # HREF
    "10": "+1V5",  # DVDD
    "11": "+2V8",  # DOVDD
    "12": "/CAM_D7",  # Y9
    "13": "/CAM_XCLK_2V8",  # XCLK1
    "14": "/CAM_D6",  # Y8
    "15": "GND",  # DGND
    "16": "/CAM_D5",  # Y7
    "17": "/CAM_PCLK",  # PCLK
    "18": "/CAM_D4",  # Y6
    "19": "/CAM_D0",  # Y2
    "20": "/CAM_D3",  # Y5
    "21": "/CAM_D1",  # Y3
    "22": "/CAM_D2",  # Y4
    "23": "GND",  # AF_GND
    "24": "+2V8",  # AFVDD
}

EXPECTED_PULLUPS = {
    ("R13", "2"): "+2V8",  # CAM_SDA pullup
    ("R14", "2"): "+2V8",  # CAM_SCL pullup
}

FORBIDDEN_NETS = {
    ("J3", "11"): "+3V3",
    ("R13", "2"): "+3V3",
    ("R14", "2"): "+3V3",
}

EXPECTED_DIVIDER_NETS = {
    ("R15", "1"): "/CAM_XCLK",
    ("R15", "2"): "/CAM_XCLK_2V8",
    ("R16", "1"): "/CAM_XCLK_2V8",
    ("R16", "2"): "GND",
    ("R17", "1"): "/CAM_RST",
    ("R17", "2"): "/CAM_RST_2V8",
    ("R18", "1"): "/CAM_RST_2V8",
    ("R18", "2"): "GND",
    ("R19", "1"): "/CAM_PWDN",
    ("R19", "2"): "/CAM_PWDN_2V8",
    ("R20", "1"): "/CAM_PWDN_2V8",
    ("R20", "2"): "GND",
}

EXPECTED_COMPONENT_VALUES = {
    "R15": "680",
    "R16": "3.9k",
    "R17": "10k",
    "R18": "56k",
    "R19": "10k",
    "R20": "56k",
}


def export_netlist() -> str:
    with tempfile.NamedTemporaryFile(suffix=".net", delete=False) as handle:
        output = Path(handle.name)

    try:
        subprocess.run(
            [
                "kicad-cli",
                "sch",
                "export",
                "netlist",
                "--format",
                "kicadsexpr",
                "-o",
                str(output),
                str(SCHEMATIC),
            ],
            check=True,
            cwd=ROOT,
        )
        return output.read_text(encoding="utf-8")
    finally:
        output.unlink(missing_ok=True)


def parse_node_nets(netlist: str) -> dict[tuple[str, str], str]:
    node_nets: dict[tuple[str, str], str] = {}
    current_net: str | None = None
    in_node = False
    node_ref: str | None = None
    node_pin: str | None = None

    for line in netlist.splitlines():
        name_match = re.search(r'^\s+\(name "([^"]+)"\)', line)
        if name_match and not in_node:
            current_net = name_match.group(1)
            continue

        if re.search(r"^\s+\(node$", line):
            in_node = True
            node_ref = None
            node_pin = None
            continue

        if in_node:
            ref_match = re.search(r'^\s+\(ref "([^"]+)"\)', line)
            if ref_match:
                node_ref = ref_match.group(1)
                continue

            pin_match = re.search(r'^\s+\(pin "([^"]+)"\)', line)
            if pin_match:
                node_pin = pin_match.group(1)
                continue

            if re.search(r"^\s+\)$", line):
                if current_net and node_ref and node_pin:
                    node_nets[(node_ref, node_pin)] = current_net
                in_node = False

    return node_nets


def parse_component_values(netlist: str) -> dict[str, str]:
    values: dict[str, str] = {}
    in_comp = False
    current_ref: str | None = None

    for line in netlist.splitlines():
        if re.search(r"^\s+\(comp$", line):
            in_comp = True
            current_ref = None
            continue

        if not in_comp:
            continue

        ref_match = re.search(r'^\s+\(ref "([^"]+)"\)', line)
        if ref_match:
            current_ref = ref_match.group(1)
            continue

        value_match = re.search(r'^\s+\(value "([^"]+)"\)', line)
        if value_match and current_ref:
            values[current_ref] = value_match.group(1)
            continue

        if re.search(r"^\s+\)$", line):
            in_comp = False

    return values


def check_expected(
    node_nets: dict[tuple[str, str], str], ref: str, pin: str, expected: str
) -> str | None:
    actual = node_nets.get((ref, pin))
    if actual != expected:
        return f"{ref} pin {pin}: expected {expected}, got {actual or 'MISSING'}"
    return None


def main() -> int:
    netlist = export_netlist()
    node_nets = parse_node_nets(netlist)
    component_values = parse_component_values(netlist)

    failures: list[str] = []

    for pin, expected in EXPECTED_J3_NETS.items():
        failure = check_expected(node_nets, "J3", pin, expected)
        if failure:
            failures.append(failure)

    for (ref, pin), expected in EXPECTED_PULLUPS.items():
        failure = check_expected(node_nets, ref, pin, expected)
        if failure:
            failures.append(failure)

    for (ref, pin), forbidden in FORBIDDEN_NETS.items():
        actual = node_nets.get((ref, pin))
        if actual == forbidden:
            failures.append(f"{ref} pin {pin}: forbidden net {forbidden}")

    for (ref, pin), expected in EXPECTED_DIVIDER_NETS.items():
        failure = check_expected(node_nets, ref, pin, expected)
        if failure:
            failures.append(failure)

    for ref, expected in EXPECTED_COMPONENT_VALUES.items():
        actual = component_values.get(ref)
        if actual != expected:
            failures.append(f"{ref}: expected value {expected}, got {actual or 'MISSING'}")

    if failures:
        print("Camera interface verification FAILED:")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    print("Camera interface verification passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
