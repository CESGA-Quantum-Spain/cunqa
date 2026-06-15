# tests/qc_protocols/test_telegate.py
"""
Tests for :py:mod:`cunqa.qc_protocols.telegate`.

Template covering the telegate protocol helpers
:py:func:`~cunqa.qc_protocols.telegate.cat_entangler` and
:py:func:`~cunqa.qc_protocols.telegate.cat_disentangler`. The helpers append the protocol
instructions onto the participating :py:class:`~cunqa.circuit.core.CunqaCircuit` objects, the first
of which owns the shared control qubit.
"""
import os, sys
import pytest

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

from cunqa.circuit.core import CunqaCircuit
from cunqa.qc_protocols.telegate import cat_entangler, cat_disentangler


@pytest.fixture(autouse=True)
def _reset_class_state():
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}
    yield
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}


@pytest.fixture
def pair():
    control = CunqaCircuit((1, 1), num_clbits=1, id="A")
    target = CunqaCircuit((1, 1), num_clbits=1, id="B")
    return control, target


# ------------------------
# cat_entangler
# ------------------------

def test_cat_entangler_distributes_control(pair):
    control, target = pair

    cat_entangler([control, target], data_qubit=0, comm_qubits=[1, 1], clbits=[0, 0], tag="tg")

    # Control circuit shares its state and sends the correction bit.
    assert [i["name"] for i in control.instructions] == [
        "gen_ent", "cx", "measure", "cif", "x", "endcif", "send",
    ]
    # Receiving circuit entangles and applies the entangler correction.
    assert [i["name"] for i in target.instructions] == [
        "gen_ent", "recv", "cif", "x", "endcif",
    ]


def test_cat_entangler_uses_shared_tag(pair):
    control, target = pair

    cat_entangler([control, target], 0, [1, 1], [0, 0], tag="shared")

    for circuit in (control, target):
        gen_ent = circuit.instructions[0]
        assert gen_ent["name"] == "gen_ent"
        assert gen_ent["tag"] == "shared"


# ------------------------
# cat_disentangler
# ------------------------

def test_cat_disentangler_closes_the_block(pair):
    control, target = pair

    cat_entangler([control, target], 0, [1, 1], [0, 0], tag="tg")
    cat_disentangler([control, target], data_qubit=0, comm_qubits=[1],
                     recv_clbits=[0], send_clbits=[0])

    # The control circuit receives the corrections and applies the conditioned phase fix.
    assert [i["name"] for i in control.instructions] == [
        "gen_ent", "cx", "measure", "cif", "x", "endcif", "send",
        "recv", "cif", "z", "endcif",
    ]
    # The receiving circuit measures out its comm qubit and sends the correction back.
    assert [i["name"] for i in target.instructions] == [
        "gen_ent", "recv", "cif", "x", "endcif",
        "h", "measure", "cif", "x", "endcif", "send",
    ]


def test_cat_disentangler_phase_correction_targets_control_data_qubit(pair):
    control, target = pair

    cat_entangler([control, target], 0, [1, 1], [0, 0], tag="tg")
    cat_disentangler([control, target], 0, [1], [0], [0])

    z_instr = next(i for i in control.instructions if i["name"] == "z")
    assert z_instr["qubits"] == 0
