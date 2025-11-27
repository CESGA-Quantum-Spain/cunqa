from __future__ import annotations

import os
import sys

HOME = os.getenv("HOME")
sys.path.insert(0, HOME)

import pytest
from unittest.mock import MagicMock
import cunqa.qpu as qpu_module
from cunqa.qpu import QPU


# Helpers para controlar inspect.stack()[1].function
class _FrameInfo:
    def __init__(self, function: str):
        self.function = function


def _stack_with_caller(caller_name: str):
    # QPU.run mira el índice 1
    return [_FrameInfo("anything0"), _FrameInfo(caller_name)]


@pytest.fixture
def backend():
    # Backend real no es necesario para estas pruebas
    return MagicMock(name="Backend")


@pytest.fixture
def qclient():
    qc = MagicMock(name="QClient")
    qc.connect = MagicMock(name="connect")
    return qc


@pytest.fixture
def qpu(qclient, backend):
    return QPU(
        id=7,
        qclient=qclient,
        backend=backend,
        name="qpu-7",
        family="fam",
        endpoint="https://fake.endpoint/qpu/7",
    )


def test_init_sets_attributes_and_properties(qpu, qclient, backend):
    assert qpu.id == 7
    assert qpu.name == "qpu-7"
    assert qpu.backend is backend

    assert qpu._qclient is qclient
    assert qpu._endpoint == "https://fake.endpoint/qpu/7"
    assert qpu._connected is False


def test_run_connects_once_and_submits_job(monkeypatch, qpu, qclient, backend):
    # QJob mock
    qjob_instance = MagicMock(name="QJobInstance")
    QJobMock = MagicMock(name="QJob", return_value=qjob_instance)
    monkeypatch.setattr(qpu_module, "QJob", QJobMock)

    # Evitar transpile
    monkeypatch.setattr(qpu_module.inspect, "stack", lambda: _stack_with_caller("test_fn"))

    circuit = {"foo": "bar"}
    qjob = qpu.run(circuit, shots=123, method="statevector")

    # Conecta y marca conectado
    qclient.connect.assert_called_once_with("https://fake.endpoint/qpu/7")
    assert qpu._connected is True

    # Crea el job con el circuito y parámetros
    QJobMock.assert_called_once()
    args, kwargs = QJobMock.call_args
    assert args[0] is qclient
    assert args[1] is backend
    assert args[2] == circuit
    assert kwargs["shots"] == 123
    assert kwargs["method"] == "statevector"

    # Submit llamado
    qjob_instance.submit.assert_called_once()
    assert qjob is qjob_instance


def test_run_does_not_reconnect_if_already_connected(monkeypatch, qpu, qclient):
    qpu._connected = True

    qjob_instance = MagicMock()
    monkeypatch.setattr(qpu_module, "QJob", MagicMock(return_value=qjob_instance))
    monkeypatch.setattr(qpu_module.inspect, "stack", lambda: _stack_with_caller("test_fn"))

    qpu.run({"x": 1})

    qclient.connect.assert_not_called()


def test_run_transpile_calls_transpiler_and_passes_result_to_qjob(monkeypatch, qpu):
    monkeypatch.setattr(qpu_module.inspect, "stack", lambda: _stack_with_caller("test_fn"))

    transpiled = {"transpiled": True}

    transpiler_mock = MagicMock(name="transpiler", return_value=transpiled)
    monkeypatch.setattr(qpu_module, "transpiler", transpiler_mock)

    qjob_instance = MagicMock(name="QJobInstance")
    QJobMock = MagicMock(return_value=qjob_instance)
    monkeypatch.setattr(qpu_module, "QJob", QJobMock)

    original_circuit = {"original": True}

    qpu.run(
        original_circuit,
        transpile=True,
        initial_layout=[1, 0, 2],
        opt_level=3,
        shots=50,
    )

    transpiler_mock.assert_called_once_with(
        original_circuit,
        qpu.backend,
        initial_layout=[1, 0, 2],
        opt_level=3,
    )

    # QJob recibe el circuito transpileado
    args, _ = QJobMock.call_args
    assert args[2] == transpiled


def test_run_transpile_failure_raises_transpileerror(monkeypatch, qpu):
    monkeypatch.setattr(qpu_module.inspect, "stack", lambda: _stack_with_caller("test_fn"))

    def boom(*_args, **_kwargs):
        raise RuntimeError("transpiler exploded")

    monkeypatch.setattr(qpu_module, "transpiler", boom)

    # Importante: la clase usa TranspileError del propio módulo
    with pytest.raises(qpu_module.TranspileError):
        qpu.run({"c": 1}, transpile=True)


def test_run_disallows_distributed_cunqacircuit(monkeypatch, qpu):
    monkeypatch.setattr(qpu_module.inspect, "stack", lambda: _stack_with_caller("test_fn"))

    class FakeCunqaCircuit:
        def __init__(self, has_cc: bool, has_qc: bool):
            self.has_cc = has_cc
            self.has_qc = has_qc

    # Para que isinstance(circuit, CunqaCircuit) funcione
    monkeypatch.setattr(qpu_module, "CunqaCircuit", FakeCunqaCircuit)

    with pytest.raises(SystemExit):
        qpu.run(FakeCunqaCircuit(has_cc=True, has_qc=False))


def test_run_disallows_distributed_dict(monkeypatch, qpu):
    monkeypatch.setattr(qpu_module.inspect, "stack", lambda: _stack_with_caller("test_fn"))

    with pytest.raises(SystemExit):
        qpu.run({"has_cc": True})  # o {"has_qc": True}


def test_run_allows_distributed_when_called_from_run_distributed(monkeypatch, qpu):
    # Simula que el caller es run_distributed -> se salta el bloqueo
    monkeypatch.setattr(qpu_module.inspect, "stack", lambda: _stack_with_caller("run_distributed"))

    qjob_instance = MagicMock()
    monkeypatch.setattr(qpu_module, "QJob", MagicMock(return_value=qjob_instance))

    # No debe tirar SystemExit aunque sea "distribuido"
    qpu.run({"has_cc": True, "payload": 1})
    qjob_instance.submit.assert_called_once()


def test_run_raises_systemexit_if_qjob_submit_fails(monkeypatch, qpu):
    monkeypatch.setattr(qpu_module.inspect, "stack", lambda: _stack_with_caller("test_fn"))

    qjob_instance = MagicMock()
    qjob_instance.submit.side_effect = Exception("submit failed")
    monkeypatch.setattr(qpu_module, "QJob", MagicMock(return_value=qjob_instance))

    with pytest.raises(SystemExit):
        qpu.run({"ok": True})


def test_run_does_not_call_transpiler_if_transpile_false_even_with_layout(monkeypatch, qpu):
    monkeypatch.setattr(qpu_module.inspect, "stack", lambda: _stack_with_caller("test_fn"))

    # Si se llamase, fallaría el test
    transpiler_mock = MagicMock(side_effect=AssertionError("transpiler should not be called"))
    monkeypatch.setattr(qpu_module, "transpiler", transpiler_mock)

    qjob_instance = MagicMock()
    monkeypatch.setattr(qpu_module, "QJob", MagicMock(return_value=qjob_instance))

    qpu.run({"c": 1}, transpile=False, initial_layout=[0, 1], opt_level=9)

    transpiler_mock.assert_not_called()
