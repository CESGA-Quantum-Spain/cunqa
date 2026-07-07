#circuit/test_transformations.py
import os, sys

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

import pytest
from unittest.mock import Mock

import cunqa.circuit.transformations as part_mod
from cunqa.circuit.core import CunqaCircuit
from cunqa.qc_protocols import qsend, qrecv


@pytest.fixture(autouse=True)
def _reset_class_state():
    # Avoid cross-test pollution from CunqaCircuit class-level state.
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}
    yield
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}


# -------------------------
# hsplit tests
# -------------------------

def test_hsplit_list_wrong_sum():
    c = CunqaCircuit(3, id="A")
    with pytest.raises(RuntimeError):
        part_mod.hsplit(c, [1, 1])  # sum != 3


def test_hsplit_int_sections_must_be_positive():
    c = CunqaCircuit(2, id="A")
    with pytest.raises(ValueError):
        part_mod.hsplit(c, 0)
    with pytest.raises(ValueError):
        part_mod.hsplit(c, -2)


def test_hsplit_partitions_local_instructions_across_sections():
    # Circuit with 2 qubits split into two single-qubit sections.
    c = CunqaCircuit(2, id="A")
    c.x(0)
    c.h(1)

    subs = part_mod.hsplit(c, [1, 1])

    assert len(subs) == 2
    assert subs[0].id == "A_0"
    assert subs[1].id == "A_1"

    # Single-qubit gates get reindexed locally (qubit 1 of c -> qubit 0 of A_1).
    assert subs[0].instructions == [{"name": "x", "qubits": 0}]
    assert subs[1].instructions == [{"name": "h", "qubits": 0}]


def test_hsplit_int_sections_splits_evenly():
    c = CunqaCircuit(4, id="A")
    for q in range(4):
        c.h(q)

    subs = part_mod.hsplit(c, 2)

    assert len(subs) == 2
    assert [s.id for s in subs] == ["A_0", "A_1"]
    assert all(s.num_qubits[0] == 2 for s in subs)


def test_hsplit_cross_section_gate_creates_telegate_protocol():
    # A two-qubit gate that crosses two sections is turned into a telegate protocol
    # (cat_entangler / remote gate / cat_disentangler) using comm qubits.
    c = CunqaCircuit(2, id="A")
    c.x(0)
    c.cx(0, 1)

    subs = part_mod.hsplit(c, [1, 1])

    # Both subcircuits gain a comm qubit and a gen_ent directive tagged as a telegate.
    for sub in subs:
        assert sub.num_qubits[1] == 1
        gen_ents = [i for i in sub.instructions if i["name"] == "gen_ent"]
        assert len(gen_ents) == 1
        assert gen_ents[0]["tag"].startswith("telegate")
        assert set(gen_ents[0]["circuits"]) == {"A_0", "A_1"}


def test_hsplit_raises_on_three_qubit_gate():
    c = CunqaCircuit(3, id="A")
    c.ccx(0, 1, 2)
    with pytest.raises(ValueError):
        part_mod.hsplit(c, [1, 2])


# -------------------------
# union tests
# -------------------------

def test_union_empty_list_raises():
    with pytest.raises(ValueError):
        part_mod.union([])


def test_union_single_circuit_returns_it(monkeypatch):
    logger_mock = Mock()
    monkeypatch.setattr(part_mod, "logger", logger_mock)

    c = CunqaCircuit(1, id="solo")
    out = part_mod.union([c])

    assert out is c
    logger_mock.warning.assert_called_once()


def test_union_reindexes_qubits_and_clbits():
    c1 = CunqaCircuit(1, num_clbits=1, id="A")
    c2 = CunqaCircuit(2, num_clbits=1, id="B")

    c1.measure(0, 0)
    c2.x(1)
    c2.measure(0, 0)

    out = part_mod.union([c1, c2])

    assert out.num_qubits == [3, 0]
    assert out.num_clbits == 2
    assert out.id == "A|B"

    # Offsets: c2 qubits +1, c2 clbits +1
    assert out.instructions == [
        {"name": "measure", "qubits": 0, "clbits": 0, "save": True},
        {"name": "x", "qubits": 2},                              # was qubit 1 in c2 -> 1+1=2
        {"name": "measure", "qubits": 1, "clbits": 1, "save": True},  # was (0,0) in c2 -> (1,1)
    ]


def test_union_collapses_teledata_into_swap():
    # A teledata (qsend/qrecv) protocol between two circuits is collapsed by union; the
    # teleportation is fully replaced by a swap between the sender and receiver data qubits.
    cA = CunqaCircuit((1, 1), num_clbits=2, id="A")
    cB = CunqaCircuit((1, 1), num_clbits=2, id="B")

    qsend(cA, 0, 1, [0, 1], cB, tag="td")
    qrecv(cB, 0, 1, [0, 1], cA, tag="td")

    out = part_mod.union([cA, cB])

    # cA data qubit 0 -> global 0, cB data qubit 0 -> global 2 (cA contributes 2 qubits).
    assert out.instructions == [{"name": "swap", "qubits": [0, 2]}]


def test_union_collapses_telegate_round_trip():
    # hsplit turns a cross-section two-qubit gate into a telegate protocol; union must
    # collapse that protocol back into the equivalent direct gate between the data qubits.
    original = CunqaCircuit(2, id="A")
    original.x(0)
    original.cx(0, 1)

    out = part_mod.union(part_mod.hsplit(original, [1, 1]))

    # The protocol is fully consumed: no communication directives or cif blocks remain.
    for instr in out.instructions:
        assert instr["name"] not in {"gen_ent", "send", "recv", "cif", "endcif", "measure"}

    # The local x stays, and the cx is reissued on the two data qubits (0 and 2).
    assert out.instructions == [
        {"name": "x", "qubits": 0},
        {"name": "cx", "qubits": [0, 2]},
    ]
    assert out.is_dynamic is False


def test_union_collapses_chained_telegates_across_sections():
    # Several cross-section gates produce independent telegate protocols (each with its own tag);
    # union must collapse all of them back into direct gates on the data qubits.
    original = CunqaCircuit(3, id="A")
    original.h(0)
    original.cx(0, 1)
    original.cx(1, 2)
    original.x(2)

    out = part_mod.union(part_mod.hsplit(original, [1, 1, 1]))

    for instr in out.instructions:
        assert instr["name"] not in {"gen_ent", "send", "recv", "cif", "endcif", "measure"}

    # Data qubits land at 0, 2 and 5 (each section also reserves comm qubits).
    assert out.instructions == [
        {"name": "h", "qubits": 0},
        {"name": "cx", "qubits": [0, 2]},
        {"name": "cx", "qubits": [2, 5]},
        {"name": "x", "qubits": 5},
    ]


# -------------------------
# add tests
# -------------------------

def test_add_empty_list_raises():
    with pytest.raises(ValueError):
        part_mod.add([])


def test_add_raises_if_circuits_communicate_with_each_other():
    cA = CunqaCircuit(1, num_clbits=1, id="A")
    cB = CunqaCircuit(1, num_clbits=1, id="B")

    cA.measure(0, 0)
    cA.send(0, "B")
    cB.recv(0, "A")

    with pytest.raises(ValueError):
        part_mod.add([cA, cB])


def test_add_concatenates_instructions_and_sets_shape_and_id():
    cA = CunqaCircuit(1, num_clbits=1, id="A")
    cB = CunqaCircuit(2, num_clbits=3, id="B")

    cA.x(0)
    cB.measure(1, 2)

    out = part_mod.add([cA, cB])

    assert out.num_qubits == [2, 0]  # max number of data qubits
    assert out.num_clbits == 3       # max number of clbits
    assert out.id == "A+B"
    assert out.instructions == [
        {"name": "x", "qubits": 0},
        {"name": "measure", "qubits": 1, "clbits": 2, "save": True},
    ]
