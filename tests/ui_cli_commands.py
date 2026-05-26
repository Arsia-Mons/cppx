#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path


def wait_ready(control_dir: Path, timeout: float) -> None:
    deadline = time.monotonic() + timeout
    ready = control_dir / "ready.json"
    while time.monotonic() < deadline:
        if ready.exists():
            return
        time.sleep(0.02)
    raise TimeoutError(f"timed out waiting for {ready}")


def run_cli(cli: Path, control_dir: Path, *args: str, expect_ok: bool = True) -> dict:
    cmd = [sys.executable, str(cli), *args, "--control-dir", str(control_dir), "--timeout", "10"]
    completed = subprocess.run(cmd, text=True, capture_output=True)
    if expect_ok and completed.returncode != 0:
        sys.stderr.write(completed.stdout)
        sys.stderr.write(completed.stderr)
        raise RuntimeError(f"CLI failed: {' '.join(cmd)}")
    if not expect_ok:
        if completed.returncode == 0:
            raise RuntimeError(f"CLI unexpectedly passed: {' '.join(cmd)}")
        return json.loads(completed.stderr)
    return json.loads(completed.stdout)


def wait_frame(cli: Path, control_dir: Path) -> dict:
    return run_cli(cli, control_dir, "wait_frames", "--n", "1")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True)
    parser.add_argument("--cli", required=True)
    parser.add_argument("--control-dir", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--capture-dir", required=True)
    args = parser.parse_args()

    exe = Path(args.exe)
    cli = Path(args.cli)
    control_dir = Path(args.control_dir)
    out = Path(args.out)
    capture_dir = Path(args.capture_dir)
    shutil.rmtree(control_dir, ignore_errors=True)
    shutil.rmtree(capture_dir, ignore_errors=True)
    out.unlink(missing_ok=True)
    control_dir.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env["SDL_VIDEODRIVER"] = "dummy"
    env["SDL_RENDER_DRIVER"] = "software"
    log = (control_dir / "hello.log").open("w", encoding="utf-8")
    proc = subprocess.Popen(
        [str(exe), "--control-dir", str(control_dir)],
        env=env,
        stdout=log,
        stderr=subprocess.STDOUT,
    )
    try:
        wait_ready(control_dir, 10)

        state = run_cli(cli, control_dir, "state")
        result = state["result"]
        if result["screen_count"] != 1 or result["top_screen"] != "ShooterGame":
            raise RuntimeError(f"unexpected state: {result}")

        run_cli(cli, control_dir, "key", "--key", "enter")
        wait_frame(cli, control_dir)
        state = run_cli(cli, control_dir, "state")
        result = state["result"]
        if result["screen_count"] != 2 or result["top_screen"] != "Pause":
            raise RuntimeError(f"pause did not open: {result}")

        run_cli(cli, control_dir, "key", "--key", "down")
        wait_frame(cli, control_dir)
        run_cli(cli, control_dir, "key", "--key", "enter")
        wait_frame(cli, control_dir)
        state = run_cli(cli, control_dir, "state")
        result = state["result"]
        if result["screen_count"] != 3 or result["top_screen"] != "Options":
            raise RuntimeError(f"options did not open from pause: {result}")

        run_cli(cli, control_dir, "key", "--key", "down")
        wait_frame(cli, control_dir)
        run_cli(cli, control_dir, "key", "--key", "down")
        wait_frame(cli, control_dir)
        run_cli(cli, control_dir, "key", "--key", "enter")
        wait_frame(cli, control_dir)
        state = run_cli(cli, control_dir, "state")
        result = state["result"]
        if result["screen_count"] != 2 or result["top_screen"] != "Pause":
            raise RuntimeError(f"options did not return to pause: {result}")

        run_cli(cli, control_dir, "key", "--key", "t")
        run_cli(cli, control_dir, "pointer", "--x", "80", "--y", "90", "--action", "move")
        run_cli(cli, control_dir, "pointer", "--x", "80", "--y", "90", "--action", "press")
        run_cli(cli, control_dir, "pointer", "--x", "120", "--y", "130", "--action", "release")
        run_cli(cli, control_dir, "resize", "--w", "640", "--h", "360")
        waited = run_cli(cli, control_dir, "wait_frames", "--n", "2")
        if waited["result"]["frame"] < result["frame"] + 1:
            raise RuntimeError(f"wait_frames did not advance: {waited}")

        shot = run_cli(cli, control_dir, "screenshot", "--out", str(out), "--no-dm")
        shot_path = Path(shot["result"]["out"])
        if not shot_path.exists() or shot_path.stat().st_size <= 0:
            raise RuntimeError(f"screenshot missing: {shot_path}")

        capture = run_cli(
            cli,
            control_dir,
            "capture_frames",
            "--out-dir",
            str(capture_dir),
            "--count",
            "2",
            "--no-dm",
        )
        frames = capture["result"]["frames"]
        if len(frames) != 2:
            raise RuntimeError(f"wrong capture count: {frames}")
        for frame in frames:
            frame_path = Path(frame)
            if not frame_path.exists() or frame_path.stat().st_size <= 0:
                raise RuntimeError(f"capture frame missing: {frame_path}")

        error = run_cli(cli, control_dir, "key", "--key", "not-a-key", expect_ok=False)
        if "BAD_KEY" not in error.get("error", "") and "BAD_KEY" not in error.get("code", ""):
            raise RuntimeError(f"bad key did not report BAD_KEY: {error}")

        run_cli(cli, control_dir, "quit")
        proc.wait(timeout=10)
        if proc.returncode != 0:
            raise RuntimeError(f"hello exited with {proc.returncode}")
        return 0
    finally:
        if proc.poll() is None:
            proc.kill()
        log.close()


if __name__ == "__main__":
    raise SystemExit(main())
