#!/usr/bin/env python3
import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path


_next_id = int(time.time() * 1000) % 1_000_000_000


def next_id() -> int:
    global _next_id
    _next_id += 1
    return _next_id


def write_atomic(path: Path, data: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(data, encoding="utf-8")
    os.replace(tmp, path)


def wait_for_file(path: Path, timeout: float) -> dict:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if path.exists():
            return json.loads(path.read_text(encoding="utf-8"))
        time.sleep(0.02)
    raise TimeoutError(f"timed out waiting for {path}")


def wait_ready(control_dir: Path, timeout: float) -> dict:
    return wait_for_file(control_dir / "ready.json", timeout)


def send_command(control_dir: Path, op: str, args: dict, timeout: float) -> dict:
    ready = wait_ready(control_dir, timeout)
    req_id = next_id()
    request = {"id": req_id, "op": op}
    request.update(args)
    write_atomic(Path(ready["requests"]) / f"{req_id}.json", json.dumps(request) + "\n")
    reply = wait_for_file(Path(ready["replies"]) / f"{req_id}.json", timeout)
    if not reply.get("ok", False):
        raise RuntimeError(json.dumps(reply))
    return reply


def add_common(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--control-dir", required=True)
    parser.add_argument("--timeout", type=float, default=5.0)


def command_main(args: argparse.Namespace) -> int:
    control_dir = Path(args.control_dir)
    op = args.command
    payload: dict = {}
    if op == "key":
        payload = {"key": args.key, "action": args.action}
    elif op == "pointer":
        payload = {"x": args.x, "y": args.y, "action": args.action}
    elif op == "resize":
        payload = {"w": args.w, "h": args.h}
    elif op == "wait_frames":
        payload = {"n": args.n}
    elif op == "screenshot":
        payload = {"out": args.out}
    elif op == "capture_frames":
        payload = {"out_dir": args.out_dir, "count": args.count}
    reply = send_command(control_dir, op, payload, args.timeout)
    print(json.dumps(reply, sort_keys=True))
    return 0


def smoke(args: argparse.Namespace) -> int:
    control_dir = Path(args.control_dir) if args.control_dir else Path(tempfile.mkdtemp(prefix="sdl3-clay-ui-"))
    control_dir.mkdir(parents=True, exist_ok=True)
    out = Path(args.out) if args.out else control_dir / "smoke.bmp"
    capture_dir = Path(args.capture_dir) if args.capture_dir else control_dir / "capture"

    env = os.environ.copy()
    if args.video_driver:
        env["SDL_VIDEODRIVER"] = args.video_driver
    if args.render_driver:
        env["SDL_RENDER_DRIVER"] = args.render_driver

    log = (control_dir / "hello.log").open("w", encoding="utf-8")
    proc = subprocess.Popen(
        [args.exe, "--control-dir", str(control_dir)],
        env=env,
        stdout=log,
        stderr=subprocess.STDOUT,
    )
    try:
        wait_ready(control_dir, args.timeout)
        send_command(control_dir, "wait_frames", {"n": 2}, args.timeout)
        send_command(control_dir, "key", {"key": "t", "action": "press"}, args.timeout)
        send_command(control_dir, "pointer", {"x": 96, "y": 96, "action": "move"}, args.timeout)
        send_command(control_dir, "pointer", {"x": 96, "y": 96, "action": "press"}, args.timeout)
        send_command(control_dir, "pointer", {"x": 140, "y": 140, "action": "release"}, args.timeout)
        state = send_command(control_dir, "state", {}, args.timeout)
        shot = send_command(control_dir, "screenshot", {"out": str(out)}, args.timeout)
        frames = send_command(
            control_dir,
            "capture_frames",
            {"out_dir": str(capture_dir), "count": args.capture_count},
            args.timeout,
        )
        send_command(control_dir, "quit", {}, args.timeout)
        try:
            proc.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            raise

        if not out.exists() or out.stat().st_size <= 0:
            raise RuntimeError(f"screenshot was not written: {out}")
        frame_paths = frames.get("result", {}).get("frames", [])
        if len(frame_paths) != args.capture_count:
            raise RuntimeError("capture_frames returned the wrong frame count")
        for frame in frame_paths:
            frame_path = Path(frame)
            if not frame_path.exists() or frame_path.stat().st_size <= 0:
                raise RuntimeError(f"capture frame was not written: {frame_path}")

        print(json.dumps({
            "ok": True,
            "control_dir": str(control_dir),
            "state": state.get("result", {}),
            "screenshot": shot.get("result", {}).get("out", str(out)),
            "frames": frame_paths,
            "exit_code": proc.returncode,
        }, sort_keys=True))
        return 0
    except Exception:
        if proc.poll() is None:
            proc.kill()
        raise
    finally:
        log.close()
        if args.clean and not args.control_dir:
            shutil.rmtree(control_dir, ignore_errors=True)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="ui_cli.py")
    sub = parser.add_subparsers(dest="command", required=True)

    for name in ("state", "inspect", "quit"):
        p = sub.add_parser(name)
        add_common(p)
        p.set_defaults(func=command_main)

    p = sub.add_parser("key")
    add_common(p)
    p.add_argument("--key", required=True)
    p.add_argument("--action", choices=["press", "down", "up", "release"], default="press")
    p.set_defaults(func=command_main)

    p = sub.add_parser("pointer")
    add_common(p)
    p.add_argument("--x", type=float, required=True)
    p.add_argument("--y", type=float, required=True)
    p.add_argument("--action", choices=["move", "press", "release"], default="move")
    p.set_defaults(func=command_main)

    p = sub.add_parser("resize")
    add_common(p)
    p.add_argument("--w", type=int, required=True)
    p.add_argument("--h", type=int, required=True)
    p.set_defaults(func=command_main)

    p = sub.add_parser("wait_frames")
    add_common(p)
    p.add_argument("--n", type=int, default=1)
    p.set_defaults(func=command_main)

    p = sub.add_parser("screenshot")
    add_common(p)
    p.add_argument("--out", required=True)
    p.set_defaults(func=command_main)

    p = sub.add_parser("capture_frames")
    add_common(p)
    p.add_argument("--out-dir", required=True)
    p.add_argument("--count", type=int, default=3)
    p.set_defaults(func=command_main)

    p = sub.add_parser("smoke")
    p.add_argument("--exe", required=True)
    p.add_argument("--control-dir")
    p.add_argument("--out")
    p.add_argument("--capture-dir")
    p.add_argument("--capture-count", type=int, default=2)
    p.add_argument("--timeout", type=float, default=10.0)
    p.add_argument("--video-driver", default="")
    p.add_argument("--render-driver", default="")
    p.add_argument("--clean", action="store_true")
    p.set_defaults(func=smoke)
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(json.dumps({"ok": False, "error": str(exc)}), file=sys.stderr)
        raise SystemExit(1)
