import sys, os
from pathlib import Path

sys.path.append(os.getenv("HOME"))

from cunqa.constants import QPUS_FILEPATH, LIBS_DIR
from cunqa.qclient import write_on_file
from cunqa.circuit import convert
from cunqa.logger import logger

sys.path.append(LIBS_DIR)

import zmq
import json
import psutil
import socket
import pickle
import threading
from queue import Queue
from typing import Optional


ZMQ_ENDPOINT = os.getenv("ZMQ_SERVER") 
PREFERRED_NETWORK_IFACE = "ib"


def _get_qmio_config(family : str, endpoint : str) -> str:
    qmio_backend_config = {
        "name":"QMIOBackend",
        "version":"",
        "n_qubits":32,
        "description":"Backend of real QMIO",
        "coupling_map":[[0,1],[2,1],[2,3],[4,3],[5,4],[6,3],[6,12],[7,0],[7,9],[9,10],
                        [11,10],[11,12],[13,21],[14,11],[14,18],[15,8],[15,16],[18,17],
                        [18,19],[20,19],[22,21],[22,31],[23,20],[23,30],[24,17],[24,27],
                        [25,16],[25,26],[26,27],[28,27],[28,29],[30,29],[30,31]],
        "basis_gates":["sx", "x", "rz", "ecr"],
        "noise":"",
    }

    qmio_config_json = {
        "real_qpu":"QMIO",
        "backend":qmio_backend_config,
        "net":{
            "endpoint":endpoint,
            "nodename":"c7-23",
            "mode":"co_located"
        },
        "family":family,
        "name":"QMIO"
    }

    return json.dumps(qmio_config_json)

def _upgrade_parameters(quantum_task : tuple[dict, dict], parameters : list[float]) ->tuple[dict, dict]:
    param_counter = 0
    for inst in quantum_task[0]["instructions"]:
        name = inst["name"]
        match(name):
            case "rz":
                inst["params"] = [parameters[param_counter]]
                param_counter += 1

    return quantum_task

def _list_interfaces(ipv4_only=True):
    interfaces = {}
    for iface_name, addrs in psutil.net_if_addrs().items():
        iface_ips = []
        for addr in addrs:
            if ipv4_only and addr.family == socket.AF_INET:
                iface_ips.append(addr.address)
            elif not ipv4_only:
                iface_ips.append(addr.address)
        if iface_ips:
            interfaces[iface_name] = iface_ips
    return interfaces


def _get_IP(preferred_net_iface : Optional[str] = None) -> str:
    all_ifaces = _list_interfaces()
    if preferred_net_iface != None:
        ifaces = {name: ips for name, ips in all_ifaces.items() if name.startswith(preferred_net_iface)}
        return all_ifaces[next(iter(ifaces))][0]
    else:
        for name, ips in all_ifaces.items():
            return ips[0]
    


class QMIOLinker:

    message_queue : 'Queue'
    client_ids_queue : 'Queue'
    context : 'zmq.Context'
    client_comm_socket : 'zmq.ROUTER'
    qmio_comm_socket : 'zmq.REQ'
    ip : str
    port : str
    endpoint : str
    _last_quantum_task : tuple[dict, dict]

    def __init__(self, family : str):
        self.message_queue = Queue()
        self.client_ids_queue = Queue()

        self.context = zmq.Context()
        self.client_comm_socket = self.context.socket(zmq.ROUTER)

        self.ip = _get_IP(preferred_net_iface = PREFERRED_NETWORK_IFACE)
        self.port = self.client_comm_socket.bind_to_random_port(f"tcp://{self.ip}")
        self.endpoint = f"tcp://{self.ip}:{self.port}"

        self.qmio_comm_socket = self.context.socket(zmq.REQ)
        self.qmio_comm_socket.connect(ZMQ_ENDPOINT)

        qmio_config = _get_qmio_config(family, self.endpoint)
        write_on_file(qmio_config, QPUS_FILEPATH, family)

        recv_thread = threading.Thread(target = self.recv_data)
        compute_thread = threading.Thread(target = self.compute_result)

        recv_thread.start()
        compute_thread.start()

    
    def recv_data(self) -> None:
        logger.debug("QMIO linker starts listening...")

        waiting = True
        while waiting:
            try:
                id, ser_message = self.client_comm_socket.recv_multipart()
                message = pickle.loads(ser_message)
                if isinstance(message, dict) and ("params" in message):
                    self._last_quantum_task = _upgrade_parameters(self._last_quantum_task, message["params"])
                    upgraded_qasm_circ = convert(self._last_quantum_task[0], convert_to = "qasm")
                    quantum_task = (upgraded_qasm_circ, self._last_quantum_task[1])
                else:
                    self._last_quantum_task = message 
                    qasm_circuit = convert(message[0], convert_to = "qasm")
                    quantum_task = (qasm_circuit, message[1])
                    
                self.message_queue.put(quantum_task)
                self.client_ids_queue.put(id)
            except zmq.ZMQError as e:
                waiting = False
                self.client_comm_socket.close()
                self.qmio_comm_socket.close()
                self.context.term()
                sys.exit(f"Error receiving data in QMIOLinker: {e}")

    def compute_result(self) -> None:
        checking_queue = True
        while checking_queue:
            try:
                quantum_task = self.message_queue.get()
                client_id = self.client_ids_queue.get()
                self.qmio_comm_socket.send_pyobj(quantum_task)
                results = self.qmio_comm_socket.recv_pyobj()
                ser_results = pickle.dumps(results)
                self.client_comm_socket.send_multipart([client_id, ser_results])
            except zmq.ZMQError as e:
                checking_queue = False
                self.client_comm_socket.close()
                self.qmio_comm_socket.close()
                self.context.term()
                sys.exit(f"Error computing result in QMIOLinker: {e}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        logger.error("No family name provided to QMIO linker")
        sys.exit("No family name provided to QMIO linker")

    qmiolinker = QMIOLinker(sys.argv[1])
