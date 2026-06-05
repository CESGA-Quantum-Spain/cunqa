from typing import Union

from cunqa.circuit.core import CunqaCircuit
    
def cat_entangler(
    target_circuits: list[CunqaCircuit],
    data_qubit: int,
    link_qubits: list[int], 
    clbits: list[int], 
    tag: str = None
):
    for target_circuit, link_qubit in zip(target_circuits, link_qubits):
        target_circuit.gen_ent(link_qubit, target_circuits, tag) 

    target_circuits[0].cx(data_qubit, link_qubits[0])
    target_circuits[0].measure(link_qubits[0], clbits[0], save=False)
    
    # Reset to 0 value of the link qubit employed
    target_circuits[0].cif(clbits[0])
    target_circuits[0].x(link_qubits[0])
    target_circuits[0].endcif()
    
    for target_circuit in target_circuits[1:]: 
        target_circuits[0].send(clbits[0], target_circuit)
        
    
    for recv_circuit, clbit, link_qubit in zip(target_circuits[1:], clbits[1:], link_qubits[1:]):
        recv_circuit.recv(clbit, target_circuits[0])
        recv_circuit.cif(clbit)
        recv_circuit.x(link_qubit)
        recv_circuit.endcif()
    
def cat_disentangler(
    target_circuits: Union[list[CunqaCircuit], list[str]],
    data_qubit: int,
    link_qubits: list[int], 
    recv_clbits: list[int],
    send_clbits: list[int]
):
    for send_circuit, clbit in zip(target_circuits[1:], recv_clbits):
        target_circuits[0].recv(clbit, send_circuit)
        
    target_circuits[0].cif(recv_clbits, operation="xor")
    target_circuits[0].z(data_qubit)
    target_circuits[0].endcif()
    
    for send_circuit, clbit, link_qubit in zip(target_circuits[1:], send_clbits, link_qubits):
        send_circuit.h(link_qubit)
        send_circuit.measure(link_qubit, clbit, save=False)
        
        # Reset to 0 value of the link qubit employed
        send_circuit.cif(clbit)
        send_circuit.x(link_qubit)
        send_circuit.endcif()
        
        send_circuit.send(clbit, target_circuits[0])