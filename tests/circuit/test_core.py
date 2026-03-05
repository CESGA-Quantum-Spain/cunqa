#circuit/test_core.py
import os, sys

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

import pytest
import numpy as np
import sympy
from unittest.mock import Mock, patch
from cunqa.circuit.parameter import Param

@pytest.fixture(autouse=True)
def _reset_class_state():
    # Avoid cross-test pollution from class-level state.
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}
    yield
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}

# Do add_instructions tests before importing CunqaCircuit globally to avoid Mock-isinstance problems
def test_add_single_instruction_no_params():
    """Test adding a single instruction with no parameters"""
    circuit = CunqaCircuit(1)
    instr = {"name": "x"}
    
    circuit.add_instructions(instr)
    
    assert len(circuit.instructions) == 1
    assert circuit.instructions[0] == instr
    assert len(circuit.params) == 0


def test_add_single_instruction_with_param_object():
    """Test adding instruction where params are already Param objects"""
    circuit = CunqaCircuit(1)
    param = Param(sympy.Symbol('theta'))
    instr = {"name": "u", "params": [param]}
    
    circuit.add_instructions(instr)
    
    assert len(circuit.instructions) == 1
    assert len(circuit.params) == 1
    # Should be a deep copy, not the same object
    assert circuit.params[0] is not param
    assert str(circuit.params[0].expr) == str(param.expr)


def test_add_single_instruction_with_symbolic_param():
    """Test adding instruction with symbolic string parameter"""
    circuit = CunqaCircuit(1)
    instr = {"name": "u", "params": ["theta"]}
    
    circuit.add_instructions(instr)
    
    assert len(circuit.instructions) == 1
    assert len(circuit.params) == 1
    assert isinstance(instr["params"][0], Param)
    assert str(instr["params"][0].expr) == "theta"


def test_add_single_instruction_with_numeric_param():
    """Test adding instruction with numeric parameter"""
    circuit = CunqaCircuit(1)
    instr = {"name": "u", "params": [3.14]}
    
    circuit.add_instructions(instr)
    
    assert len(circuit.instructions) == 1
    assert len(circuit.params) == 0
    assert instr["params"][0] == 3.14


def test_add_instruction_mixed_symbolic_and_numeric():
    """Test adding instruction with mixed symbolic and numeric parameters"""
    circuit = CunqaCircuit(1)
    instr = {"name": "u", "params": ["theta", 3.14, "phi"]}
    
    circuit.add_instructions(instr)
    
    assert len(circuit.instructions) == 1
    assert len(circuit.params) == 2
    assert isinstance(instr["params"][0], Param)
    assert instr["params"][1] == 3.14
    assert isinstance(instr["params"][2], Param)


def test_add_instruction_empty_params():
    """Test adding instruction with empty params list"""
    circuit = CunqaCircuit(1)
    instr = {"name": "x", "params": []}
    
    circuit.add_instructions(instr)
    
    assert len(circuit.instructions) == 1
    assert len(circuit.params) == 0


def test_add_multiple_instructions():
    """Test adding multiple instructions at once"""
    circuit = CunqaCircuit(1)
    instrs = [
        {"name": "x"},
        {"name": "u", "params": ["theta"]},
        {"name": "y", "params": [1.57]}
    ]
    
    circuit.add_instructions(instrs)
    
    assert len(circuit.instructions) == 3
    assert len(circuit.params) == 1
    assert isinstance(instrs[1]["params"][0], Param)


def test_add_instruction_invalid_expression():
    """Test that invalid symbolic expressions raise ValueError"""
    circuit = CunqaCircuit(1)
    instr = {"name": "u", "params": ["not a valid expression!!!"]}
    
    with pytest.raises(ValueError, match="cannot be converted to symbolic expression"):
        circuit.add_instructions(instr)


def test_add_instruction_with_expression_object():
    """Test adding instruction with sympy expression"""
    circuit = CunqaCircuit(1)
    theta = sympy.Symbol('theta')
    expr = theta + 1
    instr = {"name": "u", "params": [expr]}
    
    circuit.add_instructions(instr)
    
    assert len(circuit.instructions) == 1
    assert len(circuit.params) == 1
    assert isinstance(instr["params"][0], Param)


def test_add_instruction_modifies_original_dict():
    """Test that add_instructions modifies the original instruction dict"""
    circuit = CunqaCircuit(1)
    instr = {"name": "u", "params": ["theta", 3.14]}
    original_params = instr["params"].copy()
    
    circuit.add_instructions(instr)
    
    # The original instruction dict should be modified
    assert instr["params"] != original_params
    assert isinstance(instr["params"][0], Param)
    assert instr["params"][1] == 3.14


def test_add_multiple_instructions_preserves_order():
    """Test that multiple instructions are added in order"""
    circuit = CunqaCircuit(1)
    instrs = [
        {"name": "x"},
        {"name": "y"},
        {"name": "z"}
    ]
    
    circuit.add_instructions(instrs)
    
    assert [i["name"] for i in circuit.instructions] == ["x", "y", "z"]


def test_add_instruction_param_deep_copy():
    """Test that existing Param objects are deep copied"""
    circuit = CunqaCircuit(1)
    param = Param(sympy.Symbol('theta'))
    instr = {"name": "u", "params": [param]}
    
    circuit.add_instructions(instr)
    
    # The param in circuit.params should be a different object
    assert circuit.params[0] is not param
    # But should have the same expression
    assert str(circuit.params[0].expr) == str(param.expr)


def test_add_instruction_complex_expression():
    """Test adding instruction with complex symbolic expression"""
    circuit = CunqaCircuit(1)
    instr = {"name": "u", "params": ["theta + pi/2"]}
    
    circuit.add_instructions(instr)
    
    assert len(circuit.params) == 1
    assert isinstance(instr["params"][0], Param)

from cunqa.circuit.core import CunqaCircuit, QuantumControlContext
import cunqa.circuit.core as circuit_mod

def test_init_generates_id_and_adds_default_q_register(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "ABC")

    circuit = CunqaCircuit(2)

    assert circuit.id == "CunqaCircuit_ABC"
    assert circuit.num_qubits == 2
    assert circuit.quantum_regs["q0"] == [0, 1]


def test_init_with_num_clbits_adds_default_classical_register(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "ID")

    circuit = CunqaCircuit(2, num_clbits=3)

    assert circuit.num_clbits == 3
    assert circuit.classical_regs["c0"] == [0, 1, 2]


def test_init_duplicate_id(monkeypatch):
    logger_mock = Mock()
    monkeypatch.setattr(circuit_mod, "logger", logger_mock)
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "XYZ")

    CunqaCircuit._ids.add("dup")
    circuit = CunqaCircuit(1, id="dup")

    assert circuit.id == "CunqaCircuit_XYZ"
    logger_mock.warning.assert_called_once()


def test_info_property(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "INFO")
    
    circuit = CunqaCircuit(2, num_clbits=1)

    info = circuit.info
    assert info["id"] == circuit.id
    assert info["instructions"] == circuit.instructions
    assert info["num_qubits"] == 2
    assert info["num_clbits"] == 1
    assert info["quantum_registers"] == circuit.quantum_regs
    assert info["classical_registers"] == circuit.classical_regs
    assert info["is_dynamic"] == circuit.is_dynamic
    assert info["sending_to"] == list(circuit.sending_to)
    assert info["params"] == circuit.params


def test_add_q_register_num_qubits_not_positive(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "QREG")

    circuit = CunqaCircuit(1)
    with pytest.raises(ValueError):
        circuit.add_q_register("qX", 0)


def test_add_q_register_name_in_use(monkeypatch):
    logger_mock = Mock()
    monkeypatch.setattr(circuit_mod, "logger", logger_mock)
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "QREG2")

    circuit = CunqaCircuit(1)  # creates "q0"
    new_name = circuit.add_q_register("q0", 1)

    assert new_name == "q0_0"
    assert circuit.num_qubits == 2
    assert "q0_0" in circuit.quantum_regs
    logger_mock.warning.assert_called_once()


def test_add_cl_register_num_clbits_not_positive(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "CREG")

    circuit = CunqaCircuit(1)
    with pytest.raises(ValueError):
        circuit.add_cl_register("cX", 0)


def test_add_cl_register_name_in_use(monkeypatch):
    logger_mock = Mock()
    monkeypatch.setattr(circuit_mod, "logger", logger_mock)
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "CREG2")

    circuit = CunqaCircuit(1, num_clbits=1)  # creates "c0"
    new_name = circuit.add_cl_register("c0", 2)

    assert new_name == "c0_0"
    assert circuit.num_clbits == 3
    assert "c0_0" in circuit.classical_regs
    logger_mock.warning.assert_called_once()

ONEQUBIT_NOPARAM = [
    ("i",       (0,), {"name": "id",    "qubits": [0]}),
    ("x",       (0,), {"name": "x",     "qubits": [0]}),
    ("y",       (0,), {"name": "y",     "qubits": [0]}),
    ("z",       (0,), {"name": "z",     "qubits": [0]}),
    ("h",       (0,), {"name": "h",     "qubits": [0]}),
    ("s",       (0,), {"name": "s",     "qubits": [0]}),
    ("sdg",     (0,), {"name": "sdg",   "qubits": [0]}),
    ("sx",      (0,), {"name": "sx",    "qubits": [0]}),
    ("sxdg",    (0,), {"name": "sxdg",  "qubits": [0]}),
    ("sy",      (0,), {"name": "sy",    "qubits": [0]}),
    ("sydg",    (0,), {"name": "sydg",  "qubits": [0]}),
    ("sz",      (0,), {"name": "sz",    "qubits": [0]}),
    ("szdg",    (0,), {"name": "szdg",  "qubits": [0]}),
    ("t",       (0,), {"name": "t",     "qubits": [0]}),
    ("tdg",     (0,), {"name": "tdg",   "qubits": [0]}),
    ("p0",      (0,), {"name": "p0",    "qubits": [0]}),
    ("p1",      (0,), {"name": "p1",    "qubits": [0]}),
    ("v",       (0,), {"name": "v",     "qubits": [0]}),
    ("vdg",     (0,), {"name": "vdg",   "qubits": [0]}),
    ("k",       (0,), {"name": "k",     "qubits": [0]}),
    ("reset",   (0,), {"name": "reset", "qubits": [0]})
]
@pytest.mark.parametrize("method, args, expected", ONEQUBIT_NOPARAM)
def test_onequbit_noparam_gates(method, args, expected):
    circuit = CunqaCircuit(1)
    getattr(circuit, method)(*args)

    assert circuit.instructions[-1] == expected

TWOQUBIT_NOPARAM = [
    ("swap",      (0,1,), {"name": "swap",      "qubits": [0,1]}),
    ("iswap",     (0,1,), {"name": "iswap",     "qubits": [0,1]}),
    ("fusedswap", (0,1,), {"name": "fusedswap", "qubits": [0,1]}),
    ("ecr",       (0,1,), {"name": "ecr",       "qubits": [0,1]}),
    ("cx",        (0,1,), {"name": "cx",        "qubits": [0,1]}),
    ("cy",        (0,1,), {"name": "cy",        "qubits": [0,1]}),
    ("cz",        (0,1,), {"name": "cz",        "qubits": [0,1]}),
    ("ch",        (0,1,), {"name": "ch",        "qubits": [0,1]}),
    ("csx",       (0,1,), {"name": "csx",       "qubits": [0,1]}),
    ("csxdg",     (0,1,), {"name": "csxdg",     "qubits": [0,1]}),
    ("cs",        (0,1,), {"name": "cs",        "qubits": [0,1]}),
    ("csdg",      (0,1,), {"name": "csdg",      "qubits": [0,1]}),
    ("ct",        (0,1,), {"name": "ct",        "qubits": [0,1]}),
    ("dcx",       (0,1,), {"name": "dcx",       "qubits": [0,1]}),
]
@pytest.mark.parametrize("method, args, expected", TWOQUBIT_NOPARAM)
def test_twoqubit_noparam_gates(method, args, expected):
    circuit = CunqaCircuit(2)
    getattr(circuit, method)(*args)

    assert circuit.instructions[-1] == expected

THREEQUBIT_NOPARAM = [
    ("ccx",   (0,1,2), {"name": "ccx",   "qubits": [0,1,2]}),
    ("ccz",   (0,1,2), {"name": "ccz",   "qubits": [0,1,2]}),
    ("cecr",  (0,1,2), {"name": "cecr",  "qubits": [0,1,2]}),
    ("cswap", (0,1,2), {"name": "cswap", "qubits": [0,1,2]}),
]
@pytest.mark.parametrize("method, args, expected", THREEQUBIT_NOPARAM)
def test_threequbit_noparam_gates(method, args, expected):
    circuit = CunqaCircuit(3)
    getattr(circuit, method)(*args)

    assert circuit.instructions[-1] == expected

# this gate is added already decomposed
def test_ccy():
    circuit = CunqaCircuit(3)
    circuit.ccy(0,1,2)
    
    assert circuit.instructions[-1] == {"name":"rz",   "qubits":[2], "params":[np.pi/2]}
    assert circuit.instructions[-2] == {"name": "ccx", "qubits": [0,1,2]}
    assert circuit.instructions[-3] == {"name":"rz",   "qubits":[2], "params":[-np.pi/2]}

ONEQUBIT_PARAM = [
    ("u1",      (0.1,0,),         {"name": "u1",      "qubits": [0], "params": [0.1]}),
    ("u2",      (0.1,0.2,0,),     {"name": "u2",      "qubits": [0], "params": [0.1,0.2]}),
    ("u3",      (0.1,0.2,0.3,0,), {"name": "u3",      "qubits": [0], "params": [0.1,0.2,0.3]}),
    ("u",       (0.1,0.2,0.3,0,), {"name": "u",       "qubits": [0], "params": [0.1,0.2,0.3]}),
    ("p",       (0.1,0,),         {"name": "p",       "qubits": [0], "params": [0.1]}),
    ("r",       (0.1,0.2,0,),     {"name": "r",       "qubits": [0], "params": [0.1,0.2]}),
    ("rx",      (0.1,0,),         {"name": "rx",      "qubits": [0], "params": [0.1]}),
    ("ry",      (0.1,0,),         {"name": "ry",      "qubits": [0], "params": [0.1]}),
    ("rz",      (0.1,0,),         {"name": "rz",      "qubits": [0], "params": [0.1]}), 
    ("rotinvx", (0.1,0,),         {"name": "rotinvx", "qubits": [0], "params": [0.1]}),
    ("rotinvy", (0.1,0,),         {"name": "rotinvy", "qubits": [0], "params": [0.1]}),
    ("rotinvz", (0.1,0,),         {"name": "rotinvz", "qubits": [0], "params": [0.1]}),
]
@pytest.mark.parametrize("method, args, expected", ONEQUBIT_PARAM)
def test_onequbit_param_gates(method, args, expected):
    circuit = CunqaCircuit(1)
    getattr(circuit, method)(*args)

    assert circuit.instructions[-1] == expected

TWOQUBIT_PARAM = [
    ("rxx",   (0.1,0,1,),             {"name": "rxx",   "qubits": [0,1], "params": [0.1]}),
    ("ryy",   (0.1,0,1,),             {"name": "ryy",   "qubits": [0,1], "params": [0.1]}),
    ("rzz",   (0.1,0,1,),             {"name": "rzz",   "qubits": [0,1], "params": [0.1]}),
    ("rzx",   (0.1,0,1,),             {"name": "rzx",   "qubits": [0,1], "params": [0.1]}),
    ("cr",    (0.1,0,1,),             {"name": "cr",    "qubits": [0,1], "params": [0.1]}),
    ("crx",   (0.1,0,1,),             {"name": "crx",   "qubits": [0,1], "params": [0.1]}),
    ("cry",   (0.1,0,1,),             {"name": "cry",   "qubits": [0,1], "params": [0.1]}),
    ("crz",   (0.1,0,1,),             {"name": "crz",   "qubits": [0,1], "params": [0.1]}),
    ("cp",    (0.1,0,1,),             {"name": "cp",    "qubits": [0,1], "params": [0.1]}),
    ("cu1",   (0.1,0,1,),             {"name": "cu1",   "qubits": [0,1], "params": [0.1]}),
    ("cu2",   (0.1, 0.2,0,1,),        {"name": "cu2",   "qubits": [0,1], "params": [0.1, 0.2]}),
    ("cu3",   (0.1,0.2,0.3,0,1,),     {"name": "cu3",   "qubits": [0,1], "params": [0.1,0.2,0.3]}),
    ("cu",    (0.1,0.2,0.3,0.4,0,1,), {"name": "cu",    "qubits": [0,1], "params": [0.1,0.2,0.3,0.4]}),
    ("cu",    (0.1,0.2,0.3,0.4,0,1,), {"name": "cu",    "qubits": [0,1], "params": [0.1,0.2,0.3,0.4]}),
    ("xxmyy", (0.1,0.2,0,1,),         {"name": "xxmyy", "qubits": [0,1], "params": [0.1,0.2]}),
    ("xxpyy", (0.1,0.2,0,1,),         {"name": "xxpyy", "qubits": [0,1], "params": [0.1,0.2]}),
]
@pytest.mark.parametrize("method, args, expected", TWOQUBIT_PARAM)
def test_twoqubit_param_gates(method, args, expected):
    circuit = CunqaCircuit(2)
    getattr(circuit, method)(*args)

    assert circuit.instructions[-1] == expected

SPECIAL_GATES = [
    ("randomunitary",              (0,1,),                   {"name": "randomunitary",             "qubits": [0,1]}),
    ("diagonal",                   ([1.0+1.0j,0.0-1.0j],0,), {"name": "diagonal",                  "qubits": [0],  "matrix":[[1.0,1.0],[0.0,-1.0]]}),
    ("multipauli",                 ([1,2,3],0,),             {"name": "multipauli",                "qubits": [0],  "pauli_id_list":[1,2,3]}),
    ("multipaulirotation",         (1.0,[1,2,3],0,),         {"name": "multipaulirotation",        "qubits": [0],  "params":[1.0], "pauli_id_list":[1,2,3]}),
    ("amplitudedampingnoise",      (1.0,0,1,),               {"name": "amplitudedampingnoise",     "qubits": [0,1],"params":[1.0]}),
    ("bitflipnoise",               (1.0,0,),                 {"name": "bitflipnoise",              "qubits": [0],  "params":[1.0]}),
    ("dephasingnoise",             (1.0,0,),                 {"name": "bitflipnoise",              "qubits": [0],  "params":[1.0]}),
    ("depolarizingnoise",          (1.0,0,),                 {"name": "bitflipnoise",              "qubits": [0],  "params":[1.0]}),
    ("independentxznoise",         (1.0,0,),                 {"name": "bitflipnoise",              "qubits": [0],  "params":[1.0]}),
    ("twoqubitdepolarizingnoise",  (1.0,0,1,),               {"name": "twoqubitdepolarizingnoise", "qubits": [0,1],"params":[1.0]}),

]

def test_unitary_accepts_numpy(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "UNIT")

    circuit = CunqaCircuit(1)

    mat = np.array([[1+0j, 0+0j],
                    [0+0j, 1+0j]], dtype=complex)
    circuit.unitary(mat, 0)

    instr = circuit.instructions[-1]
    assert instr["name"] == "unitary"
    assert instr["qubits"] == [0]

    encoded = instr["matrix"][0]
    assert encoded == [
        [[1.0, 0.0], [0.0, 0.0]],
        [[0.0, 0.0], [1.0, 0.0]],
    ]


def test_unitary_invalid_matrix(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "BADUNIT")

    circuit = CunqaCircuit(1)
    with pytest.raises(ValueError):
        circuit.unitary([[1, 0, 0]], 0)


def test_measure(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "MEAS")

    circuit = CunqaCircuit(2, num_clbits=2)
    circuit.measure(0, 0)
    circuit.measure([1], [1])

    assert circuit.instructions[-2] == {"name": "measure", "qubits": [0], "clbits": [0]}
    assert circuit.instructions[-1] == {"name": "measure", "qubits": [1], "clbits": [1]}

def test_measure_all(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "MEASALL")

    circuit = CunqaCircuit(2)
    circuit.measure_all()

    assert circuit.num_clbits == 2
    assert any(k.startswith("measure") for k in circuit.classical_regs.keys())

    measure_instrs = [i for i in circuit.instructions if i["name"] == "measure"]
    assert len(measure_instrs) == 2


def test_cif_context_adds_cif_instruction(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "CIF")

    circuit = CunqaCircuit(1, num_clbits=1)

    with circuit.cif(0) as sub:
        sub.x(0)
        sub.h(0)

    assert circuit.is_dynamic is True
    cif_instr = circuit.instructions[-1]
    assert cif_instr["name"] == "cif"
    assert cif_instr["clbits"] == [0]
    assert [i["name"] for i in cif_instr["instructions"]] == ["x", "h"]


def test_cif_context_rejects_remote_ops(monkeypatch):
    
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "CIFBAD")

    circuit = CunqaCircuit(1, num_clbits=1)

    with pytest.raises(RuntimeError):
        with circuit.cif(0) as sub:
            sub.qsend(0, "B")

B_CIRCUIT = [
    (lambda: CunqaCircuit(1, id="B"), "B"),
    (lambda: "B", "B"),
]

@pytest.mark.parametrize("target_factory, expected_id", B_CIRCUIT)
def test_send(target_factory, expected_id):
    c1 = CunqaCircuit(2, num_clbits=2, id="A")
    target = target_factory()

    c1.send(0, target)
    c1.send([0, 1], target)

    assert c1.is_dynamic is True
    assert c1.instructions[-1] == {"name": "send", "clbits": [0, 1], "circuits": [expected_id]}
    assert c1.instructions[-2] == {"name": "send", "clbits": [0], "circuits": [expected_id]}
    assert c1.sending_to == {expected_id}

@pytest.mark.parametrize("target_factory, expected_id", B_CIRCUIT)
def test_recv(target_factory, expected_id):
    c1 = CunqaCircuit(1, num_clbits=2, id="A")
    target = target_factory()

    c1.recv(0, target)
    c1.recv([0, 1], target)

    assert c1.is_dynamic is True
    assert c1.instructions[-1] == {"name": "recv", "clbits": [0, 1], "circuits": [expected_id]}
    assert c1.instructions[-2] == {"name": "recv", "clbits": [0], "circuits": [expected_id]}

@pytest.mark.parametrize("target_factory, expected_id", B_CIRCUIT)
def test_qsend(target_factory, expected_id):
    c1 = CunqaCircuit(1, num_clbits=2, id="A")
    target = target_factory()

    c1.qsend(0, target)

    assert c1.is_dynamic is True
    assert c1.instructions[-1] == {"name": "qsend", "qubits": [0], "circuits": [expected_id]}

@pytest.mark.parametrize("target_factory, expected_id", B_CIRCUIT)
def test_qrecv(target_factory, expected_id):
    c1 = CunqaCircuit(1, num_clbits=2, id="A")
    target = target_factory()

    c1.qrecv(0, target)

    assert c1.is_dynamic is True
    assert c1.instructions[-1] == {"name": "qrecv", "qubits": [0], "circuits": [expected_id]}

@pytest.mark.parametrize("target_factory, expected_id", B_CIRCUIT)
def test_expose(target_factory, expected_id):
    c1 = CunqaCircuit(1, id="A")
    target = target_factory()

    ctx = c1.expose(0, target)

    assert c1.is_dynamic is True
    assert c1.instructions[-1] == {"name": "expose", "qubits": [0], "circuits": [expected_id]}
    assert isinstance(ctx, QuantumControlContext)


def test_quantum_control_context_adds_rcontrol_to_target():
    control = CunqaCircuit(1, id="CTRL")
    target = CunqaCircuit(1, id="TGT")

    with control.expose(0, target) as (rqubit, subcircuit):
        assert rqubit == -1
        subcircuit.x(0)

    rcontrol_instr = target.instructions[-1]
    assert rcontrol_instr["name"] == "rcontrol"
    assert rcontrol_instr["circuits"] == ["CTRL"]
    assert [i["name"] for i in rcontrol_instr["instructions"]] == ["x"]


def test_quantum_control_context_rejects_remote_ops_inside_block():
    control = CunqaCircuit(1, id="CTRL")
    target = CunqaCircuit(1, id="TGT")

    with pytest.raises(RuntimeError):
        with control.expose(0, target) as (rqubit, subcircuit):
            subcircuit.recv(0, "OTHER")  # forbidden by __exit__
