# tests/tools/test_probabilities.py
"""
Tests for :py:mod:`cunqa.tools.probabilities`.

Template covering :py:func:`~cunqa.tools.probabilities.probabilities`. The exact-state branches
(statevector / density matrix) are exercised here directly. The count-estimation branch relies on
the compiled ``cunqa.tools.probs_helpers`` extension (``counts_to_probs``, ``recombine_probs``,
``marginalize_counts``); those tests are scaffolded but skipped when the extension is not built.
"""
import os, sys
import numpy as np
import pytest

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

from cunqa.result import Result
from cunqa.tools.probabilities import probabilities


def _has_compiled_probs_helpers():
    """The probs_helpers extension may be stubbed out (functions return None)."""
    from cunqa.tools.probs_helpers import counts_to_probs
    try:
        return counts_to_probs({"0": 1, "1": 1}) is not None
    except Exception:
        return False


# ------------------------
# statevector branch
# ------------------------

def test_probabilities_from_statevector():
    # |00> for a 2-qubit system, encoded as a flat [real, imag, ...] list.
    statevector = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    result = Result({"statevector": statevector}, circ_id="c", registers={"c": [0, 1]})

    probs = probabilities(result)

    assert np.allclose(probs, [1.0, 0.0, 0.0, 0.0])


def test_probabilities_from_statevector_dict_returns_dict():
    statevector = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    result = Result({"statevector": {"label": statevector}}, circ_id="c", registers={"c": [0, 1]})

    probs = probabilities(result)

    assert isinstance(probs, dict)
    assert np.allclose(probs["label"], [1.0, 0.0, 0.0, 0.0])


def test_probabilities_normalizes_to_one():
    # A uniform superposition over 2 qubits: each amplitude 1/2.
    amp = 0.5
    statevector = []
    for _ in range(4):
        statevector += [amp, 0.0]
    result = Result({"statevector": statevector}, circ_id="c", registers={"c": [0, 1]})

    probs = probabilities(result)

    assert np.isclose(np.sum(probs), 1.0)
    assert np.allclose(probs, [0.25, 0.25, 0.25, 0.25])


# ------------------------
# count-estimation branch (needs the compiled probs_helpers extension)
# ------------------------

@pytest.mark.skipif(not _has_compiled_probs_helpers(),
                    reason="requires the compiled cunqa.tools.probs_helpers extension")
def test_probabilities_estimated_from_counts():
    # TODO: assert estimation of probabilities from a counts-only Result once the
    # probs_helpers extension is available in the test environment.
    result = Result({"counts": {"00": 50, "11": 50}, "time_taken": 0.1},
                    circ_id="c", registers={"c": [0, 1]})

    probs = probabilities(result)

    assert np.isclose(np.sum(probs), 1.0)
