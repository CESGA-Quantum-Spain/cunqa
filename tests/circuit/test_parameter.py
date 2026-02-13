#circuit/test_core.py
import os, sys

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

from sympy import symbols, Symbol
from cunqa.circuit.parameter import Param


def test_initialization_sets_expr_and_none_value():
    x = Symbol("x")
    p = Param(x)

    assert p.expr == x
    assert p.value is None


def test_variables_returns_free_symbols_single_variable():
    x = Symbol("x")
    p = Param(x)

    assert p.variables == {x}


def test_variables_multiple_symbols():
    x, y = symbols("x y")
    expr = x + y
    p = Param(expr)

    assert p.variables == {x, y}


def test_eval_sets_value_correctly():
    x = Symbol("x")
    p = Param(x + 1)

    p.eval({x: 3})

    assert p.value == 4


def test_eval_with_multiple_variables():
    x, y = symbols("x y")
    p = Param(x * y + 2)

    p.eval({x: 2, y: 5})

    assert p.value == 12


def test_assign_value_sets_value_directly():
    x = Symbol("x")
    p = Param(x)

    p.assign_value(3.14)

    assert p.value == 3.14


def test_float_conversion_after_assign():
    x = Symbol("x")
    p = Param(x)

    p.assign_value(2.5)

    assert float(p) == 2.5


def test_float_conversion_after_eval():
    x = Symbol("x")
    p = Param(x + 1)

    p.eval({x: 1})

    assert float(p) == 2.0


def test_eval_without_all_symbols_keeps_symbolic_expression():
    x, y = symbols("x y")
    p = Param(x + y)

    p.eval({x: 1})

    # y remains symbolic
    assert p.value == 1 + y


def test_overwriting_value_with_assign_value():
    x = Symbol("x")
    p = Param(x + 1)

    p.eval({x: 1})
    assert p.value == 2

    p.assign_value(10)
    assert p.value == 10
