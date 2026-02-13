# test_qjob.py
import json, os, sys
from unittest.mock import Mock
import pytest

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

import cunqa.qjob as qjob_mod
from cunqa.qjob import QJob, gather


@pytest.fixture
def circuit_ir():
    return {
        "id": "circuit-123",
        "classical_registers": {"c": 2},
        "num_clbits": 2,
        "num_qubits": 3,
        "instructions": [{"name": "h", "qubits": [0]}],
        "sending_to": "qpu-1",
        "is_dynamic": False,
        "params": []
    }

@pytest.fixture
def qclient_mock():
    return Mock(name="QClient")

@pytest.fixture
def default_device():
    return {"device_name": "CPU", "target_devices": []}

@pytest.fixture
def logger_mock(monkeypatch):
    mock_logger_cls = Mock(name="logger")
    monkeypatch.setattr(qjob_mod, "logger", mock_logger_cls)

    return mock_logger_cls
    
# ------------------------
# QJob.__init__
# ------------------------

def test_qjob_init_default_run_config(
    qclient_mock, logger_mock, circuit_ir, default_device
):
    job = QJob(qclient_mock, default_device, circuit_ir)

    # basic fields
    assert job._qclient is qclient_mock
    assert job._device is default_device
    assert job._circuit_id == circuit_ir["id"]
    assert job._cregisters == circuit_ir["classical_registers"]
    assert job._params == circuit_ir["params"]
    assert job._future is None
    assert job._result is None
    assert job._updated is False

    # config defaults
    config = job._quantum_task["config"]
    assert config["shots"] == 1024
    assert config["method"] == "automatic"
    assert config["avoid_parallelization"] is False
    assert config["num_clbits"] == circuit_ir["num_clbits"]
    assert config["num_qubits"] == circuit_ir["num_qubits"]
    assert config["seed"] == 123123

    # instructions / metadata copied
    assert job._quantum_task["instructions"] == circuit_ir["instructions"]
    assert job._quantum_task["sending_to"] == circuit_ir["sending_to"]
    assert job._quantum_task["is_dynamic"] == circuit_ir["is_dynamic"]

    # warning because no run_parameters
    logger_mock.warning.assert_called_once()


def test_qjob_init_overrides_run_config(qclient_mock, circuit_ir, default_device):
    job = QJob(
        qclient_mock, 
        default_device, 
        circuit_ir, 
        shots=10, method="statevector", seed=42
    )
    config = job._quantum_task["config"]

    # changed fields
    assert config["shots"] == 10
    assert config["method"] == "statevector"
    assert config["seed"] == 42

    # unchanged fields
    assert config["avoid_parallelization"] is False
    assert config["num_clbits"] == circuit_ir["num_clbits"]
    assert config["num_qubits"] == circuit_ir["num_qubits"]


# ------------------------
# QJob.result property
# ------------------------

def test_result_fetches_once_and_caches(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    future_mock = Mock(name="FutureWrapper")
    payload = {"counts": {"00": 10}}
    future_mock.get.return_value = json.dumps(payload)

    result_instance = Mock(name="Result")
    result_mock = Mock(return_value=result_instance)
    monkeypatch.setattr(qjob_mod, "Result", result_mock)

    job = QJob(qclient_mock, default_device, circuit_ir)
    job._future = future_mock

    # first access
    r1 = job.result
    assert r1 is result_instance
    future_mock.get.assert_called_once()

    # Result constructed correctly
    result_mock.assert_called_once()
    args, kwargs = result_mock.call_args
    assert args[0] == payload
    assert kwargs["circ_id"] == circuit_ir["id"]
    assert kwargs["registers"] == circuit_ir["classical_registers"]

    # second access should not call get again
    future_mock.get.reset_mock()
    r2 = job.result
    assert r2 is result_instance
    future_mock.get.assert_not_called()


def test_result_with_no_future(qclient_mock, circuit_ir, default_device):
    with pytest.raises(RuntimeError) as _:
        job = QJob(qclient_mock, default_device, circuit_ir)
        job.result
    
    assert job._future is None


# ------------------------
# QJob.submit method
# ------------------------

def test_submit_sends_serialized_task(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    future_mock = Mock()
    qclient_mock.send_circuit.return_value = future_mock

    monkeypatch.setattr(
        "cunqa.qjob.json.dumps",
        lambda obj, default=None: "serialized_task"
    )

    job = QJob(qclient_mock, default_device, circuit_ir)
    job.submit()

    qclient_mock.send_circuit.assert_called_once_with("serialized_task")
    assert job._future is future_mock


def test_submit_with_param_values_calls_assign(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    future_mock = Mock()
    qclient_mock.send_circuit.return_value = future_mock 

    job = QJob(qclient_mock, default_device, circuit_ir)

    monkeypatch.setattr(
        "cunqa.qjob.json.dumps",
        lambda obj, default=None: "serialized_task"
    )   

    assign_mock = Mock()
    monkeypatch.setattr(job, "assign_parameters_", assign_mock)

    params = {"theta": 1.0}
    job.submit(param_values=params)

    assign_mock.assert_called_once_with(params)
    qclient_mock.send_circuit.assert_called_once_with("serialized_task")
    assert job._future is future_mock


def test_submit_without_param_values_does_not_call_assign(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    future_mock = Mock()
    qclient_mock.send_circuit.return_value = future_mock

    job = QJob(qclient_mock, default_device, circuit_ir)

    assign_mock = Mock()
    monkeypatch.setattr(job, "assign_parameters_", assign_mock)

    monkeypatch.setattr(
        "cunqa.qjob.json.dumps",
        lambda obj, default: "serialized_task"
    )

    job.submit()

    assign_mock.assert_not_called()


def test_submit_twice_logs_error(
    qclient_mock, logger_mock, circuit_ir, default_device
):
    job = QJob(qclient_mock, default_device, circuit_ir)
    job._future = Mock()  # Already submitted

    job.submit()

    logger_mock.error.assert_called_once()



# ------------------------------
# QJob.upgrade_parameters method
# ------------------------------

def test_upgrade_parameters_calls_assign_and_serializes(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    future_mock = Mock()
    qclient_mock.send_parameters.return_value = future_mock

    job = QJob(qclient_mock, default_device, circuit_ir)
    job._result = Mock()
    job._future = Mock()

    assign_mock = Mock()
    monkeypatch.setattr(job, "assign_parameters_", assign_mock)

    monkeypatch.setattr(
        "cunqa.qjob.json.dumps",
        lambda obj, default=None: '{"theta":1.0}'
    )

    job._params = {"theta": 1.0}

    job.upgrade_parameters({"theta": 1.0})

    assign_mock.assert_called_once_with({"theta": 1.0})
    qclient_mock.send_parameters.assert_called_once_with('{"params":{"theta":1.0}}')
    assert job._future is future_mock
    assert job._updated is False

def test_upgrade_parameters_calls_get_if_result_not_retrieved(
    monkeypatch, qclient_mock, circuit_ir, default_device, logger_mock
):
    future_mock = Mock()
    qclient_mock.send_parameters.return_value = Mock()

    job = QJob(qclient_mock, default_device, circuit_ir)
    job._future = future_mock
    job._result = None

    monkeypatch.setattr(
        "cunqa.qjob.json.dumps",
        lambda obj, default=None: "[1.0,2.0]"
    )
    assign_mock = Mock()
    monkeypatch.setattr(job, "assign_parameters_", assign_mock)

    job._params = [1.0, 2.0]
    job.upgrade_parameters([2.0, 3.0])

    future_mock.get.assert_called_once()

def test_upgrade_parameters_replaces_old_future(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    old_future = Mock()
    new_future = Mock()
    qclient_mock.send_parameters.return_value = new_future

    job = QJob(qclient_mock, default_device, circuit_ir)
    job._future = old_future
    job._result = Mock()

    monkeypatch.setattr(
        "cunqa.qjob.json.dumps",
        lambda obj, default=None: "[1.0]"
    )
    assign_mock = Mock()
    monkeypatch.setattr(job, "assign_parameters_", assign_mock)

    job._params = [1.0]
    job.upgrade_parameters([1.0])

    assert job._future is new_future

def test_upgrade_parameters_builds_correct_message(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    future_mock = Mock()
    qclient_mock.send_parameters.return_value = future_mock

    job = QJob(qclient_mock, default_device, circuit_ir)
    job._future = Mock()
    job._result = Mock()

    monkeypatch.setattr(
        "cunqa.qjob.json.dumps",
        lambda obj, default=None: '{"a": 1}'
    )
    assign_mock = Mock()
    monkeypatch.setattr(job, "assign_parameters_", assign_mock)

    job._params = [object()]
    job.upgrade_parameters({"a": 1})

    expected = '{"params":{"a": 1}}'
    qclient_mock.send_parameters.assert_called_once_with(expected)

def test_upgrade_parameters_accepts_dict(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    qclient_mock.send_parameters.return_value = Mock()

    job = QJob(qclient_mock, default_device, circuit_ir)
    job._future = Mock()
    job._result = Mock()
    
    assign_mock = Mock()
    monkeypatch.setattr(job, "assign_parameters_", assign_mock)

    monkeypatch.setattr(
        "cunqa.qjob.json.dumps",
        lambda obj, default=None: '{"phi": 2.0}'
    )

    job._params = [object()]
    job.upgrade_parameters({"phi": 2.0})

    assign_mock.assert_called_once()
    assert job._updated is False

def test_upgrade_parameters_with_zero_parameters_raises_attribute_error(
    qclient_mock, circuit_ir, default_device
):
    job = QJob(qclient_mock, default_device, circuit_ir)

    job._result = Mock()
    job._future = Mock()

    with pytest.raises(AttributeError) as exc_info:
        job.upgrade_parameters([])

    qclient_mock.send_parameters.assert_not_called()


# ------------------------
# assign_parameters_
# ------------------------

def test_assign_parameters_with_complete_dict_calls_eval(
    qclient_mock, circuit_ir, default_device
):
    job = QJob(qclient_mock, default_device, circuit_ir)

    param_mock = Mock()
    param_mock.variables = [Mock(name="theta"), Mock(name="phi")]
    param_mock.variables[0].name = "theta"
    param_mock.variables[1].name = "phi"
    param_mock.value = 0.0

    job._params = [param_mock]

    param_values = {"theta": 1.0, "phi": 2.0}

    job.assign_parameters_(param_values)

    param_mock.eval.assert_called_once_with({"theta": 1.0, "phi": 2.0})

def test_assign_parameters_dict_partial_keeps_value(
    qclient_mock, circuit_ir, default_device
):
    job = QJob(qclient_mock, default_device, circuit_ir)

    param_mock = Mock()
    param_mock.variables = [Mock()]
    param_mock.variables[0].name = "theta"
    param_mock.value = 5.0

    job._params = [param_mock]

    job.assign_parameters_({}) 

    param_mock.eval.assert_not_called()

def test_assign_parameters_dict_partial_with_none_value_raises(
    qclient_mock, circuit_ir, default_device
):
    job = QJob(qclient_mock, default_device, circuit_ir)

    param_mock = Mock()
    param_mock.variables = [Mock()]
    param_mock.variables[0].name = "theta"
    param_mock.value = None

    job._params = [param_mock]

    with pytest.raises(ValueError):
        job.assign_parameters_({})

def test_assign_parameters_with_list_calls_assign_value(
    qclient_mock, circuit_ir, default_device
):
    job = QJob(qclient_mock, default_device, circuit_ir)

    param1 = Mock()
    param2 = Mock()

    job._params = [param1, param2]

    job.assign_parameters_([1.0, 2.0])

    param1.assign_value.assert_called_once_with(1.0)
    param2.assign_value.assert_called_once_with(2.0)

def test_assign_parameters_list_wrong_length_raises(
    qclient_mock, circuit_ir, default_device
):
    job = QJob(qclient_mock, default_device, circuit_ir)

    job._params = [Mock(), Mock()]

    with pytest.raises(ValueError) as exc_info:
        job.assign_parameters_([1.0])

def test_assign_parameters_dict_mixed_params(
    qclient_mock, circuit_ir, default_device
):
    job = QJob(qclient_mock, default_device, circuit_ir)

    param1 = Mock()
    param1.variables = [Mock()]
    param1.variables[0].name = "theta"
    param1.value = 0.0

    param2 = Mock()
    param2.variables = [Mock()]
    param2.variables[0].name = "phi"
    param2.value = 3.0

    job._params = [param1, param2]

    job.assign_parameters_({"theta": 10.0})

    param1.eval.assert_called_once_with({"theta": 10.0})
    param2.eval.assert_not_called()


# ------------------------
# gather
# ------------------------

def test_gather_returns_list_of_results(monkeypatch):
    # Use Mock to simulate QJob objects with a .result property
    qjob1 = Mock()
    qjob2 = Mock()
    qjob3 = Mock()

    qjob1.result = "r1"
    qjob2.result = "r2"
    qjob3.result = "r3"

    results = gather([qjob1, qjob2, qjob3])

    assert results == ["r1", "r2", "r3"]


def test_gather_with_non_iterable_raises():
    with pytest.raises(AttributeError) as _:
        _ = gather(None)
