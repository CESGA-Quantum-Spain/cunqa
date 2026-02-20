#circuit/test_ir.py
import os, sys

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

# tests/test_to_ir.py

import copy
import pytest
import numpy as np
from unittest.mock import Mock

# --- Adjust this import to the real mod_irule path of the file you pasted ---
# For example: import cunqa.circuit.ir as mod_ir
import cunqa.circuit.ir as mod_ir
from cunqa.circuit import CunqaCircuit
from qiskit import QuantumCircuit


def test_to_ir_fallback_calls_object_to_ir_method():
    # The generic singledispatch fallback should call .to_ir() if present.
    class Foo:
        def to_ir(self):
            return {"ok": True}

    assert mod_ir.to_ir(Foo()) == {"ok": True}


def test_to_ir_fallback_raises_typeerror_when_no_method():
    class Bar:
        pass

    with pytest.raises(TypeError) as excinfo:
        mod_ir.to_ir(Bar())


def test_to_ir_cunqacircuit_returns_deepcopy():
    c = CunqaCircuit(1, num_clbits=1, id="A")
    c.x(0)
    out = mod_ir.to_ir(c)

    # Mutate the returned dict and ensure the circuit info is not affected.
    out["id"] = "MUTATED"
    out["instructions"].append({"name": "x", "qubits": [999]})

    fresh = mod_ir.to_ir(c)
    assert fresh["id"] == "A"
    assert fresh["instructions"][-1] == {"name": "x", "qubits": [0]}


def test_to_ir_dict_returns_same_object_and_warns(monkeypatch):
    logger_mock = Mock()
    monkeypatch.setattr(mod_ir, "logger", logger_mock)

    d = {"id": "X", "instructions": []}
    out = mod_ir.to_ir(d)

    assert out is d
    logger_mock.warning.assert_called_once()


def test_to_ir_quantumcircuit_basic_register_mapping_and_measure(monkeypatch):
    from qiskit import QuantumRegister, ClassicalRegister

    monkeypatch.setattr(mod_ir, "generate_id", lambda: "GID")

    qr0 = QuantumRegister(1, "q0")
    qr1 = QuantumRegister(2, "q1")
    cr0 = ClassicalRegister(2, "c0")
    qc = QuantumCircuit(qr0, qr1, cr0)

    qc.x(qr1[1])
    qc.measure(qr1[0], cr0[1])

    ir = mod_ir.to_ir(qc)

    assert ir["id"] == "QuantumCircuit_GID"
    assert ir["num_qubits"] == 3
    assert ir["num_clbits"] == 2

    assert ir["quantum_registers"] == {"q0": [0], "q1": [1, 2]}
    assert ir["classical_registers"] == {"c0": [0, 1]}

    # First instruction: x on q1[1] => global 2
    assert ir["instructions"][0]["name"] == "x"
    assert ir["instructions"][0]["qubits"] == [2]

    # Second instruction: measure q1[0] -> c0[1] => qubit 1, clbit 1
    assert ir["instructions"][1] == {"name": "measure", "qubits": [1], "clbits": [1]}


def test_to_ir_quantumcircuit_barrier_is_ignored(monkeypatch):
    monkeypatch.setattr(mod_ir, "generate_id", lambda: "GID")

    qc = QuantumCircuit(1, 1)
    qc.barrier(0)

    ir = mod_ir.to_ir(qc)
    assert ir["instructions"] == []


def test_to_ir_quantumcircuit_unsupported_operation_raises(monkeypatch):
    monkeypatch.setattr(mod_ir, "generate_id", lambda: "GID")

    qc = QuantumCircuit(1, 0)
    qc.delay(10, 0)

    with pytest.raises(ValueError) as excinfo:
        mod_ir.to_ir(qc)



def test_to_ir_quantumcircuit_unitary_encodes_complex_matrix(monkeypatch):
    monkeypatch.setattr(mod_ir, "generate_id", lambda: "GID")

    qc = QuantumCircuit(1, 0)
    U = np.array([[1 + 0j, 0 + 0j], [0 + 0j, 1 + 0j]], dtype=complex)
    qc.unitary(U, [0])

    ir = mod_ir.to_ir(qc)

    instr = ir["instructions"][0]
    assert instr["name"] == "unitary"
    assert instr["qubits"] == [0]

    encoded = instr["params"][0]
    assert encoded == [
        [[1.0, 0.0], [0.0, 0.0]],
        [[0.0, 0.0], [1.0, 0.0]],
    ]


def _apply_c_if(qc):
    # Best-effort helper for different Qiskit versions.
    # Returns True if a conditional instruction was added successfully.
    try:
        qc.x(0).c_if(0, 1)
        return True
    except Exception:
        pass


def test_to_ir_quantumcircuit_classically_conditioned_gate_sets_dynamic(monkeypatch):
    monkeypatch.setattr(mod_ir, "generate_id", lambda: "GID")

    qc = QuantumCircuit(1, 2)

    if not _apply_c_if(qc):
        pytest.skip("This Qiskit version does not support .c_if() in a compatible way.")

    ir = mod_ir.to_ir(qc)

    assert ir["is_dynamic"] is True
    assert len(ir["instructions"]) == 1
    assert ir["instructions"][0]["name"] == "cif"
    assert ir["instructions"][0]["instructions"][0]["name"] == "x"
    assert ir["instructions"][0]["instructions"][0]["qubits"] == [0]


def _build_if_else(qc):
    # Best-effort builder for an 'if_else' instruction.
    # Returns True if an if_else op was created; otherwise False.
    try:
        # Newer Qiskit: if_test context manager generates an if_else/if op.
        with qc.if_test((qc.clbits[0], 1)):
            qc.x(0)
        return True
    except Exception:
        pass

    try:
        true_body = QuantumCircuit(1, 1)
        true_body.x(0)
        qc.if_else((qc.clbits[0], 1), true_body, None, [0], [0])
        return True
    except Exception:
        return False


def test_to_ir_quantumcircuit_if_else_without_else_is_supported(monkeypatch):
    monkeypatch.setattr(mod_ir, "generate_id", lambda: "GID")

    qc = QuantumCircuit(1, 1)

    if not _build_if_else(qc):
        pytest.skip("Could not build an if_else instruction with this Qiskit version.")

    ir = mod_ir.to_ir(qc)

    # Your code should mark it as dynamic and inline the subcircuit instructions.
    assert ir["is_dynamic"] is True
    assert ir["instructions"][0]["instructions"][0]["name"] == "x"
