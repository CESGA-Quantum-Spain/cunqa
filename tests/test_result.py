# test_result.py

import os, sys
import pytest

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

from cunqa.result import Result


def test_result_init_raises_on_none():
    with pytest.raises(ValueError) as excinfo:
        Result(None, circ_id="c1", registers={"c": [0]})

def test_result_init_raises_on_empty_dict():
    with pytest.raises(ValueError) as excinfo:
        Result({}, circ_id="c1", registers={"c": [0]})

def test_result_init_raises_on_error_key():
    with pytest.raises(RuntimeError) as excinfo:
        Result({"ERROR": "no backend"}, circ_id="c1", registers={"c": [0]})

def test_counts_from_results_key():
    result_dict = {
        "results": [
            {"data": {"counts": {"000": 34, "111": 66}}, "time_taken": 0.056}
        ]
    }
    r = Result(result_dict, circ_id="circA", registers={"c": [0, 1, 2]})
    assert r.counts == {"000": 34, "111": 66}


def test_counts_from_counts_key():
    result_dict = {"counts": {"0": 1, "1": 2}, "time_taken": 1.23}
    r = Result(result_dict, circ_id="circB", registers={"c": [0]})
    assert r.counts == {"0": 1, "1": 2}


def test_counts_for_multiple_registers():
    # Two classical registers: lengths 3 and 2 -> "00111" becomes "001 11"
    result_dict = {"counts": {"00111": 23, "11010": 77}, "time_taken": 0.5}
    registers = {"c0": [0, 1, 2], "c1": [0, 1]}

    r = Result(result_dict, circ_id="circC", registers=registers)

    assert r.counts == {"001 11": 23, "110 10": 77}


def test_counts_with_empty_registers():
    # registers == {} -> lengths list is empty -> should return counts unchanged
    result_dict = {"counts": {"00111": 23}, "time_taken": 0.5}
    r = Result(result_dict, circ_id="circD", registers={})
    assert r.counts == {"00111": 23}


def test_counts_raises_on_unknown_format():
    # Missing both "results" and "counts"
    r = Result({"foo": "bar"}, circ_id="circE", registers={"c": [0]})
    with pytest.raises(RuntimeError) as _:
        _ = r.counts


def test_time_taken_from_results_key():
    result_dict = {
        "results": [
            {"data": {"counts": {"0": 1}}, "time_taken": 0.056}
        ]
    }
    r = Result(result_dict, circ_id="circF", registers={"c": [0]})
    assert r.time_taken == 0.056


def test_time_taken_from_counts_key():
    result_dict = {"counts": {"0": 1}, "time_taken": 1.23}
    r = Result(result_dict, circ_id="circG", registers={"c": [0]})
    assert r.time_taken == 1.23


def test_time_taken_raises_on_unknown_format():
    r = Result({"foo": "bar"}, circ_id="circH", registers={"c": [0]})
    with pytest.raises(RuntimeError) as _:
        _ = r.time_taken


def test_str_contains_id_counts_and_time_taken():
    result_dict = {"counts": {"000": 1}, "time_taken": 0.1}
    r = Result(result_dict, circ_id="circI", registers={"c": [0, 1, 2]})

    s = str(r)
    assert "circI" in s
    assert "counts" in s
    assert "time_taken" in s
    assert "0.1" in s
