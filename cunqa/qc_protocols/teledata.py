
from typing import Union

from cunqa.circuit.core import CunqaCircuit

def qsend(
    circuit: CunqaCircuit, 
    data_qubit: int, 
    link_qubit: int, 
    clbits: list[int],
    recving_circuit: Union[str, 'CunqaCircuit'],
    tag: str = None
) -> None:
    """
    Class method to send a qubit from the current circuit to another one.
    
    Args:
        data_qubit (int): computation qubit to be sent.
        link_qubit (int): communication qubit employed to send.
        recving_circuit (str | CunqaCircuit): id of the circuit or circuit to which the qubit is 
                                                sent.
        tag (str): unique identifier for the operation.
    """
    
    circuit.gen_ent(link_qubit, recving_circuit, tag)
    circuit.cx(data_qubit, link_qubit)
    circuit.h(data_qubit)
    
    circuit.measure([data_qubit, link_qubit], clbits, save=False)
    
    # Reset to 0 value of the teleported qubit and link qubit employed
    circuit.cif(clbits[0])
    circuit.x(data_qubit)
    circuit.endcif()
    circuit.cif(clbits[1])
    circuit.x(link_qubit)
    circuit.endcif()
    
    circuit.send(clbits, recving_circuit)
    

def qrecv(
    circuit: CunqaCircuit, 
    data_qubit: int, 
    link_qubit: int, 
    clbits: list[int],
    control_circuit: Union[str, 'CunqaCircuit'],
    tag: str = None    
) -> None:
    """
    Class method to receive a qubit from a remote circuit into an ancilla qubit.
    
    Args:
        data_qubit (int): computation qubit the received qubit is assigned.
        link_qubit (int): communication qubit employed to receive.
        control_circuit (str | CunqaCircuit): id of the circuit from which the qubit is received.
    """
    
    circuit.gen_ent(link_qubit, control_circuit, tag)
    
    circuit.recv(clbits, control_circuit)
    
    circuit.cif(clbits[0])
    circuit.x(link_qubit)
    circuit.endcif()
    
    circuit.cif(clbits[1])
    circuit.z(link_qubit)
    circuit.endcif()
    
    circuit.swap(link_qubit, data_qubit)
    
    