from __future__ import annotations

import json
import os
import tempfile

from typing import Any, Optional

from cunqa.utils.logger import logger


def read_json(filepath: str) -> Optional[Any]:
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


def write_json(filepath: str, data: dict, indent: int = 4) -> None:
    """
    Atomically replaces the content of the file (temporary file + rename), so readers see either
    the old or the new content, never a partial one.
    """
    parent_dir = os.path.dirname(filepath) or "."
    os.makedirs(parent_dir, exist_ok=True)

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


def read_json_dir(dirpath: str) -> dict:
    """
    Returns ``{<file stem>: <content>}`` with every ``*.json`` file of the directory, skipping
    hidden files (as the temporary ones of :py:func:`write_json`). A missing directory gives ``{}``.
    """
    data = {}
    try:
        filenames = os.listdir(dirpath)
    except (FileNotFoundError, NotADirectoryError):
        return data

    for filename in filenames:
        if filename.startswith(".") or not filename.endswith(".json"):
            continue
        try:
            with open(os.path.join(dirpath, filename), "r", encoding="utf-8") as f:
                data[filename[:-len(".json")]] = json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            continue # Removed between listing and reading
    return data

