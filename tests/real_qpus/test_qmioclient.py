import os, sys
import json
from unittest.mock import Mock

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)


import pytest
import cunqa.real_qpus.qmioclient as qmioclient_mod


# -----------------------
# Helpers puros
# -----------------------

@pytest.mark.parametrize(
    "optimization, expected",
    [(0, 1), (1, 18), (2, 30)],
)
def test_optimization_options_builder_valid(optimization, expected):
    assert qmioclient_mod._optimization_options_builder(optimization=optimization) == expected


def test_optimization_options_builder_invalid_backend_raises():
    with pytest.raises(TypeError):
        qmioclient_mod._optimization_options_builder(optimization=0, optimization_backend="QAT")


def test_optimization_options_builder_invalid_level_raises():
    with pytest.raises(ValueError):
        qmioclient_mod._optimization_options_builder(optimization=99)


@pytest.mark.parametrize(
    "res_format, expected",
    [
        ("binary_count", (1, 3)),
        ("raw", (1, 2)),
        ("binary", (2, 2)),
        ("squash_binary_result_arrays", (2, 6)),
    ],
)
def test_results_format_builder_valid(res_format, expected):
    assert qmioclient_mod._results_format_builder(res_format=res_format) == expected


def test_results_format_builder_invalid_raises():
    with pytest.raises(KeyError):
        qmioclient_mod._results_format_builder(res_format="nope")


def test_config_builder_contains_expected_values():
    cfg_str = qmioclient_mod._config_builder(
        shots=123,
        repetition_period=0.5,
        optimization=2,
        res_format="binary",
    )
    cfg = json.loads(cfg_str)

    assert cfg["$data"]["repeats"] == 123
    assert cfg["$data"]["repetition_period"] == 0.5

    # binary -> (2,2)
    assert cfg["$data"]["results_format"]["$data"]["format"]["$value"] == 2
    assert cfg["$data"]["results_format"]["$data"]["transforms"]["$value"] == 2

    # optimization=2 -> opt_value 30
    assert cfg["$data"]["optimizations"]["$value"] == 30


def test_get_run_config_overrides_only_known_keys():
    run_config = {
        "shots": 10,
        "optimization": 1,
        "res_format": "raw",
        "repetition_period": 1.25,
        "unknown": "ignored",
    }
    cfg_str = qmioclient_mod._get_run_config(run_config)
    cfg = json.loads(cfg_str)

    assert cfg["$data"]["repeats"] == 10
    assert cfg["$data"]["repetition_period"] == 1.25

    # raw -> (1,2)
    assert cfg["$data"]["results_format"]["$data"]["format"]["$value"] == 1
    assert cfg["$data"]["results_format"]["$data"]["transforms"]["$value"] == 2

    # optimization=1 -> 18
    assert cfg["$data"]["optimizations"]["$value"] == 18


# -----------------------
# QMIOFuture
# -----------------------

def test_qmiofuture_valid_always_true():
    f = qmioclient_mod.QMIOFuture()
    assert f.valid() is True


def test_qmiofuture_get_with_socket_returns_json_with_results_and_time(monkeypatch):
    sock = Mock()
    sock.recv_pyobj = Mock(return_value={"results": {"00": 7}})

    # tiempo determinista
    monkeypatch.setattr(qmioclient_mod.time, "time_ns", Mock(return_value=3000))

    f = qmioclient_mod.QMIOFuture(socket=sock, start_time=1000)
    out = json.loads(f.get())

    assert out["qmio_results"] == {"00": 7}
    assert out["time_taken"] == pytest.approx((3000 - 1000) / 1e9)


def test_qmiofuture_get_with_error_returns_error_json():
    f = qmioclient_mod.QMIOFuture(error="boom")
    out = json.loads(f.get())
    assert "ERROR" in out
    assert "boom" in out["ERROR"]


def test_qmiofuture_get_with_no_socket_no_error_returns_generic_error():
    f = qmioclient_mod.QMIOFuture()
    out = json.loads(f.get())
    assert out == {"ERROR": "An error occured in QMIO."}


# -----------------------
# QMIOClient
# -----------------------

def test_qmioclient_connect_creates_dealer_and_connects(monkeypatch):
    sock = Mock()
    ctx = Mock()
    ctx.socket = Mock(return_value=sock)

    monkeypatch.setattr(qmioclient_mod.zmq, "Context", Mock(return_value=ctx))
    monkeypatch.setattr(qmioclient_mod.zmq, "DEALER", object())

    c = qmioclient_mod.QMIOClient()
    c.connect("tcp://1.2.3.4:9999")

    ctx.socket.assert_called_once()
    sock.connect.assert_called_once_with("tcp://1.2.3.4:9999")
    assert c.socket is sock


def test_send_circuit_when_params_dict_sends_dict_and_does_not_set_last_task(monkeypatch):
    sock = Mock()
    monkeypatch.setattr(qmioclient_mod.time, "time_ns", Mock(return_value=111))

    c = qmioclient_mod.QMIOClient()
    c.socket = sock
    c._last_quantum_task = False

    qt_str = json.dumps({"params": [1.0, 2.0]})
    future = c.send_circuit(qt_str)

    sock.send_pyobj.assert_called_once_with({"params": [1.0, 2.0]})
    assert isinstance(future, qmioclient_mod.QMIOFuture)
    assert future.socket is sock
    assert future.start_time == 111
    assert c._last_quantum_task is False


def test_send_circuit_when_circuit_builds_run_config_and_sends_tuple(monkeypatch):
    sock = Mock()
    monkeypatch.setattr(qmioclient_mod.time, "time_ns", Mock(return_value=222))

    get_run_config = Mock(return_value="CFGSTR")
    monkeypatch.setattr(qmioclient_mod, "_get_run_config", get_run_config)

    c = qmioclient_mod.QMIOClient()
    c.socket = sock
    c._last_quantum_task = False

    qt = {"config": {"shots": 10, "optimization": 1}, "circuit": "X"}
    future = c.send_circuit(json.dumps(qt))

    assert c._last_quantum_task is True
    get_run_config.assert_called_once_with(qt["config"])
    sock.send_pyobj.assert_called_once_with((qt, "CFGSTR"))

    assert future.socket is sock
    assert future.start_time == 222


def test_send_circuit_on_zmq_error_closes_socket_and_returns_future_with_error(monkeypatch):
    sock = Mock()
    sock.send_pyobj = Mock(side_effect=qmioclient_mod.zmq.ZMQError("nope"))

    monkeypatch.setattr(qmioclient_mod.time, "time_ns", Mock(return_value=333))

    c = qmioclient_mod.QMIOClient()
    c.socket = sock

    future = c.send_circuit(json.dumps({"params": [1.0]}))

    sock.close.assert_called_once()
    assert future.socket is None
    assert future.error is not None


def test_send_parameters_without_last_task_returns_error_future():
    c = qmioclient_mod.QMIOClient()
    c._last_quantum_task = False

    future = c.send_parameters(json.dumps({"params": [1.0]}))
    out = json.loads(future.get())

    assert "ERROR" in out
    assert "parametric circuit" in out["ERROR"]


def test_send_parameters_with_last_task_delegates_to_send_circuit(monkeypatch):
    c = qmioclient_mod.QMIOClient()
    c._last_quantum_task = True

    send_circuit = Mock(return_value="FUTURE")
    monkeypatch.setattr(c, "send_circuit", send_circuit)

    params = json.dumps({"params": [9.9]})
    out = c.send_parameters(params)

    send_circuit.assert_called_once_with(params)
    assert out == "FUTURE"
