from __future__ import annotations

import json
import os
import tempfile
import fcntl

from typing import Any, Optional

from cunqa.utils.logger import logger

def read_json(filepath: str) -> Optional[Any]:
    lockpath = filepath + ".lock"

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

    lockpath = filepath + ".lock"

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
