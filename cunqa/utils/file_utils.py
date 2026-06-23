from __future__ import annotations

import json
import os
import tempfile
import fcntl
import hashlib

from typing import Any, Optional

from cunqa.utils.constants import CUNQA_PATH
from cunqa.utils.logger import logger

LOCK_DIR = os.path.join(CUNQA_PATH, "locks")
os.makedirs(LOCK_DIR, exist_ok=True)


def _lockpath_for(filepath: str) -> str:
    """
    Map a data file's absolute path to a unique lock file path inside
    LOCK_DIR, avoiding collisions between files that share a basename
    in different directories.
    """
    abspath = os.path.abspath(filepath)
    digest = hashlib.sha256(abspath.encode("utf-8")).hexdigest()[:16]
    basename = os.path.basename(abspath)
    return os.path.join(LOCK_DIR, f"{basename}.{digest}.lock")


def read_json(filepath: str) -> Optional[Any]:
    lockpath = _lockpath_for(filepath)

    with open(lockpath, "a+", encoding="utf-8") as lock_f:
        fcntl.lockf(lock_f, fcntl.LOCK_SH)
        try:
            try:
                with open(filepath, "r", encoding="utf-8") as f:
                    return json.load(f)
            except FileNotFoundError:
                if logger:
                    logger.warning(f"File not found: {filepath}")
                return None
            except json.JSONDecodeError:
                if logger:
                    logger.warning(f"Empty or invalid JSON in file: {filepath}")
                return None
        finally:
            fcntl.lockf(lock_f, fcntl.LOCK_UN)


def write_json(filepath: str, data: dict, indent: int = 4) -> None:
    parent_dir = os.path.dirname(filepath) or "."
    os.makedirs(parent_dir, exist_ok=True)

    lockpath = _lockpath_for(filepath)

    with open(lockpath, "a+", encoding="utf-8") as lock_f:
        fcntl.lockf(lock_f, fcntl.LOCK_EX)
        try:
            fd, tmp_path = tempfile.mkstemp(prefix=".tmp_", dir=parent_dir, text=True)

            try:
                with os.fdopen(fd, "w", encoding="utf-8") as tmp_f:
                    json.dump(data, tmp_f, ensure_ascii=False, indent=indent)
                    tmp_f.write("\n")
                    tmp_f.flush()
                    os.fsync(tmp_f.fileno())

                os.chmod(tmp_path, 0o644)
                os.replace(tmp_path, filepath)

                dir_fd = os.open(parent_dir, os.O_DIRECTORY)
                try:
                    os.fsync(dir_fd)
                finally:
                    os.close(dir_fd)

            except Exception:
                try:
                    if os.path.exists(tmp_path):
                        os.remove(tmp_path)
                except Exception:
                    pass
                raise
        finally:
            fcntl.lockf(lock_f, fcntl.LOCK_UN)