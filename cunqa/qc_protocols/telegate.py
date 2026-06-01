from typing import Union

from cunqa.circuit.core import CunqaCircuit

# TODO: Generalize telegate for N circuits
    
def cat_entangler(
    target_circuits: list[CunqaCircuit],
    comp_qubit: int,
    comm_qubits: list[int], 
    clbits: list[int], 
    tag: str = None
):
    target_circuits[0].gen_ent(comm_qubits[0], target_circuits[1], tag)
    target_circuits[0].cx(comp_qubit, comm_qubits[0])
    target_circuits[0].measure(comm_qubits[0], clbits[0], save=False)
    target_circuits[0].send(clbits[0], target_circuits[1])
    
    target_circuits[1].gen_ent(comm_qubits[1], target_circuits[0], tag)
    target_circuits[1].recv(clbits[1], target_circuits[0])
    target_circuits[1].cif(clbits[1])
    target_circuits[1].x(comm_qubits[1])
    target_circuits[1].endcif(clbits[1])
    
    """ for circuit, comm_qubit, clbit in zip(target_circuits[1:], comm_qubits[1:], clbits[1:]):
        circuit.recv(clbit, target_circuits)
        circuit.cif(clbit)
        circuit.x(comm_qubit)
        circuit.endcif(clbit) """
    
def cat_disentangler(
    target_circuits: Union[list[CunqaCircuit], list[str]],
    comp_qubit: int,
    comm_qubits: list[int], 
    clbits: list[int]
):
    
    target_circuits[0].recv(clbits[0], target_circuits[1])
    target_circuits[0].cif(clbits[0])
    target_circuits[0].z(comp_qubit)
    target_circuits[0].endcif(clbits[0])
    
    target_circuits[1].h(comm_qubits[1])
    target_circuits[1].measure(comm_qubits[1], clbits[1], save=False)
    target_circuits[1].send(clbits[1], target_circuits[0])