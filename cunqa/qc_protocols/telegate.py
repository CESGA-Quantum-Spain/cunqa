from typing import Union

from cunqa.circuit.core import CunqaCircuit

def expose(
    circuit: CunqaCircuit, 
    comp_qubit: int, 
    comm_qubit: int, 
    clbits: list[int],
    target_circuits: Union[CunqaCircuit, str, list[CunqaCircuit], list[str]], 
    tag: str = None
) -> None:
    """
    Class method to expose one or several qubits to a target circuit.
    
    Args:
        qubits (int | list[int]): index or list of indices of qubit(s) to be exposed.
        target_circuit (CunqaCircuit | str): CunqaCircuit object or string ID of the circuit 
                                                that will ''see'' the exposed qubits.
        tags (int | list[int]): Optional negative integer or list of integers, each of one 
                                associated to a exposed qubit. If not set, random values are set.
    Result:
        The function returns a list of negative integers, corresponding to each exposed qubit. 
        This values can be used as arguments of controlled gates to specify that are remotely 
        controlled. 
    """
    
    circuit.gen_ent(comm_qubit, target_circuits, tag)


def unexpose(circuit: CunqaCircuit, tag: str = None) -> None:
    pass