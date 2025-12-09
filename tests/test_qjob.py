# test_qjob.py
import json, os, sys
from unittest.mock import Mock
import pytest

HOME = os.getenv("HOME")
sys.path.insert(0, HOME)

import cunqa.qjob as qjob_module
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
        "has_cc": False,
    }

@pytest.fixture
def qclient_mock():
    return Mock(name="QClient")

@pytest.fixture
def logger_mock(monkeypatch):
    mock_logger_cls = Mock(name="logger")
    monkeypatch.setattr(qjob_module, "logger", mock_logger_cls)

    return mock_logger_cls


# ------------------------
# QJob.__init__
# ------------------------

def test_qjob_init_default_run_config(qclient_mock, logger_mock, circuit_ir):
    job = QJob(qclient_mock, circuit_ir)
    task = json.loads(job._quantum_task)

    # basic fields
    assert job._qclient is qclient_mock
    assert job._circuit_id == circuit_ir["id"]
    assert job._cregisters == circuit_ir["classical_registers"]
    assert job._future is None
    assert job._result is None
    assert job._updated is False

    # config defaults
    config = task["config"]
    assert config["shots"] == 1024
    assert config["method"] == "automatic"
    assert config["avoid_parallelization"] is False
    assert config["num_clbits"] == circuit_ir["num_clbits"]
    assert config["num_qubits"] == circuit_ir["num_qubits"]
    assert config["seed"] == 123123

    # instructions / metadata copied
    assert task["instructions"] == circuit_ir["instructions"]
    assert task["sending_to"] == circuit_ir["sending_to"]
    assert task["is_dynamic"] == circuit_ir["is_dynamic"]
    assert task["has_cc"] == circuit_ir["has_cc"]

    # warning because no run_parameters
    logger_mock.warning.assert_called_once()


def test_qjob_init_overrides_run_config(qclient_mock, circuit_ir):
    job = QJob(qclient_mock, circuit_ir, shots=10, method="statevector", seed=42)

    task = json.loads(job._quantum_task)
    config = task["config"]

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

def test_result_fetches_once_and_caches(monkeypatch, qclient_mock, circuit_ir):
    future_mock = Mock(name="FutureWrapper")
    payload = {"counts": {"00": 10}}
    future_mock.get.return_value = json.dumps(payload)

    result_instance = Mock(name="Result")
    result_mock = Mock(return_value=result_instance)
    monkeypatch.setattr(qjob_module, "Result", result_mock)

    job = QJob(qclient_mock, circuit_ir)
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


def test_result_with_no_future(qclient_mock, circuit_ir):
    with pytest.raises(RuntimeError) as _:
        job = QJob(qclient_mock, circuit_ir)
        job.result
    
    assert job._future is None


# ------------------------
# QJob.time_taken property
# ------------------------

def test_time_taken_returns_correctly(qclient_mock, circuit_ir):
    job = QJob(qclient_mock, circuit_ir)
    job._future = Mock()  # submitted
    result_mock = Mock()
    result_mock.time_taken = "0.123"
    job._result = result_mock

    assert job.time_taken == "0.123"


def test_time_taken_no_time_attribute(
    qclient_mock, logger_mock, circuit_ir
):
    job = QJob(qclient_mock, circuit_ir)
    job._future = Mock(name="FutureWrapper")
    job._result = Mock(name="Result")
    del job._result.time_taken

    value = job.time_taken

    assert value == ""
    logger_mock.error.assert_called_once_with("Time taken not available.")


def test_time_taken_without_result(qclient_mock, circuit_ir):
    with pytest.raises(RuntimeError) as _:
        job = QJob(qclient_mock, circuit_ir)
        job._future = Mock()
        job._result = None
        job.time_taken


def test_time_taken_without_future(qclient_mock, circuit_ir):
    with pytest.raises(RuntimeError) as _:
        job = QJob(qclient_mock, circuit_ir)
        job.time_taken

    assert job._future is None


# ------------------------
# QJob.submit method
# ------------------------

def test_submit_task_correctly(qclient_mock, circuit_ir):
    future_mock = Mock()
    qclient_mock.send_circuit.return_value = future_mock

    job = QJob(qclient_mock, circuit_ir)

    job.submit()

    qclient_mock.send_circuit.assert_called_once_with(job._quantum_task)
    assert job._future is future_mock


def test_submit_twice_logs_error(qclient_mock, logger_mock, circuit_ir):
    job = QJob(qclient_mock, circuit_ir)
    job._future = Mock()  # pretend it was already submitted

    job.submit()

    logger_mock.error.assert_called_once()


# ------------------------
# QJob.upgrade_parameters method
# ------------------------

def test_upgrade_parameters_with_empty_parameters(
    qclient_mock, circuit_ir
):
    job = QJob(qclient_mock, circuit_ir)
    job._future = Mock(name="FutureWrapper")
    job._result = Mock(name="Result") 

    with pytest.raises(AttributeError) as _:
        parameters = []
        job.upgrade_parameters(parameters)

def test_upgrade_parameters_with_result_and_future(
    qclient_mock, circuit_ir
):
    future_mock = Mock(name="NewFutureWrapper")
    qclient_mock.send_parameters.return_value = future_mock

    job = QJob(qclient_mock, circuit_ir)
    job._future = Mock(name="OldFutureWrapper")
    job._result = Mock(name="Result")  # result already retrieved

    parameters = [1.0, 2.0, 3.0]
    expected_message = '{{"params":{} }}'.format(parameters).replace("'", '"')

    job.upgrade_parameters(parameters)

    qclient_mock.send_parameters.assert_called_once_with(expected_message)
    assert job._future is future_mock
    assert job._updated is False

def test_upgrade_parameters_without_result_but_with_future(
    qclient_mock, circuit_ir
):
    future_mock = Mock()
    qclient_mock.send_parameters.return_value = Mock()

    job = QJob(qclient_mock, circuit_ir)
    job._future = future_mock
    job._result = None

    parameters = [0.1, 0.2]
    job.upgrade_parameters(parameters)

    future_mock.get.assert_called_once()
    assert job._updated is False

def test_upgrade_parameters_without_result_and_future(
    qclient_mock, circuit_ir
):
    job = QJob(qclient_mock, circuit_ir)
    job._future = None
    job._result = None  # result not retrieved yet

    with pytest.raises(RuntimeError) as _:
        parameters = [0.1, 0.2]
        job.upgrade_parameters(parameters)

def test_upgrade_parameters_logs_error_on_exception(
    qclient_mock, logger_mock, circuit_ir, monkeypatch
):
    future_mock = Mock()
    qclient_mock.send_parameters.side_effect = RuntimeError("boom")

    job = QJob(qclient_mock, circuit_ir)
    job._future = future_mock
    job._result = Mock()

    parameters = [0.5]
    job.upgrade_parameters(parameters)

    assert job._updated is True
    assert logger_mock.error.call_count == 1


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


def test_gather_with_non_iterable_logs_error_and_returns_none(logger_mock):
    res = gather(None)

    assert res is None
    logger_mock.error.assert_called_once()
