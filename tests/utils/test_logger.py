# tests/utils/test_logger.py
"""
Tests for :py:mod:`cunqa.utils.logger`.

Template covering the module-level ``logger`` and the :py:class:`ColoredFormatter`. Extend it
if more logging behaviour is added.
"""
import os, sys
import logging
import pytest

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

import cunqa.utils.logger as logger_mod
from cunqa.utils.logger import logger, ColoredFormatter


# ------------------------
# logger configuration
# ------------------------

def test_logger_is_named_and_non_propagating():
    assert isinstance(logger, logging.Logger)
    assert logger.name == "custom_logger"
    assert logger.propagate is False


def test_logger_default_level_is_warning():
    assert logger.level == logging.WARNING


def test_logger_has_a_stream_handler():
    assert any(isinstance(h, logging.StreamHandler) for h in logger.handlers)


# ------------------------
# ColoredFormatter
# ------------------------

def _make_record(level, msg="hello"):
    return logging.LogRecord(
        name="custom_logger", level=level, pathname="/some/file.py",
        lineno=10, msg=msg, args=(), exc_info=None,
    )


def test_formatter_lowercases_levelname_and_keeps_message():
    formatter = ColoredFormatter("%(levelname)s: %(message)s")
    out = formatter.format(_make_record(logging.WARNING, "be careful"))

    assert "warning" in out
    assert "be careful" in out


def test_formatter_includes_path_and_line_for_errors():
    formatter = ColoredFormatter("%(levelname)s: %(message)s")
    out = formatter.format(_make_record(logging.ERROR, "boom"))

    assert "/some/file.py:10" in out


def test_formatter_does_not_mutate_record_levelname():
    formatter = ColoredFormatter("%(levelname)s: %(message)s")
    record = _make_record(logging.WARNING)

    formatter.format(record)

    # levelname is restored after formatting.
    assert record.levelname == "WARNING"
