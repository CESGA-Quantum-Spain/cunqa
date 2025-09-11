#import zmq
import os, sys
import random
import json, pickle
import time

from typing import Optional

sys.path.append(os.getenv("HOME"))

from cunqa.logger import logger

""" def deploy_intermediary_server():
    ZMQ_SERVER = os.getenv('ZMQ_SERVER') 

    if len(sys.argv) > 1:
        intermediary_endpoint = sys.argv[1]
    else:
        logger.error("Intermediary endpoint not provided.")

    qpu_context = zmq.Context()
    qpu_client = qpu_context.socket(zmq.REQ)  
    qpu_client.connect(ZMQ_SERVER)

    intermediary_context = zmq.Context()
    intermediary_server = intermediary_context.socket(zmq.ROUTER)  
    intermediary_server.bind(intermediary_endpoint)

    waiting = True
    while waiting:
        logger.debug("Waiting for a circuit to be executed in the QPU...")
        message = intermediary_server.recv_multipart() # circuit = (instructions, config)
        logger.debug("Circuit received for the intermediary server on the QPU node")
        client_address, serialized_circuit = message
        circuit = pickle.loads(serialized_circuit)
        start_time = time.perf_counter()
        qpu_client.send_pyobj(circuit)
        logger.debug("Circuit sent to QPU. Waiting for results...")
        result = qpu_client.recv_pyobj()
        end_time = time.perf_counter()
        execution_time = end_time - start_time
        logger.debug(f"Result: {result}")
        counts = json.loads(json.dumps(result))["results"]["reg_measure"]
        final_result = {
            "counts":counts,
            "time_taken":execution_time
        }
        serialized_result = pickle.dumps(final_result)
        intermediary_server.send_multipart([client_address, serialized_result])

    logger.debug("Everything was OK. Closing sockets...")
    qpu_client.close()
    qpu_context.term()

    intermediary_server.close()
    intermediary_context.term() """
    
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


""" if __name__ == "__main__":
    deploy_intermediary_server() """