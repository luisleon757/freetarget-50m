#!/usr/bin/env python3

import argparse
import csv
import json
import socket
import struct
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path

MAGIC = b"FT50"
VERSION = 1
HEADER = struct.Struct("<4sBBHIIIIHH")
ADC_COUNTS_FULL_SCALE = 4095.0
ADC_VREF = 3.3


@dataclass
class Capture:
    capture_id: int
    sample_rate_hz: int
    total_samples: int
    trigger_index: int
    source: str
    samples: list = field(default_factory=list)
    received: list = field(default_factory=list)

    @classmethod
    def create(cls, capture_id, sample_rate_hz, total_samples, trigger_index, source):
        return cls(
            capture_id=capture_id,
            sample_rate_hz=sample_rate_hz,
            total_samples=total_samples,
            trigger_index=trigger_index,
            source=source,
            samples=[0] * total_samples,
            received=[False] * total_samples,
        )

    def add(self, offset, values):
        end = offset + len(values)
        if offset < 0 or end > self.total_samples:
            raise ValueError("packet sample range outside capture")
        self.samples[offset:end] = values
        self.received[offset:end] = [True] * len(values)

    @property
    def complete(self):
        return all(self.received)


def save_capture(capture: Capture, output_dir: Path):
    output_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    stem = f"ft50_capture_{capture.capture_id:06d}_{timestamp}"

    csv_path = output_dir / f"{stem}.csv"
    metadata_path = output_dir / f"{stem}.json"

    mean = sum(capture.samples) / len(capture.samples)
    deltas = [abs(sample - mean) for sample in capture.samples]
    peak_delta = max(deltas)
    peak_index = deltas.index(peak_delta)

    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["index", "time_us", "adc_counts", "volts"])
        for i, sample in enumerate(capture.samples):
            time_us = i * 1_000_000.0 / capture.sample_rate_hz
            volts = sample * ADC_VREF / ADC_COUNTS_FULL_SCALE
            writer.writerow([i, f"{time_us:.3f}", sample, f"{volts:.6f}"])

    metadata = {
        "format": "ft50-pico-w-single-sensor-v1",
        "capture_id": capture.capture_id,
        "source": capture.source,
        "sample_rate_hz": capture.sample_rate_hz,
        "total_samples": capture.total_samples,
        "duration_ms": capture.total_samples * 1000.0 / capture.sample_rate_hz,
        "firmware_trigger_index": capture.trigger_index,
        "firmware_trigger_time_us": capture.trigger_index * 1_000_000.0 / capture.sample_rate_hz,
        "mean_adc_counts": mean,
        "peak_delta_counts": peak_delta,
        "peak_index": peak_index,
        "peak_time_us": peak_index * 1_000_000.0 / capture.sample_rate_hz,
        "csv_file": csv_path.name,
        "received_utc": datetime.now(timezone.utc).isoformat(),
    }

    metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    print(
        f"saved capture {capture.capture_id}: {csv_path} "
        f"({capture.total_samples} samples, peak delta {peak_delta:.1f})"
    )


def main():
    parser = argparse.ArgumentParser(description="Receive FreeTarget Pico W acoustic captures")
    parser.add_argument("--port", type=int, default=5005)
    parser.add_argument("--output", type=Path, default=Path("captures"))
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", args.port))

    print(f"listening for FT50 UDP captures on port {args.port}")
    print(f"output directory: {args.output.resolve()}")

    captures = {}

    while True:
        packet, address = sock.recvfrom(2048)
        if len(packet) < HEADER.size:
            continue

        (
            magic,
            version,
            flags,
            header_bytes,
            capture_id,
            sample_rate_hz,
            total_samples,
            sample_offset,
            sample_count,
            trigger_index,
        ) = HEADER.unpack_from(packet)

        if magic != MAGIC or version != VERSION or header_bytes != HEADER.size:
            continue

        expected_size = HEADER.size + sample_count * 2
        if len(packet) != expected_size:
            continue

        values = list(struct.unpack_from(f"<{sample_count}H", packet, HEADER.size))
        source = f"{address[0]}:{address[1]}"

        capture = captures.get(capture_id)
        if capture is None:
            capture = Capture.create(
                capture_id,
                sample_rate_hz,
                total_samples,
                trigger_index,
                source,
            )
            captures[capture_id] = capture
            print(
                f"receiving capture {capture_id} from {source}: "
                f"{total_samples} samples @ {sample_rate_hz} Hz"
            )

        try:
            capture.add(sample_offset, values)
        except ValueError:
            captures.pop(capture_id, None)
            continue

        if capture.complete:
            save_capture(capture, args.output)
            captures.pop(capture_id, None)


if __name__ == "__main__":
    main()
