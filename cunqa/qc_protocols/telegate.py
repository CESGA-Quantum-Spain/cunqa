from typing import Union

from cunqa.circuit.core import CunqaCircuit
    
def cat_entangler(
    target_circuits: list[CunqaCircuit],
    data_qubit: int,
    comm_qubits: list[int], 
    clbits: list[int],
    tag: str = None
):
    """
    Telegate entangler: distributes the control state of ``data_qubit`` (held by the first circuit
    in ``target_circuits``) onto the comm qubits of the remaining circuits, so that they can apply
    gates locally controlled by it.

    It opens a telegate block that must be closed with :py:func:`cat_disentangler` using the same
    set of ``target_circuits``. Between the two calls, each receiving circuit applies the gate(s)
    controlled on its comm qubit. Internally it requests the shared GHZ state with
    :py:meth:`~cunqa.circuit.core.CunqaCircuit.gen_ent`.

    Args:
        target_circuits (list[~cunqa.circuit.core.CunqaCircuit]): participating circuits; the first
            one owns the control ``data_qubit`` and the rest receive it on their comm qubit.
        data_qubit (int): control qubit (in the first circuit) to be shared.
        comm_qubits (list[int]): comm qubit of each circuit in ``target_circuits``.
        clbits (list[int]): classical bit used by each circuit for the entangler corrections.
        tag (str): identifier shared with the matching :py:func:`cat_disentangler` call.
    """
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
    """
    Telegate disentangler: closes the telegate block opened by :py:func:`cat_entangler`, undoing the
    shared entanglement and restoring the comm qubits, while propagating the required phase
    correction back to the control ``data_qubit``.

    It must be called with the same ``target_circuits`` used in the matching
    :py:func:`cat_entangler`, after the receiving circuits have applied their controlled gates.

    Args:
        target_circuits (list[~cunqa.circuit.core.CunqaCircuit] | list[str]): the same participants
            passed to :py:func:`cat_entangler`; the first one owns the control ``data_qubit``.
        data_qubit (int): control qubit (in the first circuit) the correction is applied to.
        comm_qubits (list[int]): comm qubit of each receiving circuit.
        recv_clbits (list[int]): classical bits the first circuit uses to receive the corrections.
        send_clbits (list[int]): classical bits the receiving circuits use to send their corrections.
    """
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