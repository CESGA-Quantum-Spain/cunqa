import sys
from pathlib import Path

current_file_dir = Path(__file__).resolve().parent
parent_dir = current_file_dir.parent.parent
sys.path.append(str(parent_dir))

from cunqa.constants import LIBS_DIR
from cunqa.circuit import convert
from cunqa.logger import logger

sys.path.append(LIBS_DIR)

import zmq
import json
import time
from typing import Optional



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


def get_run_config(run_config : dict) -> str:
    config_build_vars = {
    "shots":1024,
    "repetition_period":None,
    "optimization":0,
    "res_format":"binary_count"
    }
    for key, value in run_config.items():
        if key in config_build_vars:
            config_build_vars[key] = value
            
    config = _config_builder(shots = config_build_vars["shots"], 
                            repetition_period = config_build_vars["repetition_period"], 
                            optimization = config_build_vars["optimization"], 
                            res_format = config_build_vars["res_format"])

    return config

    

class QMIOFuture:
    def __init__(self, socket = None, error = None):
        self.socket = socket
        self.error = error
    
    def valid(self) -> bool:
        return True
    
    def get(self) -> dict:
        if self.socket is not None:
            result = self.socket.recv_pyobj()
            return result
        elif self.error is not None:
            return {f"ERROR":{self.error}}
        else:
            return {"ERROR":"An error occured in QMIO."}


class QMIOClient:
    def __init__(self):
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REQ)

    def connect(self, linker_endpoint : str) -> None:
        self.socket.connect(linker_endpoint)

    def send_circuit(self, quantum_task_str : str) -> 'QMIOFuture':

        quantum_task = json.loads(quantum_task_str)
        circuit = quantum_task["instructions"]
        run_config = quantum_task["config"]

        run_config = get_run_config(run_config)
        qasm_circuit = convert(circuit, convert_to = "qasm", qasm_version = "3.0")
        data_to_send = (qasm_circuit, run_config)

        try:
            self.socket.send_pyobj(data_to_send)
            qmiofuture = QMIOFuture(socket = self.socket) 
            return qmiofuture
        
        except zmq.ZMQError as e:
            self.socket.close()
            self.context.term()
            qmiofuture = QMIOFuture(error = e) 
            return qmiofuture






