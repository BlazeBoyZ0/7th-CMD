#!/usr/bin/env python3
"""Measure the Linux CLI process RSS after startup while it waits for input."""

import os
import subprocess
import sys
import time


def read_kib(pid: int, field: str) -> int:
    with open(f"/proc/{pid}/status", encoding="utf-8") as status:
        for line in status:
            if line.startswith(field + ":"):
                return int(line.split()[1])
    raise RuntimeError(f"{field} was not present in /proc/{pid}/status")


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: measure_runtime.py PATH_TO_EXECUTABLE", file=sys.stderr)
        return 2

    executable = os.path.abspath(sys.argv[1])
    process = subprocess.Popen(
        [executable],
        stdin=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        text=True,
        bufsize=1,
    )

    samples = []
    try:
        # Allow startup, configuration loading, and initial allocations to settle.
        time.sleep(1.0)
        if process.poll() is not None:
            raise RuntimeError(
                f"7th cmd exited during startup with code {process.returncode}"
            )

        deadline = time.monotonic() + 1.5
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError("7th cmd exited before the memory sample completed")
            samples.append(read_kib(process.pid, "VmRSS"))
            time.sleep(0.05)

        peak_kib = read_kib(process.pid, "VmHWM")
    finally:
        if process.poll() is None:
            try:
                process.stdin.write("exit\n")
                process.stdin.flush()
                process.wait(timeout=3)
            except (BrokenPipeError, subprocess.TimeoutExpired):
                process.kill()
                process.wait(timeout=3)
        if process.stdin:
            process.stdin.close()

    average_kib = sum(samples) / len(samples)
    print("Runtime memory measurement (GitHub Actions Ubuntu runner)")
    print(f"RSS average while idle: {average_kib / 1024:.2f} MiB")
    print(f"RSS peak observed while sampling: {max(samples) / 1024:.2f} MiB")
    print(f"Linux VmHWM high-water mark: {peak_kib / 1024:.2f} MiB")
    print(f"Samples: {len(samples)} over approximately 1.5 seconds")
    print("Workload: application startup and idle prompt; no command workload.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
