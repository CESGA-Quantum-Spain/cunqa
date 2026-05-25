
from typing import Union

from cunqa.circuit.core import CunqaCircuit

def qsend(
    circuit: CunqaCircuit, 
    comp_qubit: int, 
    comm_qubit: int, 
    clbits: list[int],
    recving_circuit: Union[str, 'CunqaCircuit'],
    tag: str = None
) -> None:
    """
    Class method to send a qubit from the current circuit to another one.
    
    Args:
        comp_qubit (int): computation qubit to be sent.
        comm_qubit (int): communication qubit employed to send.
        recving_circuit (str | CunqaCircuit): id of the circuit or circuit to which the qubit is 
                                                sent.
        tag (str): unique identifier for the operation.
    """
    
    circuit.gen_ent(comm_qubit, recving_circuit, tag)
    circuit.cx(comp_qubit, comm_qubit)
    circuit.h(comp_qubit)
    
    circuit.measure([comp_qubit, comm_qubit], clbits, save=False)
    circuit.send(clbits, recving_circuit)
    

def qrecv(
    circuit: CunqaCircuit, 
    comp_qubit: int, 
    comm_qubit: int, 
    clbits: list[int],
    control_circuit: Union[str, 'CunqaCircuit'],
    tag: str = None    
) -> None:
    """
    Class method to receive a qubit from a remote circuit into an ancilla qubit.
    
    Args:
        comp_qubit (int): computation qubit the received qubit is assigned.
        comm_qubit (int): communication qubit employed to receive.
        control_circuit (str | CunqaCircuit): id of the circuit from which the qubit is received.
    """
    
    circuit.gen_ent(comm_qubit, control_circuit, tag)
    
    circuit.recv(clbits, control_circuit)
    
    circuit.cif(clbits[0])
    circuit.x(comm_qubit)
    circuit.endcif(clbits[0])
    
    circuit.cif(clbits[1])
    circuit.z(comm_qubit)
    circuit.endcif(clbits[1])
    
    circuit.swap(comm_qubit, comp_qubit)
    
    