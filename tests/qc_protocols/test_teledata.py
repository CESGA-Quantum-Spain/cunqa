# tests/qc_protocols/test_teledata.py
"""
Tests for :py:mod:`cunqa.qc_protocols.teledata`.

Template covering the teleportation protocol helpers :py:func:`~cunqa.qc_protocols.teledata.qsend`
and :py:func:`~cunqa.qc_protocols.teledata.qrecv`. These functions append the protocol
instructions onto real :py:class:`~cunqa.circuit.core.CunqaCircuit` objects (which must declare a
comm qubit via the ``(num_data, num_comm)`` form), so the tests assert on the emitted instruction
stream.
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
from cunqa.qc_protocols.teledata import qsend, qrecv


@pytest.fixture(autouse=True)
def _reset_class_state():
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}
    yield
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}


# ------------------------
# qsend
# ------------------------

def test_qsend_emits_full_sender_protocol():
    circuit = CunqaCircuit((1, 1), num_clbits=2, id="A")

    qsend(circuit, data_qubit=0, comm_qubit=1, clbits=[0, 1], recving_circuit="B", tag="td")

    names = [i["name"] for i in circuit.instructions]
    # gen_ent, Bell measurement (cx + h + two measures), the two conditional resets and the send.
    assert names == [
        "gen_ent", "cx", "h", "measure", "measure",
        "cif", "x", "endcif", "cif", "x", "endcif", "send",
    ]
    assert circuit.is_dynamic is True


def test_qsend_registers_receiver_and_tag():
    circuit = CunqaCircuit((1, 1), num_clbits=2, id="A")

    qsend(circuit, 0, 1, [0, 1], "B", tag="my_tag")

    gen_ent = circuit.instructions[0]
    assert gen_ent["name"] == "gen_ent"
    assert gen_ent["tag"] == "my_tag"
    assert set(gen_ent["circuits"]) == {"A", "B"}
    assert "B" in circuit.sending_to


def test_qsend_accepts_circuit_object_as_target():
    sender = CunqaCircuit((1, 1), num_clbits=2, id="A")
    receiver = CunqaCircuit((1, 1), num_clbits=2, id="B")

    qsend(sender, 0, 1, [0, 1], receiver, tag="td")

    assert "B" in sender.sending_to


# ------------------------
# qrecv
# ------------------------

def test_qrecv_emits_full_receiver_protocol():
    circuit = CunqaCircuit((1, 1), num_clbits=2, id="B")

    qrecv(circuit, data_qubit=0, comm_qubit=1, clbits=[0, 1], control_circuit="A", tag="td")

    names = [i["name"] for i in circuit.instructions]
    # gen_ent, recv, the two conditional corrections and the final swap.
    assert names == [
        "gen_ent", "recv", "cif", "x", "endcif", "cif", "z", "endcif", "swap",
    ]
    assert circuit.is_dynamic is True


def test_qrecv_swaps_comm_into_data_qubit():
    circuit = CunqaCircuit((1, 1), num_clbits=2, id="B")

    qrecv(circuit, data_qubit=0, comm_qubit=1, clbits=[0, 1], control_circuit="A", tag="td")

    swap = circuit.instructions[-1]
    assert swap["name"] == "swap"
    assert swap["qubits"] == [1, 0]
