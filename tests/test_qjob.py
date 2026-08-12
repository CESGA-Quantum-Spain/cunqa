# test_qjob.py
import json, os, sys
from unittest.mock import Mock, patch
import pytest

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

import cunqa.qjob as qjob_mod
from cunqa.qjob import QJob, ResultBuffer, gather
from cunqa.circuit.parameter import encoder
from sympy import Symbol


@pytest.fixture
def circuit_ir():
    # After `run` remaps the ids, the circuit IR id is a (circuit_id, qpu_id) tuple.
    return {
        "id": ("circuit-123", "qpu-1"),
        "classical_registers": {"c": 2},
        "num_clbits": 2,
        "num_qubits": [3, 0],
        "instructions": [{"name": "h", "qubits": 0}],
        "sending_to": ["qpu-1"],
        "is_dynamic": False,
        "params": []
    }

@pytest.fixture
def qclient_mock():
    """Mock quantum client."""
    client = Mock(name="QClient")
    client.send_parameters = Mock(return_value=Mock())
    return client

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

    # sending_to / is_dynamic now live inside the run config
    assert config["sending_to"] == circuit_ir["sending_to"]
    assert config["is_dynamic"] == circuit_ir["is_dynamic"]
    assert config["qpu_id"] == circuit_ir["id"][1]

    # instructions / metadata copied
    assert job._quantum_task["instructions"] == circuit_ir["instructions"]
    assert job._quantum_task["id"] == circuit_ir["id"][0]

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
    # The job is not submitted through `submit`, so the buffer is told by hand that a result of
    # this circuit is on its way.
    job._result_buffer.register(job._id)

    # first access
    r1 = job.result
    assert r1 is result_instance
    future_mock.get.assert_called_once()

    # Result constructed correctly
    result_mock.assert_called_once()
    args, kwargs = result_mock.call_args
    assert args[0] == payload
    # kwargs["circ_id"] == circuit_ir["id"]
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

@pytest.fixture
def qjob_instance(qclient_mock, default_device, circuit_ir):
    """Create a QJob instance with mocked dependencies."""
    obj = QJob(qclient_mock, default_device, circuit_ir)
    obj._circuit_id = "test_circuit_123"
    obj._params = {}
    obj._result = Mock()  # Simulate that a result exists
    obj._future = None
    obj._updated = False
    obj.assign_parameters_ = Mock()
    return obj

def test_upgrade_with_dict_parameters(qjob_instance, qclient_mock):
    """Test upgrading parameters with a dictionary input."""
    param_dict = {"theta": 0.5, "phi": 1.2}
    
    qjob_instance.upgrade_parameters(param_dict)
    
    qjob_instance.assign_parameters_.assert_called_once_with(param_dict)
    qclient_mock.send_parameters.assert_called_once()
    assert qjob_instance._updated is False


def test_upgrade_with_list_parameters(qjob_instance, qclient_mock):
    """Test upgrading parameters with a list input."""
    param_list = [0.5, 1.2, 2.1]
    
    qjob_instance.upgrade_parameters(param_list)
    
    qjob_instance.assign_parameters_.assert_called_once_with(param_list)
    qclient_mock.send_parameters.assert_called_once()
    assert qjob_instance._updated is False


# ERROR HANDLING TESTS 

def test_no_result_and_no_future_raises_runtime_error(qjob_instance, qclient_mock):
    """Test that RuntimeError is raised when no circuit was sent."""
    qjob_instance._result = None
    qjob_instance._future = None
    
    with pytest.raises(RuntimeError, match="No circuit was sent before calling update_parameters"):
        qjob_instance.upgrade_parameters({"theta": 0.5})


def test_no_result_but_future_exists_gets_result(qjob_instance, qclient_mock):
    """Test that when _result is None but _future exists, the result is fetched."""
    qjob_instance._result = None
    mock_future = Mock()
    mock_future.get.return_value = json.dumps({"counts": {"00": 10}, "id": qjob_instance._id})
    qjob_instance._future = mock_future
    # The fixture does not go through `submit`, so the pending result is registered by hand.
    qjob_instance._result_buffer.register(qjob_instance._id)

    qjob_instance.upgrade_parameters({"theta": 0.5})
    
    # The future's get() method should be called
    mock_future.get.assert_called_once()

def test_empty_dict_raises_attribute_error(qjob_instance):
    """Test that an empty dictionary raises AttributeError."""
    with pytest.raises(AttributeError, match="No parameter list has been provided"):
        qjob_instance.upgrade_parameters({})


def test_empty_list_raises_attribute_error(qjob_instance):
    """Test that an empty list raises AttributeError."""
    with pytest.raises(AttributeError, match="No parameter list has been provided"):
        qjob_instance.upgrade_parameters([])
        

# STATE MANAGEMENT TESTS 

def test_updated_flag_set_to_false_on_success(qjob_instance, qclient_mock):
    """Test that _updated flag is set to False on successful send."""
    qjob_instance._updated = True
    
    qjob_instance.upgrade_parameters({"theta": 0.5})
    
    assert qjob_instance._updated is False


def test_future_is_updated_on_success(qjob_instance, qclient_mock):
    """Test that _future is updated with the new future from send_parameters."""
    new_future = Mock()
    qclient_mock.send_parameters.return_value = new_future
    
    qjob_instance.upgrade_parameters({"theta": 0.5})
    
    assert qjob_instance._future == new_future


def test_assign_parameters_called_before_send(qjob_instance, qclient_mock):
    """Test that assign_parameters_ is called before send_parameters."""
    call_order = []
    qjob_instance.assign_parameters_.side_effect = lambda x: call_order.append("assign")
    qclient_mock.send_parameters.side_effect = lambda x: (call_order.append("send"), Mock())[1]
    
    qjob_instance.upgrade_parameters({"theta": 0.5})
    
    assert call_order == ["assign", "send"]


def test_upgrade_with_multiple_parameters(qjob_instance, qclient_mock):
    """Test upgrading multiple parameters at once."""
    params = {"theta": 0.5, "phi": 1.2, "lambda": 2.1}
    
    qjob_instance.upgrade_parameters(params)
    
    qjob_instance.assign_parameters_.assert_called_once_with(params)
    qclient_mock.send_parameters.assert_called_once()


def test_json_encoder_is_used(qjob_instance, qclient_mock):
    """Test that the custom encoder is used for JSON serialization."""
    qjob_instance._params = {"theta": 0.5}

    with patch('json.dumps') as mock_dumps:
        mock_dumps.return_value = '{"theta": 0.5}'
        qjob_instance.upgrade_parameters({"theta": 0.5})

        # upgrade_parameters serializes both the params and the config, and the
        # custom encoder must be passed on every call.
        assert mock_dumps.call_count == 2
        for call in mock_dumps.call_args_list:
            assert call.kwargs["default"] == encoder


# INTEGRATION TESTS

def test_full_workflow_dict_parameters(qjob_instance, qclient_mock):
    """Test a complete workflow with dictionary parameters."""
    params = {"theta": 0.785, "phi": 1.571}
    qjob_instance._params = params
    
    qjob_instance.upgrade_parameters(params)
    
    qjob_instance.assign_parameters_.assert_called_once_with(params)
    qclient_mock.send_parameters.assert_called_once()
    assert qjob_instance._updated is False
    assert qjob_instance._future is not None


def test_full_workflow_list_parameters(qjob_instance, qclient_mock):
    """Test a complete workflow with list parameters."""
    params = [0.785, 1.571, 3.14159]
    qjob_instance._params = params
    
    qjob_instance.upgrade_parameters(params)
    
    qjob_instance.assign_parameters_.assert_called_once_with(params)
    qclient_mock.send_parameters.assert_called_once()
    assert qjob_instance._updated is False


def test_multiple_consecutive_upgrades(qjob_instance, qclient_mock):
    """Test multiple consecutive parameter upgrades."""
    qjob_instance.upgrade_parameters({"theta": 0.5})
    qjob_instance.upgrade_parameters({"theta": 1.0})
    qjob_instance.upgrade_parameters({"theta": 1.5})

    assert qclient_mock.send_parameters.call_count == 3
    assert qjob_instance.assign_parameters_.call_count == 3


def test_upgrade_parameters_sends_the_circuit_id(qjob_instance, qclient_mock):
    """The update must carry the id, otherwise the result of it could not be routed back."""
    qjob_instance.upgrade_parameters({"theta": 0.5})

    message = json.loads(qclient_mock.send_parameters.call_args.args[0])
    assert message["id"] == qjob_instance._id


def test_upgrade_parameters_registers_the_new_result(qjob_instance, qclient_mock):
    """After resending, a new result is on its way and the buffer must expect it."""
    qjob_instance.upgrade_parameters({"theta": 0.5})

    assert qjob_instance._result_buffer._outstanding[qjob_instance._id] == 1


# ------------------------
# ResultBuffer
# ------------------------

def _raw_result(circuit_id=None, counts=None):
    """Builds a result as the vQPU sends it: a raw json string, with the id stamped on it."""
    result = {"counts": counts if counts is not None else {"00": 1024}, "time_taken": 0.1}
    if circuit_id is not None:
        result["id"] = circuit_id
    return json.dumps(result)


@pytest.fixture
def result_buffer():
    return ResultBuffer()


def test_buffer_returns_the_result_asked_for(result_buffer):
    future = Mock()
    future.get.return_value = _raw_result("circuit-a")
    result_buffer.register("circuit-a")

    assert json.loads(result_buffer.get(future, "circuit-a"))["id"] == "circuit-a"
    future.get.assert_called_once()


def test_buffer_stores_results_of_other_jobs(result_buffer):
    """Results arriving out of order are kept for the job that owns them."""
    future = Mock()
    # The vQPUs answer in the opposite order to the one in which the jobs are asked for.
    future.get.side_effect = [_raw_result("circuit-b"), _raw_result("circuit-a")]
    result_buffer.register("circuit-a")
    result_buffer.register("circuit-b")

    assert json.loads(result_buffer.get(future, "circuit-a"))["id"] == "circuit-a"
    assert future.get.call_count == 2

    # The result of `circuit-b` was stored on the way, so it is taken from there and the
    # connection is not read again.
    assert json.loads(result_buffer.get(future, "circuit-b"))["id"] == "circuit-b"
    assert future.get.call_count == 2


def test_buffer_keeps_the_order_of_results_with_the_same_id(result_buffer):
    """The same circuit can be in flight twice, and its results are kept first in first out."""
    future = Mock()
    future.get.side_effect = [
        _raw_result("circuit-b", counts={"00": 1}),
        _raw_result("circuit-b", counts={"11": 2}),
        _raw_result("circuit-a"),
    ]
    result_buffer.register("circuit-a")
    result_buffer.register("circuit-b")
    result_buffer.register("circuit-b")

    result_buffer.get(future, "circuit-a")

    assert json.loads(result_buffer.get(future, "circuit-b"))["counts"] == {"00": 1}
    assert json.loads(result_buffer.get(future, "circuit-b"))["counts"] == {"11": 2}
    assert future.get.call_count == 3


def test_buffer_falls_back_to_fifo_with_results_with_no_id(result_buffer):
    """Results of a real QPU carry no id, so they are given to whoever is asking."""
    future = Mock()
    future.get.return_value = _raw_result()
    result_buffer.register("circuit-a")

    assert json.loads(result_buffer.get(future, "circuit-a"))["counts"] == {"00": 1024}
    future.get.assert_called_once()


def test_buffer_does_not_read_when_nothing_is_pending(result_buffer):
    """Asking for a result that nobody is waiting for must not block on the connection."""
    future = Mock()

    with pytest.raises(RuntimeError, match="No result is pending"):
        result_buffer.get(future, "circuit-a")

    future.get.assert_not_called()


def test_buffer_does_not_deliver_the_same_result_twice(result_buffer):
    future = Mock()
    future.get.return_value = _raw_result("circuit-a")
    result_buffer.register("circuit-a")

    result_buffer.get(future, "circuit-a")

    with pytest.raises(RuntimeError, match="No result is pending"):
        result_buffer.get(future, "circuit-a")


# ------------------------
# QJob and ResultBuffer
# ------------------------

def test_submit_registers_the_expected_result(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    qclient_mock.send_circuit.return_value = Mock()

    job = QJob(qclient_mock, default_device, circuit_ir)
    job.submit()

    assert job._result_buffer._outstanding[job._id] == 1


def test_results_are_routed_between_jobs_of_the_same_qpu(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    """Two jobs sharing a vQPU get their own result even if they are asked for out of order."""
    monkeypatch.setattr(
        qjob_mod, "Result",
        lambda result, circ_id, registers: result  # the raw dict is enough to identify it
    )

    other_circuit_ir = dict(circuit_ir, id=("circuit-456", "qpu-1"))

    buffer = ResultBuffer()
    future = Mock()
    # The result of the second job is the first one to arrive.
    future.get.side_effect = [_raw_result("circuit-456"), _raw_result("circuit-123")]
    qclient_mock.send_circuit.return_value = future

    job_1 = QJob(qclient_mock, default_device, circuit_ir, result_buffer=buffer)
    job_2 = QJob(qclient_mock, default_device, other_circuit_ir, result_buffer=buffer)
    job_1.submit()
    job_2.submit()

    assert job_1.result["id"] == "circuit-123"
    assert job_2.result["id"] == "circuit-456"
    assert future.get.call_count == 2


def test_gather_returns_each_result_to_its_job(
    monkeypatch, qclient_mock, circuit_ir, default_device
):
    monkeypatch.setattr(
        qjob_mod, "Result",
        lambda result, circ_id, registers: result
    )

    other_circuit_ir = dict(circuit_ir, id=("circuit-456", "qpu-1"))

    buffer = ResultBuffer()
    future = Mock()
    future.get.side_effect = [_raw_result("circuit-456"), _raw_result("circuit-123")]
    qclient_mock.send_circuit.return_value = future

    jobs = [
        QJob(qclient_mock, default_device, circuit_ir, result_buffer=buffer),
        QJob(qclient_mock, default_device, other_circuit_ir, result_buffer=buffer)
    ]
    for job in jobs:
        job.submit()

    assert [result["id"] for result in gather(jobs)] == ["circuit-123", "circuit-456"]

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
