from typing import Union
import copy
import numpy as np
from itertools import accumulate

from cunqa.logger import logger
from cunqa.circuit.core import CunqaCircuit
from cunqa.constants import REMOTE_GATES

def vsplit():
    pass # TODO

def hsplit(circuit: CunqaCircuit, qubits_or_sections: Union[list, int]) -> list[CunqaCircuit]:
    """
    DOCUMENTADO
    """
    num_qubits = circuit.num_qubits

    if isinstance(qubits_or_sections, list):
        # handle list case.
        if np.sum(qubits_or_sections) != num_qubits:
            raise RuntimeError(f"Error: Incorrect hsplit of the circuit, {qubits_or_sections} does "
                               f"not add up to {num_qubits} qubits")
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

                    with sub_circuit.expose(ctrl_qubit, target_circuit) as (subcircuit, rcontrol):
                        inst["qubits"][0] = rcontrol
                        inst["qubits"][1] = target_qubit
                        subcircuit.add_instructions([inst])
                else:
                    inst["qubits"][0] -= initial_qubits[i]
                    inst["qubits"][1] -= initial_qubits[i]
                    sub_circuit.add_instructions([inst])
            else:
                raise ValueError("Three qubits gates cannot be partitioned.")
        
        return sub_circuits 
    
    return get_subcircuits(copy.deepcopy(circuit), initial_qubits, Nsections)

def union(circuits: list[CunqaCircuit]) -> CunqaCircuit:
    """
    DOCUMENTADO
    """
    if not circuits:
        raise ValueError("Empty list passed to perform union.")
    if len(circuits) == 1:
        logger.warning("Not enough circuits to perform a union, returning the original circuit.")
        return circuits[0]

    circuits = copy.deepcopy(circuits) # avoid aliasing

    qubit_offsets = [0] + list(accumulate(c.num_qubits for c in circuits[:-1]))
    clbit_offsets = [0] + list(accumulate(c.num_clbits for c in circuits[:-1]))
    circuit_ids = {c.id for c in circuits}

    def reindex(instr: dict, idx: int, exposed_q: int = -1) -> dict:
        new_instr = dict(instr)
        if "instructions" in new_instr:
            sub_instructions = []
            for sub_instr in new_instr["instructions"]:
                sub_instructions.append(reindex(sub_instr, idx, exposed_q))
            new_instr = sub_instructions
        if "qubits" in new_instr:
            if exposed_q == -1:
                new_instr["qubits"] = [q + qubit_offsets[idx] for q in new_instr["qubits"]]
            else:
                new_instr["qubits"] = [q + qubit_offsets[idx] if q != -1 else exposed_q 
                                       for q in new_instr["qubits"]]
        if "clbits" in new_instr:
            new_instr["clbits"] = [c + clbit_offsets[idx] for c in new_instr["clbits"]]
        return new_instr

    def is_valid_remote(instr: dict) -> bool:
        return (
            instr["name"] in REMOTE_GATES
            and all(cid in circuit_ids for cid in instr["circuits"])
        )

    union_circuit = CunqaCircuit(
        num_qubits=sum(c.num_qubits for c in circuits),
        num_clbits=sum(c.num_clbits for c in circuits),
        id="|".join(c.id for c in circuits),
    )
    union_instructions: list[dict] = []
    blocked: dict[str, dict] = {}

    finished = [False if len(circ.instructions) > 0 else True for circ in circuits]
    pointers = [0] * len(circuits)

    def advance(idx: int) -> None:
        pointers[idx] += 1
        if pointers[idx] == len(circuits[idx].instructions):
            finished[idx] = True

    def process_remote(instr: dict, idx: int, circuit_id: str) -> bool:
        """
        Returns True if instruction was consumed.
        """
        for target_id in instr["circuits"]:
            name = instr["name"]

            if name == "send":
                if target_id not in blocked:
                    return False
                blocked_instr = blocked[target_id]
                if blocked_instr["name"] == "recv":
                    instr_i = reindex(instr, idx)
                    union_instructions.append(
                        {
                            "name": "copy",
                            "l_clbits": blocked_instr["clbits"],
                            "r_clbits": instr_i["clbits"]
                        })
                    
                    del blocked[target_id]
                    return True
                
            if name == "qsend":
                if target_id not in blocked:
                    return False
                blocked_instr = blocked[target_id]
                if blocked_instr["name"] == "qrecv":
                    instr_i = reindex(instr, idx)
                    union_instructions.append(
                        {
                            "name": "swap",
                            "qubits": [
                                instr_i["qubits"][0],
                                blocked_instr["qubits"][0],
                            ],
                        }
                    )
                    union_instructions.append(
                        {"name": "reset", "qubits": instr_i["qubits"]}
                    )
                    del blocked[target_id]
                    return True

            elif name in ("qrecv", "expose", "recv"):
                blocked[circuit_id] = reindex(instr, idx)
                return True
            elif name == "rcontrol":
                if target_id not in blocked:
                    return False
                blocked_instr = blocked[target_id]
                if blocked_instr["name"] == "expose":
                    for sub_instr in reindex(instr, idx, blocked_instr["qubits"][0]):
                        union_instructions.append(sub_instr)
                    del blocked[target_id]
                    return True

        return False

    while not all(finished):
        for idx, circuit in enumerate(circuits):
            if finished[idx]:
                continue

            instr = circuit.instructions[pointers[idx]]
            consumed = False

            if is_valid_remote(instr):
                consumed = process_remote(instr, idx, circuit.id)
                union_circuit.is_dynamic = True

            elif circuit.id not in blocked:
                union_instructions.append(reindex(instr, idx))
                consumed = True

            if consumed:
                advance(idx)

    union_circuit.add_instructions(union_instructions)
    return union_circuit

def add(circuits: list[CunqaCircuit]) -> CunqaCircuit:
    """
    DOCUMENTADO
    """
    if not circuits:
        raise ValueError("Empty list passed to perform union.")
    if len(circuits) == 1:
        logger.warning("Not enough circuits to perform an addition, returning the original circuit.")
        return circuits[0]

    circuits = copy.deepcopy(circuits)
    circuit_ids = {c.id for c in circuits}

    addition_circuit = CunqaCircuit(
        num_qubits=max(c.num_qubits for c in circuits),
        num_clbits=max(c.num_clbits for c in circuits),
        id="+".join(c.id for c in circuits),
    )

    addition_instructions: list[dict] = []

    for circuit in circuits:
        for instr in circuit.instructions:
            if instr["name"] in REMOTE_GATES:
                for circ_id in instr["circuits"]:
                    if circ_id in circuit_ids:
                        raise ValueError("Cannot add two circuits that communicate with eachother.")
            addition_instructions.append(instr)

    addition_circuit.add_instructions(addition_instructions)
    return addition_circuit        

