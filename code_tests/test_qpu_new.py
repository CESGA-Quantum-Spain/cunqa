import os
import sys

HOME = os.getenv("HOME")
sys.path.insert(0, HOME)

import pytest
from unittest.mock import Mock, patch

import cunqa.qpu as qpu_mod
from cunqa.qpu import QPU

def test_init_connects_when_endpoint_is_ok():
    backend = Mock(name="Backend")
    endpoint = "http://good-endpoint"

    with patch.object(qpu_mod, "QClient") as QClientMock:
        qclient_instance = Mock()
        QClientMock.return_value = qclient_instance

        qpu = QPU(id=1, backend=backend, name="n", family="f", endpoint=endpoint)

    QClientMock.assert_called_once_with()
    qclient_instance.connect.assert_called_once_with(endpoint)

    assert qpu._id == 1
    assert qpu._backend is backend
    assert qpu._name == "n"
    assert qpu._family == "f"
    assert qpu._qclient is qclient_instance


def test_init_raises_when_endpoint_is_bad():
    backend = Mock(name="Backend")
    endpoint = "http://bad-endpoint"

    with patch.object(qpu_mod, "QClient") as QClientMock:
        qclient_instance = Mock()
        qclient_instance.connect.side_effect = ConnectionError("cannot connect")
        QClientMock.return_value = qclient_instance

        with pytest.raises(ConnectionError):
            QPU(id=1, backend=backend, name="n", family="f", endpoint=endpoint)

    QClientMock.assert_called_once_with()
    qclient_instance.connect.assert_called_once_with(endpoint)

@pytest.fixture
def qpu():
    backend = Mock(name="Backend")
    endpoint = "http://any-endpoint"
    with patch.object(qpu_mod, "QClient") as QClientMock:
        qclient_instance = Mock(name="QClientInstance")
        QClientMock.return_value = qclient_instance
        qpu = QPU(id=1, backend=backend, name="n", family="f", endpoint=endpoint)
    return qpu

def test_execute_creates_qjob_submits_and_returns(qpu):
    circuit = {"some": "circuit"}
    run_parameters = {"shots": 100, "method": "statevector"}

    with patch.object(qpu_mod, "QJob") as QJobMock:
        qjob_instance = Mock(name="QJobInstance")
        QJobMock.return_value = qjob_instance

        result = qpu.execute(circuit, **run_parameters)

    # se construye QJob con los args correctos
    QJobMock.assert_called_once_with(qpu._qclient, qpu._backend, circuit, **run_parameters)
    # se envía
    qjob_instance.submit.assert_called_once_with()
    # se devuelve el mismo objeto
    assert result is qjob_instance


def test_execute_reraises_if_qjob_submit_fails(qpu):
    circuit = {"some": "circuit"}

    with patch.object(qpu_mod, "QJob") as QJobMock:
        qjob_instance = Mock(name="QJobInstance")
        qjob_instance.submit.side_effect = RuntimeError("boom")
        QJobMock.return_value = qjob_instance

        with pytest.raises(RuntimeError, match="boom"):
            qpu.execute(circuit)

    QJobMock.assert_called_once()          # se intentó crear
    qjob_instance.submit.assert_called_once_with()  # y se intentó enviar


def test_execute_logs_error_when_exception(qpu):
    circuit = {"some": "circuit"}

    with patch.object(qpu_mod, "QJob") as QJobMock, patch.object(qpu_mod, "logger") as logger_mock:
        qjob_instance = Mock()
        qjob_instance.submit.side_effect = ValueError("bad")
        QJobMock.return_value = qjob_instance

        with pytest.raises(ValueError):
            qpu.execute(circuit)
            
        assert logger_mock.error.call_count == 1



