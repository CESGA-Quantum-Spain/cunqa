from typing import Union

from cunqa.circuit.core import CunqaCircuit
    
def cat_entangler(
    target_circuits: list[CunqaCircuit],
    data_qubit: int,
    comm_qubits: list[int], 
    clbits: list[int], 
    tag: str = None
):
    for target_circuit, comm_qubit in zip(target_circuits, comm_qubits):
        target_circuit.gen_ent(comm_qubit, target_circuits, tag) 

    target_circuits[0].cx(data_qubit, comm_qubits[0])
    target_circuits[0].measure(comm_qubits[0], clbits[0], save=False)
    
    # Reset to 0 value of the comm qubit employed
    target_circuits[0].cif(clbits[0])
    target_circuits[0].x(comm_qubits[0])
    target_circuits[0].endcif()
    
    for target_circuit in target_circuits[1:]: 
        target_circuits[0].send(clbits[0], target_circuit)
        
    
    for recv_circuit, clbit, comm_qubit in zip(target_circuits[1:], clbits[1:], comm_qubits[1:]):
        recv_circuit.recv(clbit, target_circuits[0])
        recv_circuit.cif(clbit)
        recv_circuit.x(comm_qubit)
        recv_circuit.endcif()
    
def cat_disentangler(
    target_circuits: Union[list[CunqaCircuit], list[str]],
    data_qubit: int,
    comm_qubits: list[int], 
    recv_clbits: list[int],
    send_clbits: list[int]
):
    for send_circuit, clbit in zip(target_circuits[1:], recv_clbits):
        target_circuits[0].recv(clbit, send_circuit)
        
    target_circuits[0].cif(recv_clbits, operation="xor")
    target_circuits[0].z(data_qubit)
    target_circuits[0].endcif()
    
    for send_circuit, clbit, comm_qubit in zip(target_circuits[1:], send_clbits, comm_qubits):
        send_circuit.h(comm_qubit)
        send_circuit.measure(comm_qubit, clbit, save=False)
        
        # Reset to 0 value of the comm qubit employed
        send_circuit.cif(clbit)
        send_circuit.x(comm_qubit)
        send_circuit.endcif()
        
        send_circuit.send(clbit, target_circuits[0])