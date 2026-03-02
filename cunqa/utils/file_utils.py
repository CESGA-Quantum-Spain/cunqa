# fileUtils.py
from __future__ import annotations

import json
import os
import tempfile
import fcntl
from typing import Any, Optional
from cunqa.logger import logger

def read_json(filepath: str) -> Optional[Any]:
    """
    Read JSON content from `filepath` using a shared lock (LOCK_SH).

    Returns:
        - The parsed JSON object (dict/list/...) if successful.
        - None if the file does not exist, is empty, or contains invalid JSON.
    """
    if not os.path.exists(filepath):
        if logger:
            logger.warning(f"File not found: {filepath}")
        return None

    # Open in read mode. json.load() raises JSONDecodeError on empty/invalid files.
    with open(filepath, "r", encoding="utf-8") as f:
        fcntl.flock(f, fcntl.LOCK_SH)  # shared lock for reading
        try:
            try:
                data = json.load(f)
            except json.JSONDecodeError:
                if logger:
                    logger.warning(f"Empty or invalid JSON in file: {filepath}")
                return None
        finally:
            fcntl.flock(f, fcntl.LOCK_UN)  # always unlock

    try:
        if hasattr(data, "__len__") and len(data) == 0:
            if logger:
                logger.warning("No data were found.")
            return None
    except TypeError:
        # If `data` has no length, ignore.
        pass

    return data


def write_json(filepath: str, data: dict, indent: int = 2) -> None:
    """
    Write `data` as JSON to `filepath` using an exclusive lock (LOCK_EX).

    This implementation is crash-safe:
    - It acquires an exclusive lock on the target file.
    - Writes the JSON to a temporary file in the same directory.
    - Atomically replaces the target file using os.replace().

    Args:
        filepath: Destination file path.
        data: JSON-serializable object (dict/list/...).
        logger: Optional logger compatible with stdlib logging.
        indent: json.dump indent level (default: 2).
    """
    parent_dir = os.path.dirname(filepath) or "."
    os.makedirs(parent_dir, exist_ok=True)

    # Open (or create) the target file only to hold the lock.
    # Use "a+" so we don't truncate before acquiring the lock.
    with open(filepath, "a+", encoding="utf-8") as lock_f:
        fcntl.flock(lock_f, fcntl.LOCK_EX)  # exclusive lock for writing
        try:
            # Create the temp file in the same directory so os.replace() is atomic.
            fd, tmp_path = tempfile.mkstemp(prefix=".tmp_", dir=parent_dir, text=True)
            try:
                with os.fdopen(fd, "w", encoding="utf-8") as tmp_f:
                    json.dump(data, tmp_f, ensure_ascii=False, indent=indent)
                    tmp_f.write("\n")
                    tmp_f.flush()
                    os.fsync(tmp_f.fileno())

                # Atomic replace: either the old file or the new file will exist.
                os.replace(tmp_path, filepath)
            except Exception as e:
                # Cleanup temp file on failure.
                try:
                    if os.path.exists(tmp_path):
                        os.remove(tmp_path)
                except Exception:
                    pass

                if logger:
                    logger.exception(f"Failed writing JSON to {filepath}: {e}")
                raise
        finally:
            fcntl.flock(lock_f, fcntl.LOCK_UN)  # always unlock