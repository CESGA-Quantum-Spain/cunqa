# tests/utils/test_id_utils.py
"""
Tests for :py:mod:`cunqa.utils.id_utils`.

Template covering :py:func:`~cunqa.utils.id_utils.generate_id`. Extend it as new helpers
are added to the module.
"""
import os, sys
import string
import pytest

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

from cunqa.utils.id_utils import generate_id


# ------------------------
# generate_id
# ------------------------

def test_generate_id_default_length():
    assert len(generate_id()) == 4


@pytest.mark.parametrize("size", [1, 5, 16, 64])
def test_generate_id_respects_size(size):
    assert len(generate_id(size)) == size


def test_generate_id_only_alphanumeric():
    allowed = set(string.ascii_letters + string.digits)
    assert set(generate_id(50)) <= allowed


def test_generate_id_is_random():
    # The chance of a collision for two 16-char ids is negligible.
    assert generate_id(16) != generate_id(16)


@pytest.mark.parametrize("size", [0, -1, -10])
def test_generate_id_rejects_non_positive_size(size):
    with pytest.raises(ValueError):
        generate_id(size)
