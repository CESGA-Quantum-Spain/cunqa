
from typing import Union

from cunqa.circuit.core import CunqaCircuit

def qsend(
    circuit: CunqaCircuit, 
    data_qubit: int, 
    comm_qubit: int, 
    clbits: list[int],
    recving_circuit: Union[str, 'CunqaCircuit'],
    tag: str = None
) -> None:
    """
    Teledata sender: teleports the state of ``data_qubit`` from ``circuit`` to a remote circuit.

    This is the teleportation-protocol counterpart of :py:func:`qrecv`, which must be applied on
    the receiving circuit with the same ``tag``. Internally it generates the shared entanglement
    with :py:meth:`~cunqa.circuit.core.CunqaCircuit.gen_ent`, performs the Bell measurement and
    sends the two classical correction bits to the remote circuit.

    Args:
        circuit (~cunqa.circuit.core.CunqaCircuit): circuit holding the qubit to be teleported.
        data_qubit (int): computation qubit whose state is teleported.
        comm_qubit (int): communication qubit employed to send.
        clbits (list[int]): two classical bits used to store the Bell-measurement outcomes.
        recving_circuit (str | CunqaCircuit): id of the circuit or circuit object to which the
            qubit is teleported.
        tag (str): identifier shared with the matching :py:func:`qrecv` call.
    """
    
    circuit.gen_ent(comm_qubit, recving_circuit, tag)
    circuit.cx(data_qubit, comm_qubit)
    circuit.h(data_qubit)
    
    circuit.measure([data_qubit, comm_qubit], clbits, save=False)
    
    # Reset to 0 value of the teleported qubit and comm qubit employed
    circuit.cif(clbits[0])
    circuit.x(data_qubit)
    circuit.endcif()
    circuit.cif(clbits[1])
    circuit.x(comm_qubit)
    circuit.endcif()
    
    circuit.send(clbits, recving_circuit)
    

def qrecv(
    circuit: CunqaCircuit, 
    data_qubit: int, 
    comm_qubit: int, 
    clbits: list[int],
    control_circuit: Union[str, 'CunqaCircuit'],
    tag: str = None    
) -> None:
    """
    Teledata receiver: reconstructs into ``data_qubit`` the state teleported by a remote circuit.

    This is the receiving counterpart of :py:func:`qsend` and must be called with the same ``tag``.
    Internally it generates the shared entanglement with
    :py:meth:`~cunqa.circuit.core.CunqaCircuit.gen_ent`, receives the two classical correction bits
    and applies the conditional Pauli corrections.

    Args:
        circuit (~cunqa.circuit.core.CunqaCircuit): circuit that receives the teleported state.
        data_qubit (int): computation qubit to which the received state is assigned.
        comm_qubit (int): communication qubit employed to receive.
        clbits (list[int]): two classical bits used to store the received correction bits.
        control_circuit (str | CunqaCircuit): id of the circuit or circuit object from which the
            qubit is received.
        tag (str): identifier shared with the matching :py:func:`qsend` call.
    """
    
    circuit.gen_ent(comm_qubit, control_circuit, tag)
    
    circuit.recv(clbits, control_circuit)

    circuit.cif(clbits[1])
    circuit.x(comm_qubit)
    circuit.endcif()

    circuit.cif(clbits[0])
    circuit.z(comm_qubit)
    circuit.endcif()
    
    circuit.swap(comm_qubit, data_qubit)
    
    