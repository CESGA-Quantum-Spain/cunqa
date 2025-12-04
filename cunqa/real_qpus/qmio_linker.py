import sys, os
from pathlib import Path

current_file_dir = Path(__file__).resolve().parent
parent_dir = current_file_dir.parent.parent
sys.path.append(str(parent_dir))

from cunqa.constants import QPUS_FILEPATH, LIBS_DIR
from cunqa.qclient import write_on_file
from cunqa.logger import logger

sys.path.append(LIBS_DIR)

import zmq
import json
import psutil
import socket
from typing import Optional


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
    

def start_linker_server(family : str) -> None:
    logger.debug("Starting QMIO linker...")

    ZMQ_ENDPOINT = os.getenv("ZMQ_SERVER") 
    PREFERRED_NETWORK_IFACE = "ib"

    linker_context = zmq.Context()
    client_comm_socket = linker_context.socket(zmq.REP)

    ip = _get_IP(preferred_net_iface = PREFERRED_NETWORK_IFACE)
    port = client_comm_socket.bind_to_random_port(f"tcp://{ip}")

    qmio_comm_socket = linker_context.socket(zmq.REQ)
    qmio_comm_socket.connect(ZMQ_ENDPOINT)

    linker_endpoint = f"tcp://{ip}:{port}"
    qmio_config = _get_qmio_config(family, linker_endpoint)
    write_on_file(qmio_config, QPUS_FILEPATH, family)

    waiting = True
    while waiting:
        try:
            circuit = client_comm_socket.recv_pyobj()
            qmio_comm_socket.send_pyobj(circuit)
            results = qmio_comm_socket.recv_pyobj()
            client_comm_socket.send_pyobj(results)

        except zmq.ZMQError as e:
            waiting = False
            client_comm_socket.close()
            qmio_comm_socket.close()
            linker_context.term()
            sys.exit(f"ZMQError: {e}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        logger.error("No family name provided to QMIO linker")
        sys.exit("No family name provided to QMIO linker")

    start_linker_server(sys.argv[1])
