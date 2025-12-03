import sys, os
from pathlib import Path

current_file_dir = Path(__file__).resolve().parent
parent_dir = current_file_dir.parent.parent
sys.path.append(str(parent_dir))

from cunqa.constants import QPUS_FILEPATH, LIBS_DIR
from cunqa.qclient import write_on_file
from cunqa.circuit import CunqaCircuit
from cunqa.circuit import convert
from cunqa.logger import logger

sys.path.append(LIBS_DIR)

import zmq
import json
import random
import time
from typing import Optional, Union, Any

from qiskit import QuantumCircuit # Only for typing


def _optimization_options_builder(
    optimization: int, optimization_backend: str = "Tket"
) -> int:
    """
    Builds the optimization options for the backend.

    This helper function ensures that the optimization is not dependent on QAT.
    Currently, only Tket optimization is supported.

    Parameters
    ----------
    optimization : int
        The optimization level to use.
    optimization_backend : str, default="Tket"
        The optimization backend to use. Currently, only Tket is supported.

    Returns
    -------
    int
        The optimization value understood by the control server.

    Raises:
    -------
        TypeError
            If asked for a not valid optimization backend
        ValueError
            If asked for a not valid optimization level

    """
    start = time.time_ns()
    if optimization_backend != "Tket":
        raise TypeError(f"{optimization_backend}: Not a valid type")
    if optimization == 1:
        opt_value = 18
    elif optimization == 2:
        opt_value = 30
    elif optimization == 0:
        opt_value = 1
    else:
        raise ValueError(f"{optimization}: Not a valid Optimization Value")
    end = time.time_ns()
    return opt_value


def _results_format_builder(res_format: str = "binary_count") -> tuple[int, int]:
    """
    Builds the results format without using QAT.

    This function returns the InlineResultsProcessing and ResultFormatting integers
    to be used as input for the configuration builder.

    Parameters
    ----------
    res_format : str, default="binary_count"
        The format in which the results will be processed.
        Possible values are:

        - "binary_count": Returns a count of each instance of measured qubit registers. Switches result format to raw.
        - "binary": Returns results as a binary string.
        - "raw": Returns raw results.
        - "squash_binary_result_arrays": Squashes binary result list into a singular bit string. Switches results to binary.

    Returns
    -------
    tuple of int
        A tuple containing two integers: InlineResultsProcessing and ResultsFormatting.

    Raises
    ------
    KeyError
        If the provided `res_format` is not a valid result format.
    """
    match = {
        "binary_count": {"InlineResultsProcessing": 1, "ResultsFormatting": 3},
        "raw": {"InlineResultsProcessing": 1, "ResultsFormatting": 2},
        "binary": {"InlineResultsProcessing": 2, "ResultsFormatting": 2},
        "squash_binary_result_arrays": {"InlineResultsProcessing": 2, "ResultsFormatting": 6}
    }
    if res_format not in match.keys():
        raise KeyError(f"{res_format}: Not a valid result format")
    return match[res_format]["InlineResultsProcessing"], match[res_format]["ResultsFormatting"]


def _config_builder(
    shots: int,
    repetition_period: Optional[float] = None,
    optimization: int = 0,
    res_format: str = "binary_count",
) -> str:
    """
    Builds a config json object from options. Non qat-dependent

    Args:
        shots: int : Number of shots
        repetition_period: float : Duration of the circuit execution window.
            Include relaxation time
        optimization: int : 0, 1, 2. Optimization level defined and processed
            in server side
        res_format: str : binary_count(default), raw, binary,
            squash_binary_arrays. Result formatting defined and applied in
            server side.
    Returns:
        config_str: str : json-string object that is sent and loaded in the
            server side
    """
    inlineResultsProcessing, resultsFormatting = _results_format_builder(res_format)
    opt_value = _optimization_options_builder(optimization=optimization)
    start = time.time_ns()
    config = {
        "$type": "<class 'qat.purr.compiler.config.CompilerConfig'>",
        "$data": {
            "repeats": shots,
            "repetition_period": repetition_period,
            "results_format": {
                "$type": "<class 'qat.purr.compiler.config.QuantumResultsFormat'>",
                "$data": {
                    "format": {
                        "$type": "<enum 'qat.purr.compiler.config.InlineResultsProcessing'>",
                        "$value": inlineResultsProcessing,
                    },
                    "transforms": {
                        "$type": "<enum 'qat.purr.compiler.config.ResultsFormatting'>",
                        "$value": resultsFormatting,
                    },
                },
            },
            "metrics": {
                "$type": "<enum 'qat.purr.compiler.config.MetricsType'>",
                "$value": 6,
            },
            "active_calibrations": [],
            "optimizations": {
                "$type": "<enum 'qat.purr.compiler.config.TketOptimizations'>",
                "$value": opt_value,
            },
        },
    }
    config_str = json.dumps(config)
    end = time.time_ns()
    return config_str


def _get_available_port() -> str: #TODO: Check availability
    return str(random.randint(49152, 65535))


def run_on_qmio(linker_endpoint : str, circuit: Union[dict, 'CunqaCircuit', 'QuantumCircuit'], **run_parameters: Any) -> dict:
    external_context = zmq.Context()
    client = external_context.socket(zmq.REQ) 
    client.connect(linker_endpoint)

    config = _config_builder(100) # TODO: run_parameters

    qasm_circuit = convert(circuit, convert_to = "qasm", qasm_version = "3.0")
    data_to_send = (qasm_circuit, config)

    try:
        client.send_pyobj(data_to_send)
        result = client.recv_pyobj()

        print("Received results: {}", result)

        return result

    except zmq.ZMQError as e:
        return {"ZMQError":f"{e}"}


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
