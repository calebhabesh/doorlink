#!/usr/bin/env python3
"""Verify the Rev C OV5640 physical mating map and camera power circuit.

The DCXYX-LZTKQJ-5M-357-V1 flex is inserted lens-up and contacts-down into
bottom-contact J3. The seller drawing is a contact-side bottom view, so camera
pin ``n`` physically mates with J3 pad ``25 - n``. This check deliberately
models that mirror instead of assuming same-numbered camera and connector pins.

Camera pins 23 and 24 remain isolated behind DNP contingency links. Rev C also
requires the GPIO42-controlled TPS22919 camera load switch to be populated
while the former R36 bypass and fixed R26 thermistor resistor remain DNP.
"""

from __future__ import annotations

import re
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCHEMATIC = ROOT / "pcb" / "smart-doorbell" / "smart-doorbell.kicad_sch"

EXPECTED_CAMERA_PIN_NETS = {
    1: "unconnected-(J3-CAM1_NC-Pad24)",
    2: "GND",  # AGND
    3: "/CAM_SDA",  # SIO_D
    4: "+2V8",  # AVDD
    5: "/CAM_SCL",  # SIO_C
    6: "/CAM_RST_2V8",  # RESET
    7: "/CAM_VSYNC",
    8: "/CAM_PWDN_2V8",
    9: "/CAM_HREF",
    10: "+1V5",  # DVDD
    11: "+2V8",  # DOVDD
    12: "/CAM_D7",  # Y9
    13: "/CAM_XCLK_2V8",
    14: "/CAM_D6",  # Y8
    15: "GND",  # DGND
    16: "/CAM_D5",  # Y7
    17: "/CAM_PCLK",
    18: "/CAM_D4",  # Y6
    19: "/CAM_D0",  # Y2
    20: "/CAM_D3",  # Y5
    21: "/CAM_D1",  # Y3
    22: "/CAM_D2",  # Y4
    23: "Net-(J3-CAM23_AF_GND)",
    24: "Net-(J3-CAM24_AFVDD)",
}

EXPECTED_CAMERA_CONTINGENCY_NETS = {
    ("R21", "1"): "Net-(J3-CAM23_AF_GND)",
    ("R21", "2"): "GND",
    ("TP2", "1"): "Net-(J3-CAM23_AF_GND)",
    ("R11", "1"): "Net-(J3-CAM24_AFVDD)",
    ("R11", "2"): "+2V8",
    ("TP1", "1"): "Net-(J3-CAM24_AFVDD)",
}

EXPECTED_CAMERA_POWER_NETS = {
    ("R37", "1"): "/GPIO42_AUX",
    ("R37", "2"): "/CAM_PWR_EN",
    ("R38", "1"): "GND",
    ("R38", "2"): "/CAM_PWR_EN",
    ("U9", "1"): "+3V3",
    ("U9", "2"): "GND",
    ("U9", "3"): "/CAM_PWR_EN",
    ("U9", "6"): "/CAM_3V3",
}

EXPECTED_DNP_COMPONENTS = {"R11", "R21", "R26", "R36"}
EXPECTED_POPULATED_COMPONENTS = {"C35", "R37", "R38", "U9"}
ASSEMBLY_STATE_COMPONENTS = EXPECTED_DNP_COMPONENTS | EXPECTED_POPULATED_COMPONENTS

EXPECTED_PULLUPS = {
    ("R13", "2"): "+2V8",  # CAM_SDA pullup
    ("R14", "2"): "+2V8",  # CAM_SCL pullup
}

FORBIDDEN_NETS = {
    ("J3", "14"): "+3V3",  # Camera DOVDD must be +2V8.
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
    "C35": "1u",
    "R37": "1k",
    "R38": "100k",
    "U9": "TPS22919DCK",
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


def parse_dnp_components() -> set[str]:
    """Return refs marked DNP in the KiCad schematic source.

    KiCad's sexpr netlist exporter omits the DNP flag when a component is also
    excluded from the BOM, so this one assembly-safety property is checked in
    the schematic itself.
    """
    dnp_refs: set[str] = set()
    source = SCHEMATIC.read_text(encoding="utf-8")
    for ref in ASSEMBLY_STATE_COMPONENTS:
        reference = f'\t\t(property "Reference" "{ref}"'
        reference_at = source.find(reference)
        symbol_at = source.rfind("\n\t(symbol\n", 0, reference_at)
        if reference_at >= 0 and symbol_at >= 0 and "\n\t\t(dnp yes)" in source[symbol_at:reference_at]:
            dnp_refs.add(ref)

    return dnp_refs


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
    dnp_components = parse_dnp_components()

    failures: list[str] = []

    for camera_pin, expected in EXPECTED_CAMERA_PIN_NETS.items():
        j3_pad = str(25 - camera_pin)
        failure = check_expected(node_nets, "J3", j3_pad, expected)
        if failure:
            failures.append(
                f"camera pin {camera_pin} -> physical J3 pad {j3_pad}: {failure}"
            )

    for (ref, pin), expected in EXPECTED_PULLUPS.items():
        failure = check_expected(node_nets, ref, pin, expected)
        if failure:
            failures.append(failure)

    for (ref, pin), expected in EXPECTED_CAMERA_CONTINGENCY_NETS.items():
        failure = check_expected(node_nets, ref, pin, expected)
        if failure:
            failures.append(failure)

    for (ref, pin), expected in EXPECTED_CAMERA_POWER_NETS.items():
        failure = check_expected(node_nets, ref, pin, expected)
        if failure:
            failures.append(failure)

    for ref in EXPECTED_DNP_COMPONENTS:
        if ref not in dnp_components:
            failures.append(f"{ref}: expected DNP marking")

    for ref in EXPECTED_POPULATED_COMPONENTS:
        if ref in dnp_components:
            failures.append(f"{ref}: expected populated, got DNP marking")

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

    print(
        "Camera interface verification passed: "
        "camera pin n -> physical J3 pad 25-n, Rev C power gate valid."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
