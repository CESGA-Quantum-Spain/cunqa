import os
import sys
import glob
import json

sys.path.append(os.getenv("HOME"))
from cunqa.logger import logger
from cunqa.backend import Backend

from qiskit.providers import QubitProperties, BackendV2, Options
from qiskit_aer import AerSimulator
from qiskit.circuit.library import Measure, UnitaryGate
from qiskit.transpiler import Target, InstructionProperties, TranspilerError
from qiskit.circuit import Parameter


class CunqaBackend(BackendV2):

    def __init__(self, noise_properties_json = None, backend = None, **kwargs):

        if noise_properties_json is not None:

            super().__init__(self,name="cunqa", description="backend for cunqa", **kwargs)

            self._from_noise_properties(noise_properties_json)

        elif isinstance(backend, Backend):

                # instanciating base BackendV2
                super().__init__(self,
                                 name = backend.name,
                                 description = backend.description)

                # non-ideal backend, we get to gather noise_properties
                if backend.noise_properties_path and backend.noise_properties_path.strip():

                    with open(backend.noise_properties_path, "r") as file:
                        noise_properties_json = json.load(file)

                    self._from_noise_properties(noise_properties_json)
                    
                # ideal backend, we do not gather any noise_properties, so ideal target is built.
                else:

                    self._from_backend_json(backend_json=backend.__dict__)


    def _from_noise_properties(self, noise_properties_json):

        # loading qubit noise_properties and readout errors
        qubits= noise_properties_json["Qubits"]

        self._num_qubits = len(qubits)
        
        readout_errors = {}
        qubits_properties = []
        for k,q in qubits.items():
            # TODO: check if key is the correct format q[i]
            qubits_properties.append(QubitProperties(t1=q["T1 (s)"],t2=q["T2 (s)"],frequency=q["Drive Frequency (Hz)"]))
            readout_errors[(_get_qubit_index(k),)] = InstructionProperties(duration=q["Readout duration (s)"], error = 1-q["Readout fidelity (RB)"])
        
        logger.debug(f"{self._num_qubits} qubits properties loaded from noise_properties_json.")


        # creating target
        target = Target(num_qubits=self._num_qubits, qubit_properties=qubits_properties)

        logger.debug(f"Target created for {self._num_qubits} qubits.")


        # adding readout errors to target
        target.add_instruction(Measure(),readout_errors)

        logger.debug(f"Readout errors added for {len(readout_errors)} qubits.")


        # loading single-qubit-gate errors
        single_qubit_gates = {}

        for qubit,gates_dict in noise_properties_json["Q1Gates"].items():
            for gate in gates_dict.keys():
                try:
                    single_qubit_gates[gate]=(_get_gate(gate),{})
                except ValueError:
                    logger.warning(f"Gate {gate} is not supported by Aer Simulator.")
                    # because gate will be ignored, we delete it from noise_properties_json
                    gates_dict.pop(gate)

        logger.debug(f"{len(single_qubit_gates)} single qubit gates where found: {single_qubit_gates}")


        for qubit,gates_dict in noise_properties_json["Q1Gates"].items():
            for gate, gate_properties in gates_dict.items():
                try:
                    single_qubit_gates[gate][1][(_get_qubit_index(qubit),)]  = InstructionProperties(duration = gate_properties["Gate duration (s)"], error = 1- gate_properties["Fidelity(RB)"] )

                except ValueError as error:
                    logger.warning(f"Qubit {qubit} does not have the right sintax [{type(error).__name__}].")
                    logger.warning("Instruction will be ignored.")
                    # because gate will be ignored, we delete it from noise_properties_json and single_qubit_gates dict
                    gates_dict.pop(gate)
                    single_qubit_gates.pop(gate)

                except Exception as error:
                    logger.error(f"Some error occured while adding instruction for gate {gate} in qubit {qubit[2:-1]}: {error} [{type(error).__name__}].")
                    raise SystemExit # User's level

        
        # adding single qubit gates errors to target
        for gate, instruction in single_qubit_gates.items():
            try:
                target.add_instruction(*instruction)

            except TranspilerError as error:
                logger.warning(f"Error adding instructions for gate {gate}: {error.message}.")
                logger.warning("Instruction will be ignored.")

        logger.debug("Added single qubit gates instructions to Target:")
        #logger.debug(f"{single_qubit_gates}")

        
        # loading two-qubit-gate errors
        two_qubit_gates = {}

        for qubits,gates_dict in noise_properties_json["Q2Gates(RB)"].items():
            for gate in gates_dict.keys():
                try:
                    two_qubit_gates[gate]=(_get_gate(gate),{})
                except ValueError:
                    logger.warning(f"Gate {gate} is not supported by Aer Simulator.")
                    # because gate will be ignored, we delete it from noise_properties_json
                    gates_dict.pop(gate)

        logger.debug(f"{len(two_qubit_gates)} two qubit gates where found: {two_qubit_gates}")


        for qubits,gates_dict in noise_properties_json["Q2Gates(RB)"].items():
            for gate, gate_properties in gates_dict.items():
                try:
                    if _get_qubits_indexes(qubits) != [gate_properties["Control"],gate_properties["Target"]]:
                        logger.warning(f"Inconsistency in control and target qubits for gate {gate}({_get_qubits_indexes(qubits)}!={[gate_properties['Control'],gate_properties['Target']]}), instruction will be added for qubits {[gate_properties['Control'],gate_properties['Target']]}.")

                    two_qubit_gates[gate][1][(gate_properties["Control"],gate_properties["Target"],)]  = InstructionProperties(duration = gate_properties["Duration (s)"], error = 1- gate_properties["Fidelity(RB)"] )

                except ValueError as error:
                    logger.warning(f"Qubits {qubits} do not have the right sintax [{type(error).__name__}].")
                    logger.warning("Instruction will be ignored.")
                    # because gate will be ignored, we delete it from noise_properties_json and single_qubit_gates dict
                    gates_dict.pop(gate)
                    two_qubit_gates.pop(gate)

                except Exception as error:
                    logger.error(f"Some error occured while adding instruction for gate {gate} in qubit {_get_qubits_indexes(qubits)}: {error}.")
                    raise SystemExit # User's level


        # adding two qubit gates error to target
        for gate, instruction in two_qubit_gates.items():
            try:
                target.add_instruction(*instruction)

            except TranspilerError as error:
                logger.warning(f"Error adding instructions for gate {gate}: {error.message}.")
                logger.warning("Instruction will be ignored.")
                # because gate will be ignored, we delete it from noise_properties_json
        
        logger.debug("Added two qubit gates instructions to Target:")
        logger.debug(f"{two_qubit_gates}")

        self._target = target


    def _from_backend_json(self, backend_json):
        
        if backend_json["name"] in ["SimpleBackend" , "CCBackend", "QCBackend"]:

            target =  AerSimulator().target
        
        else:

            target = Target(num_qubits = backend_json["n_qubits"])

            if len(backend_json["coupling_map"]) == 0:
                coupling_map = []
                for i in range(backend_json["n_qubits"]):
                    for j in range(backend_json["n_qubits"]):
                        if i != j:
                            coupling_map.append([i,j])
                        
                        for k in range(backend_json["n_qubits"]):
                            if k != j and k != i:
                                coupling_map.append([i,j,k])
            else:
                coupling_map = backend_json["coupling_map"]

            target.add_instruction(Measure(), { (q,): None for q in range(backend_json["n_qubits"]) })

            for gate in backend_json["basis_gates"]:

                if gate == "measure":
                    continue

                elif gate == "unitary":

                    # target.add_instruction(UnitaryGate([[1,0],[0,1]]),{None:None})
                    target.add_instruction(UnitaryGate([[1,0],[0,1]]),{None:None})
                
                else:
                    gate_object = _get_gate(gate)

                    if gate_object.num_qubits > 1:

                        inst_map = { tuple(couple): None for couple in coupling_map if len(couple) == gate_object.num_qubits}

                    else:
                        inst_map = { (q,): None for q in range(backend_json["n_qubits"]) }

                    target.add_instruction(gate_object, inst_map)

        self._target = target


    @classmethod
    def _default_options(cls):
        return Options()

    def max_circuits(self):
        # Return the maximum number of circuits the backend can handle
        return None  # Replace with an appropriate value if needed

    @property
    def target(self):
        # Return the target object for the backend
        return self._target
    
    @property
    def coupling_map_list(self):
        return list(self.coupling_map)
    
    @property
    def basis_gates(self):
        return [gate for gate in self.target._gate_map.keys() if gate != "measure"]

    def run(self, run_input, **kwargs):
        # Implement the logic to execute a quantum circuit or schedule
        raise NotImplementedError("The 'run' method must be implemented.")



import re
def _get_qubit_index(qubit_str):

    match = re.match(r"q\[(\d+)\]", qubit_str)
    if match:
        return int(match.group(1))
    else:
        logger.error(f"Invalid qubit string format: {qubit_str}.")
        raise ValueError # I capture this error at CunqaBackend

def _get_qubits_indexes(qubits_str):
    match = re.match(r"(\d+)-(\d+)", qubits_str)
    if match:
        return [int(match.group(1)), int(match.group(2))]
    else:
        logger.error(f"Invalid qubit string format: {qubits_str}.")
        raise ValueError # I capture this error at CunqaBackend



from qiskit.circuit.library import (U1Gate, U2Gate, U3Gate, CU1Gate, CU3Gate, UGate, CUGate, PhaseGate, RGate, RXGate, RYGate, RZGate, ECRGate,
                                    CRXGate, CRYGate, CRZGate, IGate, XGate, YGate, ZGate, HGate, SGate, SdgGate, SXGate, SXdgGate, TGate, TdgGate,
                                    SwapGate, CXGate, CYGate, CZGate, CSXGate, CSwapGate, CCXGate, CCZGate, CPhaseGate, RXXGate, RYYGate, RZZGate, RZXGate,
                                    )

def _get_gate(name: str):

    no_param_gate_map = {
        "id":  IGate,"x": XGate, "y": YGate,"z": ZGate,"h": HGate,"s":SGate,"sdg": SdgGate,"sx":  SXGate,"sxdg": SXdgGate,"t":   TGate,
        "tdg": TdgGate,  "swap": SwapGate,   "cx":  CXGate,  "cy":  CYGate,    "cz":  CZGate, "csx": CSXGate, "ccx": CCXGate,  # Toffoli
        "ccz": CCZGate,"cswap": CSwapGate, "ecr":ECRGate
    }

    param_gate_map = {
        "u1":  (U1Gate, 1), "u2":  (U2Gate, 2),"u3":  (U3Gate, 3), "cu1": (CU1Gate, 1), "cu3": (CU3Gate, 3), "u":   (UGate, 3), "cu":  (CUGate, 4), "p":   (PhaseGate, 1),  # u1(λ) se mapea a PhaseGate(λ)
        "r":   (RGate, 2), "rx":  (RXGate, 1), "ry":  (RYGate, 1), "rz":  (RZGate, 1),  "crx": (CRXGate, 1), "cry": (CRYGate, 1), "crz": (CRZGate, 1),
        "rxx": (RXXGate, 1), "ryy": (RYYGate, 1),"rzz": (RZZGate, 1),"rzx": (RZXGate, 1), "cp": (CPhaseGate, 1)
    }

    gate_name = name.lower()

    # parametric gate
    if gate_name in no_param_gate_map:
        return no_param_gate_map[gate_name]()
    
    elif gate_name in param_gate_map:
        gate_cls, num_params = param_gate_map[gate_name]
        params = [Parameter(f"theta_{i}") for i in range(num_params)]

        return gate_cls(*params)
    
    else:
        logger.error(f"No such gate as '{gate_name}'.")
        raise ValueError # I capture this error at CunqaBackend


