from __future__ import annotations
from functools import singledispatch
import copy

from qiskit import QuantumCircuit
from .core import CunqaCircuit
from ..qiskit_deps.converter import qc_to_json

from cunqa.logger import logger

@singledispatch
def to_ir(circuit: object) -> dict:
    meth = getattr(circuit, "to_ir", None)
    if callable(meth):
        return meth()

    raise TypeError(
        f"Not a method to convert {type(circuit).__name__} to dict."
    )

@to_ir.register
def _(c: CunqaCircuit) -> dict:
    return copy.deepcopy(c.info)

@to_ir.register
def _(c: QuantumCircuit) -> dict:
    return copy.deepcopy(qc_to_json(c))

@to_ir.register
def _(c: dict) -> dict:
    logger.warning("Circuit is already in IR format, returning it as is.")
    return c