#import zmq
import os, sys
import random
import json, pickle, base64
import time

from typing import Optional

HOME = os.getenv("HOME")
STORE = os.getenv("STORE")
sys.path.append(HOME)

from cunqa.logger import logger
    
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

def make_json_serializable(obj):
    if isinstance(obj, bytes):
        return {"__bytes__": base64.b64encode(obj).decode('ascii')}
    elif isinstance(obj, dict):
        return {k: make_json_serializable(v) for k, v in obj.items()}
    elif isinstance(obj, list):
        return [make_json_serializable(v) for v in obj]
    else:
        return obj

def seriaize_and_write(deserialized_circuit_filepath, serialized_results_filepath):
    with open(serialized_results_filepath, 'rb') as f:
        message = f.read()

    serialized_message = pickle.dumps(message)

    with open(serialized_results_filepath, 'wb') as f:
        f.write(serialized_message)


def deserialize_and_write(serialized_results_filepath, deserialized_results_filepath):
    with open(serialized_results_filepath, 'rb') as sf:
        results = pickle.load(sf)

    print(results)

    with open(deserialized_results_filepath, 'w') as df:
        df.write(json.dumps(results))

if __name__ == "__main__":
    deserialized_circuit_filepath = STORE + "/.cunqa/deserialized_circuit.bin"
    serialized_circuit_filepath = STORE + "/.cunqa/serialized_circuit.bin"
    serialized_results_filepath = STORE + "/.cunqa/serialized_results.bin"
    deserialized_results_filepath = STORE + "/.cunqa/deserialized_results.bin"

    ser_or_deser = sys.argv[1]
    logger.info(f"Serialize or not?: {ser_or_deser}")
    if ser_or_deser == "serialize":
        seriaize_and_write(deserialized_circuit_filepath, serialized_results_filepath)
    elif ser_or_deser == "deserialize":
        deserialize_and_write(serialized_results_filepath, deserialized_results_filepath)
    
    
    