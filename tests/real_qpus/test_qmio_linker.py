import os, sys
import pickle
from unittest.mock import Mock

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

import pytest
import cunqa.real_qpus.qmio_linker as qmio_linked_mod

os.environ.setdefault("ZMQ_SERVER", "tcp://127.0.0.1:5555")

def test_get_qmio_config_contains_family_endpoint_and_backend():
    cfg = qmio_linked_mod._get_qmio_config("famA", "tcp://1.2.3.4:7777")
    assert isinstance(cfg, str)
    assert '"family": "famA"' in cfg
    assert '"endpoint": "tcp://1.2.3.4:7777"' in cfg


def test_upgrade_parameters_updates_only_rz_instructions():
    qt = (
        {"instructions": [
            {"name": "x", "params": []},
            {"name": "rz", "params": [0.0]},
            {"name": "rz", "params": [0.0]},
        ]},
        {"meta": 1},
    )
    params = [0.12, 3.14]
    out = qmio_linked_mod._upgrade_parameters(qt, params)

    # Mutates in place and returns the same tuple object
    assert out is qt
    assert qt[0]["instructions"][0]["params"] == []
    assert qt[0]["instructions"][1]["params"] == [0.12]
    assert qt[0]["instructions"][2]["params"] == [3.14]


def test_list_interfaces_filters_ipv4_only(monkeypatch):
    inet = Mock(family=socket.AF_INET, address="10.0.0.2")
    inet6 = Mock(family=object(), address="fe80::1")

    net_if_addrs = Mock(return_value={"eth0": [inet, inet6], "lo": []})
    monkeypatch.setattr(qmio_linked_mod.psutil, "net_if_addrs", net_if_addrs)

    out = qmio_linked_mod._list_interfaces(ipv4_only=True)
    assert out == {"eth0": ["10.0.0.2"]}

    out2 = qmio_linked_mod._list_interfaces(ipv4_only=False)
    assert out2["eth0"] == ["10.0.0.2", "fe80::1"]


def test_get_ip_prefers_named_iface_prefix(monkeypatch):
    monkeypatch.setattr(
        qmio_linked_mod,
        "_list_interfaces",
        Mock(return_value={"ib0": ["192.168.1.10"], "eth0": ["10.0.0.2"]}),
    )
    ip = qmio_linked_mod._get_IP(preferred_net_iface="ib")
    assert ip == "192.168.1.10"


def test_get_ip_falls_back_to_first_iface(monkeypatch):
    monkeypatch.setattr(
        qmio_linked_mod,
        "_list_interfaces",
        Mock(return_value={"eth0": ["10.0.0.2"], "ib0": ["192.168.1.10"]}),
    )
    ip = qmio_linked_mod._get_IP(preferred_net_iface=None)
    assert ip == "10.0.0.2"

def _make_linker_without_init():
    """
    Crea una instancia sin ejecutar __init__ para testear recv_data/compute_result
    sin hilos/sockets reales.
    """
    linker = qmio_linked_mod.QMIOLinker.__new__(qmio_linked_mod.QMIOLinker)
    linker.message_queue = Mock()
    linker.client_ids_queue = Mock()
    linker.client_comm_socket = Mock()
    linker.qmio_comm_socket = Mock()
    linker.context = Mock()
    return linker


def test_init_binds_connects_writes_config_and_starts_threads(monkeypatch):
    # --- sockets/context zmq mockeados ---
    router_socket = Mock()
    router_socket.bind_to_random_port = Mock(return_value=43210)

    req_socket = Mock()

    context = Mock()
    context.socket = Mock(side_effect=[router_socket, req_socket])
    monkeypatch.setattr(qmio_linked_mod.zmq, "Context", Mock(return_value=context))

    # --- dependencias externas del init ---
    monkeypatch.setattr(qmio_linked_mod, "_get_IP", Mock(return_value="10.1.2.3"))
    monkeypatch.setattr(qmio_linked_mod, "_get_qmio_config", Mock(return_value='{"cfg":1}'))
    write_on_file = Mock()
    monkeypatch.setattr(qmio_linked_mod, "write_on_file", write_on_file)

    # IMPORTANTE: no arrancar hilos reales
    thread_instances = []

    def thread_ctor(*, target):
        t = Mock()
        t.start = Mock()
        t._target = target
        thread_instances.append(t)
        return t

    monkeypatch.setattr(qmio_linked_mod.threading, "Thread", thread_ctor)

    # Asegurar endpoint conocido
    monkeypatch.setattr(qmio_linked_mod, "ZMQ_ENDPOINT", "tcp://127.0.0.1:5555")

    linker = qmio_linked_mod.QMIOLinker("famX")

    # Se bind-ea a tcp://<ip> y genera endpoint con puerto devuelto
    router_socket.bind_to_random_port.assert_called_once_with("tcp://10.1.2.3")
    assert linker.ip == "10.1.2.3"
    assert linker.port == 43210
    assert linker.endpoint == "tcp://10.1.2.3:43210"

    # Conecta al ZMQ_ENDPOINT
    req_socket.connect.assert_called_once_with("tcp://127.0.0.1:5555")

    # Escribe config
    write_on_file.assert_called_once_with('{"cfg":1}', qmio_linked_mod.QPUS_FILEPATH, "famX")

    # Crea 2 hilos y los arranca
    assert len(thread_instances) == 2
    assert {thread_instances[0]._target, thread_instances[1]._target} == {
        linker.recv_data,
        linker.compute_result,
    }
    assert thread_instances[0].start.called
    assert thread_instances[1].start.called


def test_recv_data_when_message_is_tuple_converts_enqueues_and_tracks_last_task(monkeypatch):
    linker = _make_linker_without_init()

    msg = ({"ir": "circuit"}, {"shots": 100})
    ser = pickle.dumps(msg)

    # Primera iteración: devuelve 1 mensaje. Segunda: provoca salida por error.
    def recv_side_effect():
        if not hasattr(recv_side_effect, "called"):
            recv_side_effect.called = True
            return [b"CID", ser]
        raise qmio_linked_mod.zmq.ZMQError("stop")

    linker.client_comm_socket.recv_multipart = Mock(side_effect=recv_side_effect)

    ir_to_qasm = Mock(return_value="QASM1")
    monkeypatch.setattr(qmio_linked_mod, "_IR_to_QASM", ir_to_qasm)

    # sys.exit -> SystemExit para cortar el bucle limpiamente en test
    monkeypatch.setattr(qmio_linked_mod.sys, "exit", Mock(side_effect=SystemExit))

    with pytest.raises(SystemExit):
        linker.recv_data()

    # Se guarda el mensaje como last task
    assert linker._last_quantum_task == msg

    # Convierte solo el circuito (message[0])
    ir_to_qasm.assert_called_once_with(msg[0])

    # Encola quantum_task y id
    linker.message_queue.put.assert_called_once_with(("QASM1", {"shots": 100}))
    linker.client_ids_queue.put.assert_called_once_with(b"CID")


def test_recv_data_when_message_is_params_dict_upgrades_uses_last_task_and_enqueues(monkeypatch):
    linker = _make_linker_without_init()

    # last task previo
    last = ({"ir": "prev"}, {"shots": 10})
    linker._last_quantum_task = last

    params_msg = {"params": [1.23, 4.56]}
    ser = pickle.dumps(params_msg)

    def recv_side_effect():
        if not hasattr(recv_side_effect, "called"):
            recv_side_effect.called = True
            return [b"CID2", ser]
        raise qmio_linked_mod.zmq.ZMQError("stop")

    linker.client_comm_socket.recv_multipart = Mock(side_effect=recv_side_effect)

    upgraded = ({"ir": "upgraded"}, {"shots": 10})
    upgrade_parameters = Mock(return_value=upgraded)
    monkeypatch.setattr(qmio_linked_mod, "_upgrade_parameters", upgrade_parameters)

    ir_to_qasm = Mock(return_value="QASM_UP")
    monkeypatch.setattr(qmio_linked_mod, "_IR_to_QASM", ir_to_qasm)

    monkeypatch.setattr(qmio_linked_mod.sys, "exit", Mock(side_effect=SystemExit))

    with pytest.raises(SystemExit):
        linker.recv_data()

    # Llama al upgrade con last_task actual + params
    upgrade_parameters.assert_called_once_with(last, params_msg["params"])

    # Actualiza _last_quantum_task al retorno de upgrade
    assert linker._last_quantum_task == upgraded

    # Convierte el circuito upgraded[0]
    ir_to_qasm.assert_called_once_with(upgraded[0])

    # Encola (qasm, meta) desde upgraded
    linker.message_queue.put.assert_called_once_with(("QASM_UP", upgraded[1]))
    linker.client_ids_queue.put.assert_called_once_with(b"CID2")


def test_recv_data_on_zmq_error_closes_sockets_terms_context_and_exits(monkeypatch):
    linker = _make_linker_without_init()

    linker.client_comm_socket.recv_multipart = Mock(side_effect=qmio_linked_mod.zmq.ZMQError("boom"))

    monkeypatch.setattr(qmio_linked_mod.sys, "exit", Mock(side_effect=SystemExit))

    with pytest.raises(SystemExit):
        linker.recv_data()

    linker.client_comm_socket.close.assert_called_once()
    linker.qmio_comm_socket.close.assert_called_once()
    linker.context.term.assert_called_once()


def test_compute_result_sends_task_receives_result_and_replies(monkeypatch):
    linker = _make_linker_without_init()

    # queue.get devuelve 1 vez; luego forzamos ZMQError en send para salir
    linker.message_queue.get = Mock(return_value=("QASM", {"shots": 5}))
    linker.client_ids_queue.get = Mock(return_value=b"CID3")

    linker.qmio_comm_socket.recv_pyobj = Mock(return_value={"counts": {"0": 1}})

    def send_side_effect(_qt):
        if not hasattr(send_side_effect, "called"):
            send_side_effect.called = True
            return None
        raise qmio_linked_mod.zmq.ZMQError("stop")

    linker.qmio_comm_socket.send_pyobj = Mock(side_effect=send_side_effect)

    monkeypatch.setattr(qmio_linked_mod.sys, "exit", Mock(side_effect=SystemExit))

    with pytest.raises(SystemExit):
        linker.compute_result()

    # Envía quantum_task y recibe resultados
    linker.qmio_comm_socket.send_pyobj.assert_any_call(("QASM", {"shots": 5}))
    linker.qmio_comm_socket.recv_pyobj.assert_called()

    # Responde al cliente con multipart [client_id, pickle(results)]
    assert linker.client_comm_socket.send_multipart.called
    sent = linker.client_comm_socket.send_multipart.call_args[0][0]
    assert sent[0] == b"CID3"
    assert pickle.loads(sent[1]) == {"counts": {"0": 1}}

    # Cleanup al ZMQError
    linker.client_comm_socket.close.assert_called_once()
    linker.qmio_comm_socket.close.assert_called_once()
    linker.context.term.assert_called_once()


def test_compute_result_on_zmq_error_closes_sockets_terms_context_and_exits(monkeypatch):
    linker = _make_linker_without_init()

    linker.message_queue.get = Mock(return_value=("QASM", {"shots": 1}))
    linker.client_ids_queue.get = Mock(return_value=b"CID")

    linker.qmio_comm_socket.send_pyobj = Mock(side_effect=qmio_linked_mod.zmq.ZMQError("boom"))

    monkeypatch.setattr(qmio_linked_mod.sys, "exit", Mock(side_effect=SystemExit))

    with pytest.raises(SystemExit):
        linker.compute_result()

    linker.client_comm_socket.close.assert_called_once()
    linker.qmio_comm_socket.close.assert_called_once()
    linker.context.term.assert_called_once()

