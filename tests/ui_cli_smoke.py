#!/usr/bin/env python3
import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True)
    parser.add_argument("--cli", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--capture-dir", required=True)
    parser.add_argument("--control-dir", required=True)
    args = parser.parse_args()
    control_dir = Path(args.control_dir)
    shutil.rmtree(control_dir, ignore_errors=True)

    cmd = [
        sys.executable,
        args.cli,
        "smoke",
        "--exe",
        args.exe,
        "--out",
        args.out,
        "--capture-dir",
        args.capture_dir,
        "--control-dir",
        str(control_dir),
        "--capture-count",
        "2",
        "--timeout",
        "10",
        "--video-driver",
        "dummy",
        "--render-driver",
        "software",
        "--no-dm",
    ]
    completed = subprocess.run(cmd, text=True, capture_output=True)
    if completed.returncode != 0:
        sys.stderr.write(completed.stdout)
        sys.stderr.write(completed.stderr)
        return completed.returncode

    result = json.loads(completed.stdout)
    screenshot = Path(result["screenshot"])
    if not screenshot.exists() or screenshot.stat().st_size <= 0:
        raise RuntimeError(f"missing screenshot: {screenshot}")
    for frame in result["frames"]:
        frame_path = Path(frame)
        if not frame_path.exists() or frame_path.stat().st_size <= 0:
            raise RuntimeError(f"missing capture frame: {frame_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
