"""
Holds dunder methods for the class CunqaCircuit and other functions to extract information from it.
"""
from typing import Union
import copy
import numpy as np

from cunqa.logger import logger
from cunqa.circuit.core import CunqaCircuit

def layers(circuit: CunqaCircuit) -> dict:
    """
    Dictionary with keys the qubits and values lists of sublists with elements 
    [layer_number, index_of_gate_in_instructions, gate_name]. 
    """
    layer_dict = {f"{i}": [] for i in range(circuit.num_qubits)} 
    last_active_layer = [0 for _ in range(circuit.num_qubits)]

    for i, instr in enumerate(circuit.instructions):
        if len(instr["qubits"]) == 1:
            last_active_layer[instr["qubits"][0]]+=1
            layer_dict[str(instr["qubits"][0])].append(
                [last_active_layer[instr["qubits"][0]], i, instr["name"]]
            )

        elif "conditional_reg" in instr:
            max_layer_qubit = max([last_active_layer[j] for j in set(instr["qubits"] + 
                                                                            instr["conditonal_reg"])])
            for j in instr["qubits"]:
                last_active_layer[j] = max_layer_qubit + 1
                layer_dict[str(j)].append([last_active_layer[j], i, instr["name"]])

        elif len(instr["qubits"]) > 1:
            max_layer_qubit = max([last_active_layer[j] for j in instr["qubits"]])
            for j in instr["qubits"]:
                last_active_layer[j] = max_layer_qubit + 1
                layer_dict[str(j)].append([last_active_layer[j], i, instr["name"]])

        # TODO: support the ever problematic rcontrol and recv (they have zero qubits)

        return layer_dict


def vsplit():
    pass

def hsplit(circuit: CunqaCircuit, qubits_or_sections: Union[list, int]) -> list[CunqaCircuit]:
    num_qubits = circuit.num_qubits

    if isinstance(qubits_or_sections, list):
        # handle list case.
        if np.sum(qubits_or_sections) != num_qubits:
            logger.error(f"Error: Incorrect hsplit of the circuit, {qubits_or_sections} does not add up to {num_qubits} qubits")
            raise RuntimeError
        Nsections = len(qubits_or_sections)
        initial_qubits = [0] + list(np.cumsum(qubits_or_sections))

    elif isinstance(qubits_or_sections, int):
        # indices_or_sections is a scalar, not a list.
        Nsections = int(qubits_or_sections)
        if Nsections <= 0:
            raise ValueError('number sections must be larger than 0.') from None
        Neach_section, extras = divmod(num_qubits, Nsections)
        section_sizes = (extras * [Neach_section + 1] +
                         (Nsections - extras) * [Neach_section])
        initial_qubits = [0] + list(np.cumsum(section_sizes))

    def get_subcircuits(circuit, initial_qubits, Nsections):
        sub_circuits = []
        for i in range(Nsections):
            num_qubits_i = initial_qubits[i + 1] - initial_qubits[i]
            sub_circuits.append(CunqaCircuit(num_qubits_i, id= circuit.info["id"] + f"_{i}"))

        def find_index(array, value):
            for i, elem in enumerate(array):
                if(elem > value):
                    return i - 1

        for inst in circuit.instructions[:]:
            i = find_index(initial_qubits, inst["qubits"][0])
            sub_circuit = sub_circuits[i]

            if len(inst["qubits"]) == 1:
                # One qubit gate
                inst["qubits"][0] -= initial_qubits[i]
                sub_circuit.add_instructions([inst])
            elif len(inst["qubits"]) == 2:
                # Two qubits gate
                j = find_index(initial_qubits, inst["qubits"][1])
                if i != j:
                    # Have to divide the gate
                    target_circuit = sub_circuits[j]

                    ctrl_qubit = inst["qubits"][0] - initial_qubits[i]
                    target_qubit = inst["qubits"][1] - initial_qubits[j]

                    with sub_circuit.expose(ctrl_qubit, target_circuit) as rcontrol:
                        inst["qubits"][0] = rcontrol
                        inst["qubits"][1] = target_qubit
                        target_circuit.add_instructions([inst])
                else:
                    inst["qubits"][0] -= initial_qubits[i]
                    inst["qubits"][1] -= initial_qubits[i]
                    sub_circuit.add_instructions([inst])
            else:
                # Puertas de más de dos qubits
                pass
        
        return sub_circuits 
    
    return get_subcircuits(copy.deepcopy(circuit), initial_qubits, Nsections)