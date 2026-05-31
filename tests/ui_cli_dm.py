#!/usr/bin/env python3
import importlib.util
import json
import os
import sys
import tempfile
from pathlib import Path


def load_ui_cli(repo_root: Path):
    module_path = repo_root / "tools" / "ui_cli.py"
    spec = importlib.util.spec_from_file_location("ui_cli", module_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"failed to load {module_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    ui_cli = load_ui_cli(repo_root)

    with tempfile.TemporaryDirectory(prefix="sdl3-retained-dm-test-") as tmp:
        root = Path(tmp)
        artifact = root / "shot.png"
        artifact.write_bytes(b"\x89PNG\r\n\x1a\n")
        log = root / "dm-args.json"
        fake_send = root / "fake_send.py"
        fake_send.write_text(
            "import json, os, sys\n"
            "from pathlib import Path\n"
            "Path(os.environ['FAKE_DM_LOG']).write_text(json.dumps(sys.argv[1:]))\n"
            "print('sent 123456789')\n",
            encoding="utf-8",
        )

        old_env = {
            "BUN": os.environ.get("BUN"),
            "DISCORD_DM_SEND": os.environ.get("DISCORD_DM_SEND"),
            "FAKE_DM_LOG": os.environ.get("FAKE_DM_LOG"),
            "SDL3_RETAINED_UI_CLI_DM": os.environ.get("SDL3_RETAINED_UI_CLI_DM"),
        }
        try:
            os.environ["BUN"] = sys.executable
            os.environ["DISCORD_DM_SEND"] = str(fake_send)
            os.environ["FAKE_DM_LOG"] = str(log)
            os.environ["SDL3_RETAINED_UI_CLI_DM"] = "1"

            result = ui_cli.send_artifacts_to_discord(
                [artifact],
                "proof image",
                timeout=5.0,
                require=True,
            )
            if not result["ok"] or result["message_id"] != "123456789":
                raise RuntimeError(f"unexpected dm result: {result}")
            args = json.loads(log.read_text(encoding="utf-8"))
            if args != ["proof image", str(artifact)]:
                raise RuntimeError(f"unexpected fake send args: {args}")
        finally:
            for key, value in old_env.items():
                if value is None:
                    os.environ.pop(key, None)
                else:
                    os.environ[key] = value

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
