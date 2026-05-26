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


def resolve_pointer_target(control_dir: Path, args: argparse.Namespace) -> tuple[float, float, dict | None]:
    has_xy = args.x is not None or args.y is not None
    has_target = args.target or args.target_id is not None
    if has_xy and (args.x is None or args.y is None):
        raise RuntimeError("pointer coordinates require both --x and --y")
    if has_xy and has_target:
        raise RuntimeError("use either pointer coordinates or a target, not both")
    if has_xy:
        return float(args.x), float(args.y), None
    if not has_target:
        raise RuntimeError("pointer requires --x/--y, --target, or --target-id")

    state = send_command(control_dir, "inspect", {}, args.timeout)
    focusables = state.get("result", {}).get("focusables", [])
    matches = []
    for element in focusables:
        if args.target_id is not None and int(element.get("id", 0)) != args.target_id:
            continue
        if args.target and element.get("name") != args.target:
            continue
        if args.index is not None and int(element.get("offset", 0)) != args.index:
            continue
        matches.append(element)

    if not matches:
        target = args.target or str(args.target_id)
        suffix = "" if args.index is None else f" index {args.index}"
        raise RuntimeError(f"pointer target not found: {target}{suffix}")
    if len(matches) > 1:
        labels = ", ".join(
            f"{item.get('name')}#{item.get('offset')}({item.get('id')})"
            for item in matches[:6]
        )
        raise RuntimeError(f"pointer target is ambiguous: {labels}")

    element = matches[0]
    rect = element.get("rect", {})
    x = float(rect.get("x", 0.0)) + float(rect.get("w", 0.0)) * 0.5
    y = float(rect.get("y", 0.0)) + float(rect.get("h", 0.0)) * 0.5
    return x, y, element


def discord_send_script() -> Path | None:
    configured = os.environ.get("DISCORD_DM_SEND")
    if configured:
        path = Path(configured).expanduser()
        return path if path.exists() else None

    candidates: list[Path] = []
    plugin_root = os.environ.get("CLAUDE_PLUGIN_ROOT")
    if plugin_root:
        candidates.append(Path(plugin_root).expanduser() / "skills" / "discord-dm" / "send.ts")
    codex_home = Path(os.environ.get("CODEX_HOME", Path.home() / ".codex")).expanduser()
    candidates.append(codex_home / "skills" / "discord-dm" / "send.ts")
    candidates.append(Path("/Users/hv/repos/hv-skills/discord-dm/skills/discord-dm/send.ts"))

    for path in candidates:
        if path.exists():
            return path
    return None


def dm_disabled_by_env() -> bool:
    value = os.environ.get("SDL3_CLAY_UI_CLI_DM", "1").strip().lower()
    return value in {"0", "false", "no", "off"}


def should_auto_dm(args: argparse.Namespace) -> bool:
    return not getattr(args, "no_dm", False) and not dm_disabled_by_env()


def convert_bmp_for_discord(path: Path) -> Path:
    if path.suffix.lower() != ".bmp":
        return path
    sips = shutil.which("sips")
    if not sips:
        return path
    out = path.with_suffix(".png")
    completed = subprocess.run(
        [sips, "-s", "format", "png", str(path), "--out", str(out)],
        text=True,
        capture_output=True,
    )
    return out if completed.returncode == 0 and out.exists() else path


def send_artifacts_to_discord(paths: list[Path],
                              message: str,
                              timeout: float,
                              require: bool = False) -> dict:
    existing = [path for path in paths if path.exists() and path.stat().st_size > 0]
    if not existing:
        result = {"ok": False, "skipped": True, "reason": "no artifact files"}
        if require:
            raise RuntimeError(result["reason"])
        return result

    script = discord_send_script()
    bun = os.environ.get("BUN") or shutil.which("bun")
    if not script or not bun:
        result = {
            "ok": False,
            "skipped": True,
            "reason": "discord sender is not configured",
        }
        if require:
            raise RuntimeError(result["reason"])
        return result

    attachments = [convert_bmp_for_discord(path) for path in existing]
    cmd = [bun, str(script), message, *[str(path) for path in attachments]]
    completed = subprocess.run(cmd, text=True, capture_output=True, timeout=timeout)
    stdout = completed.stdout.strip()
    stderr = completed.stderr.strip()
    if completed.returncode != 0:
        result = {"ok": False, "error": stderr or stdout or "discord send failed"}
        if require:
            raise RuntimeError(result["error"])
        return result

    message_id = ""
    parts = stdout.split()
    if len(parts) >= 2 and parts[0] == "sent":
        message_id = parts[1]
    return {
        "ok": True,
        "message_id": message_id,
        "attachments": [str(path) for path in attachments],
    }


def add_common(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--control-dir", required=True)
    parser.add_argument("--timeout", type=float, default=5.0)


def add_dm_options(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--no-dm", action="store_true",
                        help="do not send generated artifacts to Discord")
    parser.add_argument("--require-dm", action="store_true",
                        help="fail if generated artifacts cannot be sent to Discord")
    parser.add_argument("--dm-message", default="",
                        help="message to send with generated artifacts")
    parser.add_argument("--dm-timeout", type=float, default=20.0)


def maybe_attach_dm_result(reply: dict,
                           args: argparse.Namespace,
                           paths: list[Path],
                           default_message: str) -> dict:
    if not should_auto_dm(args):
        return reply
    message = args.dm_message or default_message
    dm = send_artifacts_to_discord(paths, message, args.dm_timeout, args.require_dm)
    reply.setdefault("result", {})["discord_dm"] = dm
    return reply


def command_main(args: argparse.Namespace) -> int:
    control_dir = Path(args.control_dir)
    op = args.command
    wire_op = op
    payload: dict = {}
    if op == "key":
        payload = {"key": args.key, "action": args.action}
    elif op == "gamepad":
        payload = {"button": args.button, "action": args.action}
    elif op == "pointer":
        x, y, target = resolve_pointer_target(control_dir, args)
        if args.action == "click":
            press = send_command(control_dir, "pointer", {"x": x, "y": y, "action": "press"}, args.timeout)
            release = send_command(control_dir, "pointer", {"x": x, "y": y, "action": "release"}, args.timeout)
            reply = {
                "id": release.get("id"),
                "ok": True,
                "result": {
                    "accepted": True,
                    "x": x,
                    "y": y,
                    "target": target,
                    "press": press.get("result", {}),
                    "release": release.get("result", {}),
                },
            }
            print(json.dumps(reply, sort_keys=True))
            return 0
        payload = {"x": x, "y": y, "action": args.action}
    elif op == "resize":
        payload = {"w": args.w, "h": args.h}
    elif op in ("wait_frames", "wait", "step"):
        wire_op = op
        payload = {"n": args.n}
    elif op == "screenshot":
        payload = {"out": args.out}
    elif op == "capture_frames":
        payload = {"out_dir": args.out_dir, "count": args.count}
    reply = send_command(control_dir, wire_op, payload, args.timeout)
    if op == "screenshot":
        out = Path(reply.get("result", {}).get("out", args.out))
        reply = maybe_attach_dm_result(
            reply, args, [out], f"sdl3-clay UI screenshot: {out.name}")
    elif op == "capture_frames":
        frames = [Path(path) for path in reply.get("result", {}).get("frames", [])]
        reply = maybe_attach_dm_result(
            reply, args, frames, f"sdl3-clay UI frame capture: {len(frames)} frame(s)")
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

        result = {
            "ok": True,
            "control_dir": str(control_dir),
            "state": state.get("result", {}),
            "screenshot": shot.get("result", {}).get("out", str(out)),
            "frames": frame_paths,
            "exit_code": proc.returncode,
        }
        if should_auto_dm(args):
            paths = [Path(result["screenshot"]), *[Path(path) for path in frame_paths]]
            result["discord_dm"] = send_artifacts_to_discord(
                paths,
                args.dm_message or "sdl3-clay UI smoke proof",
                args.dm_timeout,
                args.require_dm,
            )
        print(json.dumps(result, sort_keys=True))
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

    p = sub.add_parser("gamepad")
    add_common(p)
    p.add_argument("--button", required=True)
    p.add_argument("--action", choices=["press", "down", "up", "release"], default="press")
    p.set_defaults(func=command_main)

    p = sub.add_parser("pointer")
    add_common(p)
    p.add_argument("--x", type=float)
    p.add_argument("--y", type=float)
    p.add_argument("--target",
                   help="focusable element name from inspect output, such as GearTab")
    p.add_argument("--target-id", type=int,
                   help="numeric focusable element id from inspect output")
    p.add_argument("--index", type=int,
                   help="Clay id offset for indexed focusables, such as WeaponTile --index 3")
    p.add_argument("--action", choices=["move", "press", "release", "click"], default="move")
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

    for name in ("wait", "step"):
        p = sub.add_parser(name)
        add_common(p)
        p.add_argument("--frames", "--n", dest="n", type=int, default=1)
        p.set_defaults(func=command_main)

    p = sub.add_parser("screenshot")
    add_common(p)
    p.add_argument("--out", required=True)
    add_dm_options(p)
    p.set_defaults(func=command_main)

    p = sub.add_parser("capture_frames")
    add_common(p)
    p.add_argument("--out-dir", required=True)
    p.add_argument("--count", type=int, default=3)
    add_dm_options(p)
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
    add_dm_options(p)
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
