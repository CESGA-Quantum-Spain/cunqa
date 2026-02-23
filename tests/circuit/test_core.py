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
from unittest.mock import Mock

# Adjust this import to your real module path.
from cunqa.circuit.core import CunqaCircuit, QuantumControlContext

import cunqa.circuit.core as circuit_mod

@pytest.fixture(autouse=True)
def _reset_class_state():
    # Avoid cross-test pollution from class-level state.
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}
    yield
    CunqaCircuit._ids = set()
    CunqaCircuit._communicated = {}


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


def test_add_instructions_accepts_dict_or_list(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "INS")

    circuit = CunqaCircuit(1)
    circuit.add_instructions({"name": "x", "qubits": [0]})
    circuit.add_instructions([{"name": "h", "qubits": [0]}, {"name": "z", "qubits": [0]}])

    assert [i["name"] for i in circuit.instructions] == ["x", "h", "z"]

def test_add_instruction_with_symbolic_params_creates_param(monkeypatch):
    circuit = CunqaCircuit(1)

    fake_expr = Mock()
    fake_expr.is_real = False
    monkeypatch.setattr("cunqa.circuit.core.sympify", lambda _: [fake_expr])

    ParamMock = Mock()
    monkeypatch.setattr("cunqa.circuit.core.Param", ParamMock)

    instr = {"name": "rx", "params": ["theta"]}
    circuit.add_instructions(instr)

    ParamMock.assert_called_once_with(fake_expr)
    assert len(circuit.params) == 1
    assert instr["params"][0] == ParamMock.return_value

def test_add_instruction_with_real_param_does_not_create_param(monkeypatch):
    circuit = CunqaCircuit(1)

    fake_expr = Mock()
    fake_expr.is_real = True
    monkeypatch.setattr("cunqa.circuit.core.sympify", lambda _: [fake_expr])

    ParamMock = Mock()
    monkeypatch.setattr("cunqa.circuit.core.Param", ParamMock)

    instr = {"name": "rx", "params": [3.14]}
    circuit.add_instructions(instr)

    ParamMock.assert_not_called()
    assert instr["params"][0] == 3.14

def test_add_instruction_sympify_error_raises_valueerror(monkeypatch):
    from cunqa.circuit import CunqaCircuit
    from sympy import SympifyError

    circuit = CunqaCircuit(1)

    def raise_error(x):
        raise SympifyError("bad")

    monkeypatch.setattr("cunqa.circuit.core.sympify", raise_error)

    instr = {"name": "rx", "params": ["bad_expr"]}

    with pytest.raises(ValueError):
        circuit.add_instructions(instr)

def test_add_instruction_mixed_params(monkeypatch):
    from cunqa.circuit import CunqaCircuit

    circuit = CunqaCircuit(1)

    expr_symbolic = Mock()
    expr_symbolic.is_real = False
    real_number = Mock()
    real_number.is_real = True
    monkeypatch.setattr("cunqa.circuit.core.sympify", lambda x: [expr_symbolic, real_number])

    ParamMock = Mock()
    monkeypatch.setattr("cunqa.circuit.core.Param", ParamMock)

    instr = {"name": "u", "params": ["theta", 3.14]}
    circuit.add_instructions(instr)

    ParamMock.assert_called_once_with(expr_symbolic)
    assert len(circuit.params) == 1
    assert instr["params"][0] == ParamMock.return_value
    assert instr["params"][1] == 3.14


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
    ("P0",      (0,), {"name": "p0",    "qubits": [0]}),
    ("P1",      (0,), {"name": "p1",    "qubits": [0]}),
    ("reset",   (0,), {"name": "reset", "qubits": [0]})
]
@pytest.mark.parametrize("method, args, expected", ONEQUBIT_NOPARAM)
def test_onequbit_noparam_gates(method, args, expected):
    circuit = CunqaCircuit(1)
    getattr(circuit, method)(*args)

    assert circuit.instructions[-1] == expected

TWOQUBIT_NOPARAM = [
    ("swap", (0,1,), {"name": "swap", "qubits": [0,1]}),
    ("ecr",  (0,1,), {"name": "ecr",  "qubits": [0,1]}),
    ("cx",   (0,1,), {"name": "cx",   "qubits": [0,1]}),
    ("cy",   (0,1,), {"name": "cy",   "qubits": [0,1]}),
    ("cz",   (0,1,), {"name": "cz",   "qubits": [0,1]}),
    ("csx",  (0,1,), {"name": "csx",  "qubits": [0,1]})
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
    ("RotInvX", (0.1,0,),         {"name": "rotinvx", "qubits": [0], "params": [0.1]}),
    ("RotInvY", (0.1,0,),         {"name": "rotinvy", "qubits": [0], "params": [0.1]}),
    ("RotInvZ", (0.1,0,),         {"name": "rotinvz", "qubits": [0], "params": [0.1]}),
]
@pytest.mark.parametrize("method, args, expected", ONEQUBIT_PARAM)
def test_onequbit_param_gates(method, args, expected):
    circuit = CunqaCircuit(1)
    getattr(circuit, method)(*args)

    assert circuit.instructions[-1] == expected

TWOQUBIT_PARAM = [
    ("rxx", (0.1,0,1,),             {"name": "rxx", "qubits": [0,1], "params": [0.1]}),
    ("ryy", (0.1,0,1,),             {"name": "ryy", "qubits": [0,1], "params": [0.1]}),
    ("rzz", (0.1,0,1,),             {"name": "rzz", "qubits": [0,1], "params": [0.1]}),
    ("rzx", (0.1,0,1,),             {"name": "rzx", "qubits": [0,1], "params": [0.1]}),
    ("cr",  (0.1,0,1,),             {"name": "cr",  "qubits": [0,1], "params": [0.1]}),
    ("crx", (0.1,0,1,),             {"name": "crx", "qubits": [0,1], "params": [0.1]}),
    ("cry", (0.1,0,1,),             {"name": "cry", "qubits": [0,1], "params": [0.1]}),
    ("crz", (0.1,0,1,),             {"name": "crz", "qubits": [0,1], "params": [0.1]}),
    ("cp",  (0.1,0,1,),             {"name": "cp",  "qubits": [0,1], "params": [0.1]}),
    ("cu",  (0.1,0.2,0.3,0.4,0,1,), {"name": "cu",  "qubits": [0,1], "params": [0.1,0.2,0.3,0.4]}),
]
@pytest.mark.parametrize("method, args, expected", TWOQUBIT_PARAM)
def test_twoqubit_param_gates(method, args, expected):
    circuit = CunqaCircuit(2)
    getattr(circuit, method)(*args)

    assert circuit.instructions[-1] == expected


def test_unitary_accepts_numpy(monkeypatch):
    monkeypatch.setattr(circuit_mod, "generate_id", lambda: "UNIT")

    circuit = CunqaCircuit(1)

    mat = np.array([[1+0j, 0+0j],
                    [0+0j, 1+0j]], dtype=complex)
    circuit.unitary(mat, 0)

    instr = circuit.instructions[-1]
    assert instr["name"] == "unitary"
    assert instr["qubits"] == [0]

    encoded = instr["elements"][0]
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
