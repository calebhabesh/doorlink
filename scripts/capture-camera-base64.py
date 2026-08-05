#!/usr/bin/env python3
"""Capture one base64-wrapped diagnostic JPEG without resetting the target."""

import argparse
import base64
import binascii
from pathlib import Path
import re
import sys
import time

import serial


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=60.0)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    deadline = time.monotonic() + args.timeout
    chunks: list[bytes] = []
    exporting = False
    expected_encoded_len: int | None = None

    target = serial.Serial(port=None, baudrate=args.baud, timeout=0.25)
    target.dtr = False
    target.rts = False
    target.port = args.port
    target.open()

    try:
        while time.monotonic() < deadline:
            line = target.readline()
            if not line:
                continue
            clean = line.strip()
            if b"JPEG_BASE64_BEGIN" in clean:
                exporting = True
                length_match = re.search(rb"encoded_len=(\d+)", clean)
                if length_match:
                    expected_encoded_len = int(length_match.group(1))
                print(clean.decode("utf-8", errors="replace"))
                continue
            if clean == b"JPEG_BASE64_END":
                break
            if exporting and clean.startswith(b"JPEG64:"):
                chunks.append(clean.removeprefix(b"JPEG64:"))
            elif not exporting:
                decoded = clean.decode("utf-8", errors="replace")
                if any(marker in decoded for marker in (
                    "Detected OV5640", "Sensor detected", "Post-init frame size",
                    "full-readout", "BLC recalibration", "OV5640 AEC timing",
                    "OV5640 timing regs",
                    "OV5640 readout regs", "OV5640 manual controls",
                    "OV5640 capture controls", "OV5640 automatic controls",
                    "Frame captured", "FB-OVF",
                    "rails disabled", "failed",
                )):
                    print(decoded)
        else:
            print("Timed out waiting for JPEG_BASE64_END", file=sys.stderr)
            return 2
    finally:
        target.close()

    encoded = b"".join(chunks)
    if expected_encoded_len is None:
        print("JPEG_BASE64_BEGIN did not report encoded_len", file=sys.stderr)
        return 3
    if len(encoded) != expected_encoded_len:
        print(
            f"Incomplete base64 stream: expected {expected_encoded_len} chars, "
            f"received {len(encoded)}",
            file=sys.stderr,
        )
        return 3

    try:
        jpeg = base64.b64decode(encoded, validate=True)
    except (binascii.Error, ValueError) as exc:
        print(f"Invalid base64 stream: {exc}", file=sys.stderr)
        return 3

    if not (jpeg.startswith(b"\xff\xd8") and jpeg.endswith(b"\xff\xd9")):
        print("Decoded data lacks JPEG SOI/EOI markers", file=sys.stderr)
        return 4

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(jpeg)
    print(f"JPEG saved: {args.output} ({len(jpeg)} bytes, {len(encoded)} base64 chars)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
