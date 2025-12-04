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
import random


def _get_available_port() -> str: #TODO: Check availability
    return str(random.randint(49152, 65535))

def start_linker_server(frontend_endpoint : str) -> None:
    logger.debug("Starting QMIO linker server...")
    ZMQ_ENDPOINT = os.getenv("ZMQ_SERVER") 

    linker_context = zmq.Context()

    client_comm_socket = linker_context.socket(zmq.REP)
    client_comm_socket.bind(frontend_endpoint)

    qmio_comm_socket = linker_context.socket(zmq.REQ)
    qmio_comm_socket.connect(ZMQ_ENDPOINT)


    waiting = True
    while waiting:
        try:
            circuit = client_comm_socket.recv_pyobj()
            qmio_comm_socket.send_pyobj(circuit)
            results = qmio_comm_socket.recv_pyobj()
            client_comm_socket.send_pyobj(results)

        except zmq.ZMQError as e:
            client_comm_socket.close()
            qmio_comm_socket.close()
            linker_context.term()
            sys.exit(f"ZMQError: {e}")


if __name__ == "__main__":
    print("Inside QMIO linker")
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

    linker_endpoint = "tcp://10.5.7.23:" + _get_available_port()

    qmio_config = {
        "real_qpu":"QMIO",
        "backend":qmio_backend_config,
        "net":{
            "endpoint":linker_endpoint,
            "nodename":"qmio_node",
            "mode":"co_located"
        },
        "family":"real_qmio",
        "name":"QMIO"
    }

    str_qmio_config = json.dumps(qmio_config)
    
    write_on_file(str_qmio_config, QPUS_FILEPATH)
    start_linker_server(linker_endpoint)
