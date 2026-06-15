# tests/utils/test_file_utils.py
"""
Tests for :py:mod:`cunqa.utils.file_utils`.

Template covering :py:func:`~cunqa.utils.file_utils.read_json` and
:py:func:`~cunqa.utils.file_utils.write_json`. These helpers do real (locked) file I/O, so
the tests use pytest's ``tmp_path`` fixture rather than mocking ``open``.
"""
import os, sys
import json
import pytest

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

from cunqa.utils.file_utils import read_json, write_json


# ------------------------
# write_json / read_json round-trip
# ------------------------

def test_write_then_read_round_trip(tmp_path):
    filepath = str(tmp_path / "data.json")
    data = {"a": 1, "b": [1, 2, 3], "c": {"nested": True}}

    write_json(filepath, data)
    assert read_json(filepath) == data


def test_write_json_creates_parent_directories(tmp_path):
    filepath = str(tmp_path / "sub" / "dir" / "data.json")

    write_json(filepath, {"x": 1})

    assert os.path.exists(filepath)
    assert read_json(filepath) == {"x": 1}


def test_write_json_overwrites_existing_content(tmp_path):
    filepath = str(tmp_path / "data.json")

    write_json(filepath, {"first": 1})
    write_json(filepath, {"second": 2})

    assert read_json(filepath) == {"second": 2}


# ------------------------
# read_json error branches
# ------------------------

def test_read_json_missing_file_returns_none(tmp_path):
    assert read_json(str(tmp_path / "does_not_exist.json")) is None


def test_read_json_invalid_json_returns_none(tmp_path):
    filepath = tmp_path / "broken.json"
    filepath.write_text("{not valid json", encoding="utf-8")

    assert read_json(str(filepath)) is None
