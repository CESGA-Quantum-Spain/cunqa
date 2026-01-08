import os, sys
from unittest.mock import Mock, patch, mock_open
import pytest

HOME = os.getenv("HOME")
sys.path.insert(0, HOME)

import cunqa.qpu as qpu_mod
from cunqa.qpu import QPU


# ------------------------
# QPU.__init__ tests
# ------------------------

def test_init_connects_when_endpoint_is_ok():
    backend, endpoint = Mock(name="Backend"), "http://good-endpoint"
    with patch.object(qpu_mod, "QClient") as QClientMock:
        qclient_instance = Mock()
        QClientMock.return_value = qclient_instance
        qpu = QPU(id=1, backend=backend, family="f", endpoint=endpoint)
    QClientMock.assert_called_once_with()
    qclient_instance.connect.assert_called_once_with(endpoint)
    assert qpu._id == 1
    assert qpu._backend is backend
    assert qpu._family == "f"
    assert qpu._qclient is qclient_instance

def test_init_raises_when_endpoint_is_bad():
    backend, endpoint = Mock(name="Backend"), "http://bad-endpoint"
    with patch.object(qpu_mod, "QClient") as QClientMock:
        qclient_instance = Mock()
        qclient_instance.connect.side_effect = ConnectionError("cannot connect")
        QClientMock.return_value = qclient_instance
        with pytest.raises(ConnectionError):
            QPU(id=1, backend=backend, family="f", endpoint=endpoint)
    QClientMock.assert_called_once_with()
    qclient_instance.connect.assert_called_once_with(endpoint)


# ------------------------
# QPU.execute tests
# ------------------------

@pytest.fixture
def qpu(monkeypatch):
    backend, endpoint = Mock(name="Backend"), "http://any-endpoint"

    QClientMock = Mock(name="QClient")
    qclient_instance = Mock(name="QClientInstance")
    QClientMock.return_value = qclient_instance
    monkeypatch.setattr(qpu_mod, "QJob", QClientMock)

    return QPU(id=1, backend=backend, family="f", endpoint=endpoint)

def test_execute_creates_qjob_submits_and_returns(monkeypatch, qpu):
    circuit, run_parameters = {"some": "circuit"}, {}

    QJobMock = Mock(name="QJob")
    qjob_instance = Mock(name="QJobInstance")
    QJobMock.return_value = qjob_instance
    monkeypatch.setattr(qpu_mod, "QJob", QJobMock)

    result = qpu.execute(circuit, **run_parameters)
    
    QJobMock.assert_called_once_with(qpu._qclient, qpu._backend, circuit, **run_parameters)
    qjob_instance.submit.assert_called_once_with()
    assert result is qjob_instance

def test_execute_reraises_if_qjob_creation_fails(monkeypatch, qpu):
    circuit = {"some": "circuit"}

    QJobMock = Mock(name="QJob")
    QJobMock.side_effect = RuntimeError("boom")

    monkeypatch.setattr(qpu_mod, "QJob", QJobMock)
    with pytest.raises(RuntimeError, match="boom"):
        qpu.execute(circuit)
    
    QJobMock.assert_called_once_with(qpu._qclient, qpu._backend, circuit)

def test_execute_reraises_if_qjob_submit_fails(monkeypatch, qpu):
    circuit = {"some": "circuit"}

    QJobMock = Mock(name="QJob")
    qjob_instance = Mock(name="QJobInstance")
    qjob_instance.submit.side_effect = RuntimeError("Failed to submit the job")
    QJobMock.return_value = qjob_instance
    monkeypatch.setattr(qpu_mod, "QJob", QJobMock)

    with pytest.raises(RuntimeError, match="Failed to submit the job"):
        qpu.execute(circuit)
    
    QJobMock.assert_called_once()
    qjob_instance.submit.assert_called_once_with()

# ------------------------
# run tests
# ------------------------
from cunqa.qpu import run

def test_run_with_list_converts_to_ir_and_executes_on_each_qpu(monkeypatch):
    circuits = ["c1", "c2"]
    c1_ir = {"id": "c1"}
    c2_ir = {"id": "c2"}

    qpu1, qpu2 = Mock(name="QPU1"), Mock(name="QPU2")
    qpu1._id, qpu2._id = 1, 2
    job1, job2 = Mock(name="Job1"), Mock(name="Job2")
    qpu1.execute.return_value, qpu2.execute.return_value = job1, job2

    def _to_ir_side_effect(circuit):
        if circuit == "c1":
            return c1_ir
        elif circuit == "c2":
            return c2_ir

    to_ir_mock = Mock(side_effect=_to_ir_side_effect)
    monkeypatch.setattr(qpu_mod, "to_ir", to_ir_mock)
    
    result = run(circuits, [qpu1, qpu2], shots=100, method="sv")

    assert result == [job1, job2]
    assert to_ir_mock.call_args_list[0].args[0] == "c1"
    assert to_ir_mock.call_args_list[1].args[0] == "c2"
    qpu1.execute.assert_called_once_with(c1_ir, shots=100, method="sv")
    qpu2.execute.assert_called_once_with(c2_ir, shots=100, method="sv")

def test_run_with_single_circuit_returns_single_qjob(monkeypatch):
    circuit = "c1"
    circuit_ir = {"id": "c1"}

    qpu, job = Mock(name="QPU"), Mock(name="Job")
    qpu._id = 1
    qpu.execute.return_value = job

    to_ir_mock = Mock(return_value=circuit_ir)
    monkeypatch.setattr(qpu_mod, "to_ir", to_ir_mock)
    
    result = run(circuit, [qpu], shots=50)

    to_ir_mock.assert_called_once_with(circuit)
    qpu.execute.assert_called_once_with(circuit_ir, shots=50)
    assert result is job

def test_run_raises_if_not_enough_qpus(monkeypatch):
    circuits = ["c1", "c2"]
    c1_ir = {"id": "c1"}
    c2_ir = {"id": "c2"}

    qpu = Mock(name="QPU")
    qpu._id = 1

    def _to_ir_side_effect(circuit):
        if circuit == "c1":
            return c1_ir
        elif circuit == "c2":
            return c2_ir

    to_ir_mock = Mock(side_effect=_to_ir_side_effect)
    monkeypatch.setattr(qpu_mod, "to_ir", to_ir_mock)

    with pytest.raises(ValueError, match="There are not enough QPUs"):
        run(circuits, [qpu])

    qpu.execute.assert_not_called()

def test_run_warns_if_extra_qpus_and_ignores_them(monkeypatch):
    circuits = ["c1"]
    circuit_ir = {"id": "c1"}

    qpu1, qpu2 = Mock(name="QPU1"), Mock(name="QPU2")
    qpu1._id, qpu2._id = 1, 2
    job1 = Mock(name="Job1")
    qpu1.execute.return_value = job1

    to_ir_mock = Mock(return_value=circuit_ir)
    monkeypatch.setattr(qpu_mod, "to_ir", to_ir_mock)

    logger_mock = Mock()
    monkeypatch.setattr(qpu_mod, "logger", logger_mock)

    result = run(circuits, [qpu1, qpu2])

    logger_mock.warning.assert_called_once()
    assert "More QPUs provided than the number of circuits" in logger_mock.warning.call_args[0][0]
    qpu1.execute.assert_called_once_with(circuit_ir)
    qpu2.execute.assert_not_called()
    assert result is job1

def test_run_updates_remote_instructions_sending_to_and_ids(monkeypatch):
    circuits = ["c1"]
    circuit_ir = {
        "id": "c1",
        "instructions": [
            {"name": "REMOTE_GATE", "circuits": ["c1"]},
            {"name": "LOCAL_GATE"},
        ],
        "sending_to": ["c1"]
    }

    qpu = Mock(name="QPU")
    qpu._id = 10
    job = Mock(name="Job")
    qpu.execute.return_value = job

    to_ir_mock = Mock(return_value=circuit_ir)
    monkeypatch.setattr(qpu_mod, "to_ir", to_ir_mock)
    monkeypatch.setattr(qpu_mod, "REMOTE_GATES", ["REMOTE_GATE"])

    result = run(circuits, [qpu])

    remote_instr = circuit_ir["instructions"][0]
    assert remote_instr["qpus"] == [10]
    assert "circuits" not in remote_instr

    assert circuit_ir["sending_to"] == [10]
    assert circuit_ir["id"] == 10

    qpu.execute.assert_called_once_with(circuit_ir)
    assert result is job

def test_run_does_not_touch_instructions_without_remote_gates_but_remaps_ids(monkeypatch):
    circuits = ["c1"]
    original_instr = {"name": "LOCAL_GATE", "qubits": []}
    circuit_ir = {
        "id": "c1",
        "instructions": [original_instr],
        "sending_to": ["c1"],
    }

    qpu = Mock(name="QPU")
    qpu._id = 7
    job = Mock(name="Job")
    qpu.execute.return_value = job

    to_ir_mock = Mock(return_value=circuit_ir)
    monkeypatch.setattr(qpu_mod, "to_ir", to_ir_mock)
    monkeypatch.setattr(qpu_mod, "REMOTE_GATES", ["REMOTE_GATE"])

    run(circuits, [qpu])

    assert circuit_ir["instructions"][0] is original_instr
    assert "qpus" not in original_instr

    assert circuit_ir["sending_to"] == [7]
    assert circuit_ir["id"] == 7


# ------------------------
# qraise tests
# ------------------------
from cunqa.qpu import qraise

def _subprocess_run_side_effect_ok(job_id="12345"):
    """
    Returns a side_effect for subprocess.run:
    - First call: qraise command (shell=True) → returns stdout with a job_id
    - Following calls: squeue commands → always return RUNNING
    """
    def _side_effect(*args, **kwargs):
        if kwargs.get("shell", False):  # qraise call
            proc = Mock()
            proc.stdout = f"{job_id};my_cluster\n" #this is what "sbatch --parsable" returns
            return proc
        # squeue calls
        proc = Mock()
        proc.stdout = "RUNNING\n"
        return proc
    return _side_effect


# --- Command building + happy path ---

def test_qraise_builds_command_and_returns_job_id_without_family(monkeypatch):
    n, t = 1, "00:10:00"

    monkeypatch.setattr(qpu_mod.os.path, "exists", lambda _: True)
    monkeypatch.setattr("builtins.open", mock_open())

    load_mock = Mock(return_value={"12345-0": {}})
    monkeypatch.setattr(qpu_mod.json, "load", load_mock)

    run_mock = Mock()
    run_mock.side_effect = _subprocess_run_side_effect_ok("12345")
    monkeypatch.setattr(qpu_mod.subprocess, "run", run_mock)

    result = qraise(
        n, t,
        classical_comm=True,
        quantum_comm=False,
        co_located=False,   # ensures that --co-located is NOT added
    )

    (cmd_str,), cmd_kwargs = run_mock.call_args_list[0]
    assert cmd_kwargs["shell"] is True
    assert f"qraise -n {n} -t {t}" in cmd_str
    assert "--classical_comm" in cmd_str
    assert "--quantum_comm" not in cmd_str
    assert "--co-located" not in cmd_str

    (squeue_cmd,), squeue_kwargs = run_mock.call_args_list[1]
    assert squeue_cmd[:3] == ["squeue", "-h", "-j"]
    assert squeue_cmd[3] == "12345"

    assert result == "12345"


def test_qraise_builds_full_command_with_all_options_and_family_tuple_return(monkeypatch):
    n, t = 2, "01:00:00"
    family = "my_family"

    monkeypatch.setattr(qpu_mod.os.path, "exists", lambda _: True)
    monkeypatch.setattr("builtins.open", mock_open())

    load_mock = Mock(return_value={"54321-0": {}, "54321-1": {}})
    monkeypatch.setattr(qpu_mod.json, "load", load_mock)

    run_mock = Mock()
    run_mock.side_effect = _subprocess_run_side_effect_ok("54321")
    monkeypatch.setattr(qpu_mod.subprocess, "run", run_mock)

    result = qraise(
        n, t,
        classical_comm=True,
        quantum_comm=True,
        simulator="aer_sim",
        backend="/path/to/backend.json",
        fakeqmio=True,
        family=family,
        co_located=True,
        cores=8,
        mem_per_qpu=16,
        n_nodes=3,
        node_list="node01,node02",
        qpus_per_node=2,
        partition="partition1"
    )

    (cmd_str,), _ = run_mock.call_args_list[0]

    assert "--fakeqmio" in cmd_str
    assert "--classical_comm" in cmd_str
    assert "--quantum_comm" in cmd_str
    assert "--simulator=aer_sim" in cmd_str
    assert "--backend=/path/to/backend.json" in cmd_str
    assert "--family_name=my_family" in cmd_str
    assert "--co-located" in cmd_str
    assert "--cores=8" in cmd_str
    assert "--mem-per-qpu=16G" in cmd_str
    assert "--n_nodes=3" in cmd_str
    assert "--node_list=node01,node02" in cmd_str
    assert "--qpus_per_node=2" in cmd_str
    assert cmd_str == (f"qraise -n {2} -t {t} --fakeqmio --classical_comm --quantum_comm "
                       f"--simulator=aer_sim --family_name={family} --co-located --cores=8 "
                       f"--mem-per-qpu=16G --n_nodes=3 --node_list=node01,node02 --qpus_per_node=2 "
                       f"--backend=/path/to/backend.json --partition=partition1")
    assert result == family


# --- QPUS_FILEPATH creation ---

def test_qraise_creates_qpus_file_if_not_exists(monkeypatch):
    n, t = 1, "00:05:00"

    monkeypatch.setattr(qpu_mod.os.path, "exists", lambda _: False)

    m_open = mock_open()
    monkeypatch.setattr("builtins.open", m_open)

    load_mock = Mock(return_value={"99999-0": {}})
    monkeypatch.setattr(qpu_mod.json, "load", load_mock)

    run_mock = Mock()
    run_mock.side_effect = _subprocess_run_side_effect_ok("99999")
    monkeypatch.setattr(qpu_mod.subprocess, "run", run_mock)

    result = qraise(n, t)

    # Ensure file initialisation "{}" was written
    m_open.assert_any_call(qpu_mod.QPUS_FILEPATH, "w")
    handle = m_open()
    handle.write.assert_called_once_with("{}")

    assert result == "99999"


# --- JSON decode retry logic ---

def test_qraise_retries_on_jsondecodeerror_until_valid_json(monkeypatch):
    n, t = 1, "00:05:00"

    monkeypatch.setattr(qpu_mod.os.path, "exists", lambda _: True)
    monkeypatch.setattr("builtins.open", mock_open())

    run_mock = Mock()
    run_mock.side_effect = _subprocess_run_side_effect_ok("77777")
    monkeypatch.setattr(qpu_mod.subprocess, "run", run_mock)

    # mock de json.load con primer intento que lanza JSONDecodeError y segundo que va bien
    decode_error = qpu_mod.json.JSONDecodeError("bad json", "{}", 0)
    load_mock = Mock(side_effect=[decode_error, {"77777-0": {}}])
    monkeypatch.setattr(qpu_mod.json, "load", load_mock)
    result = qraise(n, t)

    assert load_mock.call_count >= 2
    assert result == "77777"


# --- subprocess error handling ---

def test_qraise_raises_runtimeerror_on_subprocess_error(monkeypatch):
    n, t = 1, "00:05:00"

    error = qpu_mod.subprocess.CalledProcessError(
        returncode=1,
        cmd="qraise",
        stderr="boom",
    )

    monkeypatch.setattr(qpu_mod.os.path, "exists", lambda _: True)
    monkeypatch.setattr("builtins.open", mock_open())

    run_mock = Mock(side_effect=error)
    monkeypatch.setattr(qpu_mod.subprocess, "run", run_mock)

    with pytest.raises(RuntimeError) as excinfo:
        qraise(n, t)

    msg = str(excinfo.value)
    assert "An error was encoutered while qraising" in msg
    assert "boom" in msg


# ------------------------
# qdrop tests
# ------------------------
from cunqa.qpu import qdrop

def test_qdrop_no_families(monkeypatch):
    """If no families are passed, qdrop should call: ['qdrop', '--all']."""
    called = {}

    def fake_run(cmd, *args, **kwargs):
        called["cmd"] = cmd

    monkeypatch.setattr(qpu_mod.subprocess, "run", fake_run)
    qdrop()

    assert called["cmd"] == ["qdrop", "--all"]


def test_qdrop_single_family(monkeypatch):
    """If one family is passed, qdrop should call: ['qdrop', '--fam', family]."""
    called = {}

    def fake_run(cmd, *args, **kwargs):
        called["cmd"] = cmd

    monkeypatch.setattr(qpu_mod.subprocess, "run", fake_run)
    qdrop("famA")

    assert called["cmd"] == ["qdrop", "--fam", "famA"]


def test_qdrop_multiple_families(monkeypatch):
    """
    If multiple families are passed, qdrop should call:
    ['qdrop', '--fam', fam1, fam2, ...] preserving order.
    """
    called = {}

    def fake_run(cmd, *args, **kwargs):
        called["cmd"] = cmd

    monkeypatch.setattr(qpu_mod.subprocess, "run", fake_run)
    qdrop("famA", "famB", "famC")

    assert called["cmd"] == ["qdrop", "--fam", "famA", "famB", "famC"]

# ------------------------
# get_QPUs tests
# ------------------------
from cunqa.qpu import get_QPUs

def _mock_qpus_json(monkeypatch, qpus_dict: dict):
    """
    Make get_QPUs read QPU info from `qpus_dict` instead of a real file.
    We:
      - mock `open` in the module where get_QPUs is defined
      - mock `json.load` in that same module to just return our dict
    """
    monkeypatch.setattr("builtins.open", mock_open())
    monkeypatch.setattr(qpu_mod.json, "load", lambda f: qpus_dict)


@pytest.fixture
def qpu_backend_mocks(monkeypatch):
    """
    Replace real QPU and Backend with mocks so we don't depend on
    their implementation, only on how get_QPUs uses them.
    """
    mock_backend_cls = Mock(name="Backend")
    mock_qpu_cls = Mock(name="QPU")

    monkeypatch.setattr(qpu_mod, "Backend", mock_backend_cls)
    monkeypatch.setattr(qpu_mod, "QPU", mock_qpu_cls)

    return mock_backend_cls, mock_qpu_cls

def test_qpu_file_empty(monkeypatch):
    _mock_qpus_json(monkeypatch, {})

    result = get_QPUs()

    assert result is None

def test_login_node_hpc(monkeypatch):
    _mock_qpus_json(
        monkeypatch,
        {
            "qpu-1": {
                "backend": "backend-A",
                "family": "fam-A",
                "net": {
                    "nodename": "node-1", 
                    "endpoint": "tcp://node-1:1234", 
            }
        },
    })
    monkeypatch.delenv("SLURMD_NODENAME", raising=False)

    result = get_QPUs()

    assert result is None

def test_login_node_co_located(
    monkeypatch, qpu_backend_mocks
):
    backend_mock, qpu_mock = qpu_backend_mocks
    _mock_qpus_json(
        monkeypatch,
        {
            "qpu-1": {
                "backend": "backend-A",
                "family": "fam-A",
                "net": {
                    "nodename": "node-1", 
                    "endpoint": "tcp://node-1:1234", 
                    "mode": "co_located"
                },
            }
        },
    )
    monkeypatch.delenv("SLURMD_NODENAME", raising=False)

    qpus = get_QPUs(co_located=True)

    assert qpus is not None
    assert len(qpus) == 1
    assert qpus[0] is qpu_mock.return_value
    backend_mock.assert_called_once_with("backend-A")
    qpu_mock.assert_called_once_with(
        id="qpu-1",
        backend=backend_mock.return_value,
        family="fam-A",
        endpoint="tcp://node-1:1234",
    )


def test_hpc_without_family_filter(
    monkeypatch, qpu_backend_mocks
):
    backend_mock, qpu_mock = qpu_backend_mocks

    _mock_qpus_json(
        monkeypatch,
        {
            "qpu-1": {
                "backend": "backend-A",
                "family": "fam-A",
                "net": {"nodename": "node-1", "endpoint": "tcp://node-1:1234"},
            },
            "qpu-2": {
                "backend": "backend-B",
                "family": "fam-B",
                "net": {"nodename": "node-2", "endpoint": "tcp://node-2:5678"},
            },
        },
    )
    monkeypatch.setenv("SLURMD_NODENAME", "node-1")

    qpus = get_QPUs(co_located=False)

    assert qpus is not None
    assert len(qpus) == 1
    assert qpus[0] is qpu_mock.return_value

    backend_mock.assert_called_once_with("backend-A")
    qpu_mock.assert_called_once_with(
        id="qpu-1",
        backend=backend_mock.return_value,
        family="fam-A",
        endpoint="tcp://node-1:1234",
    )


def test_hpc_with_family_filter(
    monkeypatch, qpu_backend_mocks
):
    backend_mock, qpu_mock = qpu_backend_mocks

    _mock_qpus_json(
        monkeypatch,
        {
            "qpu-1": {
                "backend": "backend-A",
                "family": "fam-A",
                "net": {"nodename": "node-1", "endpoint": "tcp://node-1:1234"},
            }
        },
    )
    monkeypatch.setenv("SLURMD_NODENAME", "node-1")

    result = get_QPUs(co_located=False, family="other-family")
    assert result is None

    backend_mock.assert_not_called()
    qpu_mock.assert_not_called()


def test_co_located_without_family_filter(
    monkeypatch, qpu_backend_mocks
):
    backend_mock, qpu_mock = qpu_backend_mocks

    _mock_qpus_json(
        monkeypatch,
        {
            "qpu-1": {  # Same node
                "backend": "backend-A",
                "family": "fam-A",
                "net": {
                    "nodename": "node-1",
                    "endpoint": "tcp://node-1:1234",
                    "mode": "hpc",
                },
            },
            "qpu-2": {  # Different node but co_located mode
                "backend": "backend-B",
                "family": "fam-B",
                "net": {
                    "nodename": "node-2",
                    "endpoint": "tcp://node-2:5678",
                    "mode": "co_located",
                },
            },
            "qpu-3": {  # Different node, not co_located
                "backend": "backend-C",
                "family": "fam-C",
                "net": {
                    "nodename": "node-3",
                    "endpoint": "tcp://node-3:9999",
                    "mode": "hpc",
                },
            },
        },
    )
    monkeypatch.setenv("SLURMD_NODENAME", "node-1")

    qpus = get_QPUs(co_located=True)
    assert qpus is not None
    assert len(qpus) == 2
    assert qpus[0] is qpu_mock.return_value

    ids = {call.kwargs["id"] for call in qpu_mock.call_args_list}
    assert ids == {"qpu-1", "qpu-2"}

    backend_calls = [call.args[0] for call in backend_mock.call_args_list]
    assert backend_calls == ["backend-A", "backend-B"]


def test_co_located_with_family_filter(monkeypatch, qpu_backend_mocks):
    backend_mock, qpu_mock = qpu_backend_mocks

    _mock_qpus_json(
        monkeypatch,
        {
            "qpu-1": {
                "backend": "backend-A",
                "family": "fam-A",
                "net": {
                    "nodename": "node-1",
                    "endpoint": "tcp://node-1:1234",
                    "mode": "co_located",
                },
            },
            "qpu-2": {
                "backend": "backend-B",
                "family": "fam-B",
                "net": {
                    "nodename": "node-1",
                    "endpoint": "tcp://node-1:5678",
                    "mode": "co_located",
                },
            },
        },
    )
    monkeypatch.setenv("SLURMD_NODENAME", "node-1")

    qpus = get_QPUs(co_located=True, family="fam-B")
    assert qpus is not None
    assert len(qpus) == 1
    assert qpus[0] is qpu_mock.return_value

    # ensure we filtered to fam-B and used the right backend
    backend_mock.assert_called_once_with("backend-B")
    qpu_mock.assert_called_once()
    assert qpu_mock.call_args.kwargs["family"] == "fam-B"
    assert qpu_mock.call_args.kwargs["id"] == "qpu-2"
