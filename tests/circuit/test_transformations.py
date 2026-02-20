#circuit/test_partitioning.py
import os, sys

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

import copy
import pytest

import cunqa.circuit.transformations as part_mod

class FakeQuantumControlContext:
    def __init__(self, control_circuit, target_circuit) -> int:
        """Class constructor.
        
            Args:
                control_circuit (~cunqa.circuit.CunqaCircuit): circuit which qubit is exposed.
            
                target_circuit (~cunqa.circuit.CunqaCircuit): circuit in which the instructions are 
                implemented.
        """
        self.control_circuit = control_circuit
        self.target_circuit = target_circuit

    def __enter__(self):
        self._subcircuit = FakeCircuit(self.target_circuit.num_qubits, self.target_circuit.num_clbits)
        return -1, self._subcircuit

    def __exit__(self, exc_type, exc_val, exc_tb):
        rcontrol = {
            "name": "rcontrol",
            "instructions": self._subcircuit.instructions,
            "circuits": [self.control_circuit.info['id']]
        }
        self.target_circuit.add_instructions(rcontrol)

        return False

class FakeCircuit:
    """Small test double for CunqaCircuit with only what's needed by hsplit/union/add."""

    def __init__(self, num_qubits: int, num_clbits: int = 0, id: str = "C"):
        self.num_qubits = num_qubits
        self.num_clbits = num_clbits
        self.id = id
        self.instructions = []
        self.is_dynamic = False
        self._expose_calls = []

    @property
    def info(self):
        return {"id": self.id}

    def add_instructions(self, instrs):
        if isinstance(instrs, dict):
            instrs = [instrs]
        self.instructions.extend(instrs)

    def add_cl_register(self, name, num_clbits):
        self.num_clbits += num_clbits

    def expose(self, ctrl_qubit: int, target_circuit: "FakeCircuit"):
        self._expose_calls.append((ctrl_qubit, target_circuit.id))
        self.add_instructions({
            "name": "expose",
            "qubits": [ctrl_qubit],
            "circuits": [target_circuit.id]
        })

        return FakeQuantumControlContext(self, target_circuit)


@pytest.fixture(autouse=True)
def _patch_dependencies(monkeypatch):
    # Patch the module-level CunqaCircuit reference and REMOTE_GATES and logger.
    monkeypatch.setattr(part_mod, "CunqaCircuit", FakeCircuit)
    monkeypatch.setattr(part_mod, "REMOTE_GATES", {"send", "recv", "qsend", 
                                                   "qrecv", "expose", "rcontrol"})


# -------------------------
# hsplit tests
# -------------------------

def test_hsplit_list_wrong_sum():
    c = FakeCircuit(num_qubits=3, id="A")
    with pytest.raises(RuntimeError):
        part_mod.hsplit(c, [1, 1])  # sum != 3


def test_hsplit_int_sections_must_be_positive():
    c = FakeCircuit(num_qubits=2, id="A")
    with pytest.raises(ValueError):
        part_mod.hsplit(c, 0)
    with pytest.raises(ValueError):
        part_mod.hsplit(c, -2)


def test_hsplit_partitions_one_and_two_qubit_instructions_across_sections():
    # Circuit with 2 qubits split into 2 sections: [0] and [1].
    c = FakeCircuit(num_qubits=2, id="A")

    # One-qubit gate on q0, one-qubit gate on q1, and a 2-qubit gate across sections.
    c.add_instructions({"name": "x", "qubits": [0]})
    c.add_instructions({"name": "h", "qubits": [1]})
    c.add_instructions({"name": "cx", "qubits": [0, 1]})

    original = copy.deepcopy(c.instructions)

    subs = part_mod.hsplit(c, [1, 1])
    assert len(subs) == 2
    assert subs[0].id == "A_0"
    assert subs[1].id == "A_1"

    # Single-qubit gates get reindexed locally.
    assert subs[0].instructions[0] == {"name": "x", "qubits": [0]}
    assert subs[1].instructions[0] == {"name": "h", "qubits": [0]}

    # Cross-section 2-qubit gate should be moved to the target circuit,
    # and the control qubit replaced by the "rcontrol" placeholder (-1).
    assert subs[0].instructions[1] == {"name": "expose", "qubits": [0], "circuits": ["A_1"]}
    assert subs[1].instructions[1] == {
            "name": "rcontrol",
            "instructions": [{"name": "cx", "qubits": [-1, 0]}],
            "circuits": ["A_0"]
        }

    # Ensure original circuit was not mutated (hsplit deep-copies internally).
    assert c.instructions == original


def test_hsplit_raises_on_three_qubit_gate():
    c = FakeCircuit(num_qubits=3, id="A")
    c.add_instructions({"name": "ccx", "qubits": [0, 1, 2]})
    with pytest.raises(ValueError):
        part_mod.hsplit(c, [1, 2])


# -------------------------
# union tests
# -------------------------

def test_union_empty_list_raises():
    with pytest.raises(ValueError):
        part_mod.union([])

def test_union_reindexes_qubits_and_clbits():
    c1 = FakeCircuit(num_qubits=1, num_clbits=1, id="A")
    c2 = FakeCircuit(num_qubits=2, num_clbits=1, id="B")

    c1.add_instructions({"name": "measure", "qubits": [0], "clbits": [0]})
    c2.add_instructions({"name": "x", "qubits": [1]})
    c2.add_instructions({"name": "measure", "qubits": [0], "clbits": [0]})

    out = part_mod.union([c1, c2])

    assert out.num_qubits == 3
    assert out.num_clbits == 2
    assert out.id == "A|B"

    # Offsets: c2 qubits +1, c2 clbits +1
    assert out.instructions == [
        {"name": "measure", "qubits": [0], "clbits": [0]},
        {"name": "x", "qubits": [2]},                # was qubit 1 in c2 -> 1+1=2
        {"name": "measure", "qubits": [1], "clbits": [1]},  # was (0,0) in c2 -> (1,1)
    ]


def test_union_send_recv():
    cA = FakeCircuit(num_qubits=1, num_clbits=1, id="A")
    cB = FakeCircuit(num_qubits=1, num_clbits=1, id="B")

    # recv blocks on circuit B; send consumes and produces a copy instruction.
    cA.add_instructions({"name": "send", "clbits": [0], "circuits": ["B"]})
    cB.add_instructions({"name": "recv", "clbits": [0], "circuits": ["A"]})

    out = part_mod.union([cA, cB])

    assert out.is_dynamic is True
    # cB clbit offset is 1 (A has 1 clbit), so blocked recv clbits become [1]
    assert out.instructions == [
        {"name": "copy", "l_clbits": [1], "r_clbits": [0]},
    ]


def test_union_qsend_qrecv():
    cA = FakeCircuit(num_qubits=1, num_clbits=0, id="A")
    cB = FakeCircuit(num_qubits=1, num_clbits=0, id="B")

    # qrecv blocks on circuit B; qsend consumes and produces swap + reset.
    cA.add_instructions({"name": "qsend", "qubits": [0], "circuits": ["B"]})
    cB.add_instructions({"name": "qrecv", "qubits": [0], "circuits": ["A"]})

    out = part_mod.union([cA, cB])

    assert out.is_dynamic is True
    # cB qubit offset is 1 (A has 1 qubit), so blocked qrecv qubit becomes [1]
    assert out.instructions == [
        {"name": "swap", "qubits": [0, 1]},
        {"name": "reset", "qubits": [0]},
    ]


def test_union_expose_rcontrol():
    cA = FakeCircuit(num_qubits=1, num_clbits=0, id="A")
    cB = FakeCircuit(num_qubits=1, num_clbits=0, id="B")

    # expose blocks on circuit A; rcontrol consumes and inlines its internal instructions.
    cA.add_instructions({"name": "expose", "qubits": [0], "circuits": ["B"]})
    cB.add_instructions(
        {
            "name": "rcontrol",
            "circuits": ["A"],
            "instructions": [{"name": "x", "qubits": [0]},
                             {"name": "cx", "qubits": [-1, 0]}],
        }
    )

    out = part_mod.union([cA, cB])

    assert out.is_dynamic is True
    assert out.instructions == [{"name": "x", "qubits": [1]},
                                {"name": "cx", "qubits": [0, 1]}]


def test_union_remote_with_unknown_target_is_treated_as_local_instruction():
    cA = FakeCircuit(num_qubits=1, num_clbits=1, id="A")
    cB = FakeCircuit(num_qubits=1, num_clbits=1, id="B")

    # "C" is not in circuit_ids -> is_valid_remote returns False -> appended as local.
    cA.add_instructions({"name": "send", "clbits": [0], "circuits": ["C"]})

    out = part_mod.union([cA, cB])
    assert out.instructions == [{"name": "send", "clbits": [0], "circuits": ["C"]}]


# -------------------------
# add tests
# -------------------------

def test_add_empty_list_raises():
    with pytest.raises(ValueError):
        part_mod.add([])

def test_add_raises_if_circuits_communicate_with_each_other():
    cA = FakeCircuit(num_qubits=1, num_clbits=1, id="A")
    cB = FakeCircuit(num_qubits=1, num_clbits=1, id="B")

    cA.add_instructions({"name": "send", "clbits": [0], "circuits": ["B"]})

    with pytest.raises(ValueError):
        part_mod.add([cA, cB])


def test_add_concatenates_instructions_and_sets_shape_and_id():
    cA = FakeCircuit(num_qubits=1, num_clbits=1, id="A")
    cB = FakeCircuit(num_qubits=2, num_clbits=3, id="B")

    cA.add_instructions({"name": "x", "qubits": [0]})
    cB.add_instructions({"name": "measure", "qubits": [1], "clbits": [2]})

    out = part_mod.add([cA, cB])

    assert out.num_qubits == 2  # max
    assert out.num_clbits == 3  # max
    assert out.id == "A+B"
    assert out.instructions == [
        {"name": "x", "qubits": [0]},
        {"name": "measure", "qubits": [1], "clbits": [2]},
    ]
