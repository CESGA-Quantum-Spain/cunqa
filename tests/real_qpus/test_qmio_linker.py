import os, sys
import pickle
import socket
import json

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

# -----------------------
# Helpers
# -----------------------

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

    assert out is qt
    assert qt[0]["instructions"][0]["params"] == []
    assert qt[0]["instructions"][1]["params"] == [0.12]
    assert qt[0]["instructions"][2]["params"] == [3.14]


def test_list_interfaces_filters_ipv4_only(monkeypatch):
    inet = Mock(family=socket.AF_INET, address="10.0.0.2")
    inet6 = Mock(family=socket.AF_INET6, address="fe80::1")

    net_if_addrs = Mock(return_value={"eth0": [inet, inet6], "lo": []})
    monkeypatch.setattr(qmio_linked_mod.psutil, "net_if_addrs", net_if_addrs)

    out = qmio_linked_mod._list_interfaces(ipv4_only=True)
    assert out == {"eth0": ["10.0.0.2"]}
    out2 = qmio_linked_mod._list_interfaces(ipv4_only=False)
    assert out2 == {"eth0": ["10.0.0.2", "fe80::1"]}


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

# -----------------------
# QMIOLinker
# -----------------------

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


def test_init_binds_connects_writes_config(monkeypatch):
    router_socket = Mock()
    router_socket.bind_to_random_port = Mock(return_value=43210)

    req_socket = Mock()

    context = Mock()
    context.socket = Mock(side_effect=[router_socket, req_socket])
    monkeypatch.setattr(qmio_linked_mod.zmq, "Context", Mock(return_value=context))

    monkeypatch.setattr(qmio_linked_mod, "_get_IP", Mock(return_value="10.1.2.3"))
    monkeypatch.setattr(qmio_linked_mod, "_get_qmio_config", Mock(return_value='{"cfg":1}'))
    write_on_file = Mock()
    monkeypatch.setattr(qmio_linked_mod, "write_on_file", write_on_file)
    monkeypatch.setattr(qmio_linked_mod, "ZMQ_ENDPOINT", "tcp://127.0.0.1:5555")

    linker = qmio_linked_mod.QMIOLinker("famX")

    router_socket.bind_to_random_port.assert_called_once_with("tcp://10.1.2.3")
    assert linker.ip == "10.1.2.3"
    assert linker.port == 43210
    assert linker.endpoint == "tcp://10.1.2.3:43210"
    req_socket.connect.assert_called_once_with("tcp://127.0.0.1:5555")
    write_on_file.assert_called_once_with('{"cfg":1}', qmio_linked_mod.QPUS_FILEPATH, "famX")


def test_run_starts_two_threads(monkeypatch):
    linker = _make_linker_without_init()

    thread_instances = []

    def thread_ctor(*, target):
        t = Mock()
        t.start = Mock()
        t._target = target
        thread_instances.append(t)
        return t

    monkeypatch.setattr(qmio_linked_mod.threading, "Thread", thread_ctor)

    linker.run()

    assert len(thread_instances) == 2
    assert thread_instances[0]._target == linker.recv_data
    assert thread_instances[1]._target == linker.compute_result
    assert thread_instances[0].start.called
    assert thread_instances[1].start.called


def test_recv_data_when_message_is_tuple_converts_enqueues_and_tracks_last_task(monkeypatch):
    linker = _make_linker_without_init()

    msg = ({"ir": "circuit"}, {"shots": 100})
    ser = pickle.dumps(msg)

    def recv_side_effect():
        if not hasattr(recv_side_effect, "called"):
            recv_side_effect.called = True
            return [b"CID", ser]
        raise StopIteration()

    linker.client_comm_socket.recv_multipart = Mock(side_effect=recv_side_effect)

    json_to_qasm2 = Mock(return_value="QASM1")
    monkeypatch.setattr(qmio_linked_mod, "json_to_qasm2", json_to_qasm2)

    with pytest.raises(StopIteration):
        linker.recv_data()

    assert linker._last_quantum_task == msg
    json_to_qasm2.assert_called_once()
    called_arg = json_to_qasm2.call_args[0][0]
    assert called_arg == json.dumps(msg[0])
    linker.message_queue.put.assert_called_once_with(("QASM1", {"shots": 100}))
    linker.client_ids_queue.put.assert_called_once_with(b"CID")


def test_recv_data_when_message_is_params_dict_upgrades_uses_last_task_and_enqueues(monkeypatch):
    linker = _make_linker_without_init()

    last = ({"ir": "prev"}, {"shots": 10})
    linker._last_quantum_task = last

    params_msg = {"params": [1.23, 4.56]}
    ser = pickle.dumps(params_msg)

    def recv_side_effect():
        if not hasattr(recv_side_effect, "called"):
            recv_side_effect.called = True
            return [b"CID2", ser]
        raise StopIteration()

    linker.client_comm_socket.recv_multipart = Mock(side_effect=recv_side_effect)

    upgraded = ({"ir": "upgraded"}, {"shots": 10})
    upgrade_parameters = Mock(return_value=upgraded)
    monkeypatch.setattr(qmio_linked_mod, "_upgrade_parameters", upgrade_parameters)

    json_to_qasm2 = Mock(return_value="QASM_UP")
    monkeypatch.setattr(qmio_linked_mod, "json_to_qasm2", json_to_qasm2)

    with pytest.raises(StopIteration):
        linker.recv_data()

    upgrade_parameters.assert_called_once_with(last, params_msg["params"])
    assert linker._last_quantum_task == upgraded
    json_to_qasm2.assert_called_once()
    called_arg = json_to_qasm2.call_args[0][0]
    assert called_arg == json.dumps(upgraded[0])
    linker.message_queue.put.assert_called_once_with(("QASM_UP", upgraded[1]))
    linker.client_ids_queue.put.assert_called_once_with(b"CID2")


def test_recv_data_on_zmq_error_closes_sockets_terms_context_and_raises_valueerror(monkeypatch):
    linker = _make_linker_without_init()

    linker.client_comm_socket.recv_multipart = Mock(
        side_effect=qmio_linked_mod.zmq.ZMQError("boom")
    )

    with pytest.raises(ValueError) as e:
        linker.recv_data()

    assert "Error receiving data in QMIOLinker" in str(e.value)
    linker.client_comm_socket.close.assert_called_once()
    linker.qmio_comm_socket.close.assert_called_once()
    linker.context.term.assert_called_once()


def test_compute_result_sends_task_receives_result_and_replies(monkeypatch):
    linker = _make_linker_without_init()
    linker.message_queue.get = Mock(side_effect=[("QASM", {"shots": 5}), StopIteration()])
    linker.client_ids_queue.get = Mock(return_value=b"CID3")
    linker.qmio_comm_socket.recv_pyobj = Mock(return_value={"counts": {"0": 1}})
    linker.qmio_comm_socket.send_pyobj = Mock(return_value=None)

    with pytest.raises(StopIteration):
        linker.compute_result()

    linker.qmio_comm_socket.send_pyobj.assert_called_once_with(("QASM", {"shots": 5}))
    linker.qmio_comm_socket.recv_pyobj.assert_called_once()

    linker.client_comm_socket.send_multipart.assert_called_once()
    sent = linker.client_comm_socket.send_multipart.call_args[0][0]
    assert sent[0] == b"CID3"
    assert pickle.loads(sent[1]) == {"counts": {"0": 1}}


def test_compute_result_on_zmq_error_closes_sockets_terms_context_and_raises_runtimeerror(monkeypatch):
    linker = _make_linker_without_init()

    linker.message_queue.get = Mock(return_value=("QASM", {"shots": 1}))
    linker.client_ids_queue.get = Mock(return_value=b"CID")
    linker.qmio_comm_socket.send_pyobj = Mock(side_effect=qmio_linked_mod.zmq.ZMQError("boom"))

    with pytest.raises(RuntimeError) as e:
        linker.compute_result()

    assert "Error computing result in QMIOLinker" in str(e.value)
    linker.client_comm_socket.close.assert_called_once()
    linker.qmio_comm_socket.close.assert_called_once()
    linker.context.term.assert_called_once()
