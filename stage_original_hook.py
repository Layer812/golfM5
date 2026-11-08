#!/usr/bin/env python3
"""Stage the unmodified, verified GitHub sample then add ONE post-drawString hook.

Only changes in the staging copy:
  - board.cpp: insert 2 lines after the exact original line 82.
  - add the 3 original golf source files to firmware/shared.
Never modify the original sample_repo or any other projects.
"""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

COMMIT = "f2549b0bfba6f5cfc201f51b975fa2b134a2d3a5"
EXPECTED = {
    "platformio.ini": "78916a054922669d7a7ee507ef338fb42a6c71b9",
    "boards/m5stack-stopwatch.json": "f6b266fd427e5e2879aaba2421d5b5b652e61106",
    "firmware/shared/board.cpp": "cb5489542b11880b83a2d83e618aa10b705ec8d4",
    "firmware/shared/board.hpp": "cff1527163ef1b95cdeb5b95d2e5450182dc6281",
    "firmware/apps/00_smoke/main.cpp": "d3607a10656aef4fe03ddcdb1c013001924c9f23",
}
LINE82 = b"    M5.Display.drawString(title, w / 2, 66);"
INSERT = (b"    extern void startBallsEyeGolfFromVisibleSample();\n"
          b"    startBallsEyeGolfFromVisibleSample();\n")

def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args], stderr=subprocess.STDOUT)

def blob_sha(data):
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()

def main():
    here = Path(__file__).resolve().parent
    # Use the known-successful original repository if it still exists.
    prior = here.parent / "STOPWATCH_ORIGINAL_GITHUB_00_SMOKE_COM16" / "sample_repo"
    repo = prior if (prior / ".git").is_dir() else here / "original_sample_git"
    if not (repo / ".git").is_dir():
        if repo.exists():
            raise RuntimeError("STOP: original_sample_git exists without .git; not touching it")
        subprocess.run(["git", "clone", "https://github.com/MAPLE0212/m5stack-stopwatch-platformio.git", str(repo)], check=True)
        subprocess.run(["git", "-C", str(repo), "checkout", "--detach", COMMIT], check=True)
    head = git(repo, "rev-parse", "HEAD").decode().strip()
    if head != COMMIT:
        raise RuntimeError("STOP: Git source differs from the displayed original sample. Nothing staged.")
    def original(path):
        return git(repo, "show", COMMIT + ":" + path)
    for rel, expect in EXPECTED.items():
        actual = blob_sha(original(rel))
        if actual != expect:
            raise RuntimeError(f"STOP: original Git blob mismatch {rel} {actual} != {expect}")
    print("PASS: all 5 Git original file hashes match the displayed sample", flush=True)
    # Reconstruct a new isolated stage from exact Git blobs. No original file writes.
    stage = here / "golf_build_stage"
    temp = here / "golf_build_stage.new"
    if temp.exists():
        shutil.rmtree(temp)
    temp.mkdir()
    try:
        names = git(repo, "ls-tree", "-r", "--name-only", COMMIT).decode().splitlines()
        for path in names:
            dst = temp / path
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_bytes(original(path))
        board = temp / "firmware/shared/board.cpp"
        raw = board.read_bytes()
        if raw.count(LINE82) != 1 or blob_sha(raw) != EXPECTED["firmware/shared/board.cpp"]:
            raise RuntimeError("STOP: original line 82 is not exact")
        # Validate that the exact drawString expression is at original line 82.
        ls = raw.splitlines(keepends=True)
        if len(ls) < 83 or ls[81].rstrip(b"\r\n") != LINE82:
            raise RuntimeError("STOP: original board.cpp line 82 does not match")
        updated_source = ls[:82] + INSERT.splitlines(keepends=True) + ls[82:]
        board.write_bytes(b"".join(updated_source))
        for name in ("golf_game.cpp", "GamePhysics.hpp", "GameControls.hpp"):
            shutil.copyfile(here / name, temp / "firmware/shared" / name)
        # Nothing except board.cpp and the three added files can differ.
        for path in names:
            if path != "firmware/shared/board.cpp" and (temp / path).read_bytes() != original(path):
                raise RuntimeError("STOP: non-hook original file changed: " + path)
        updated = board.read_bytes().splitlines(keepends=True)
        if updated[:82] != ls[:82] or updated[82:84] != INSERT.splitlines(keepends=True) or updated[84:] != ls[82:]:
            raise RuntimeError("STOP: board.cpp change outside insertion")
        if stage.exists():
            shutil.rmtree(stage)  # Our own previous isolated stage only.
        temp.rename(stage)
    except BaseException:
        if temp.exists():
            shutil.rmtree(temp)
        raise
    print("PASS: original board.cpp lines 1-82 and all other original files UNCHANGED")
    print("PATCH: two added calls immediately AFTER original line 82")
    print("ADD: golf_game.cpp, GamePhysics.hpp, GameControls.hpp")
    print("PASS: original platformio.ini, board JSON, 00_smoke main.cpp and library versions untouched")
    print("STAGE:", stage)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("SAFE STOP:", e, file=sys.stderr)
        sys.exit(1)
