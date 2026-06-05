import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.transformations import add
from cunqa.qc_protocols import qsend, qrecv

# ---------------------------
# Acquiring the resources
# ---------------------------
family = qraise(2, "00:10:00", quantum_comm=True, simulator="Aer", co_located=True)
qpus  = get_QPUs(co_located=True, family=family)

try:
    # ---------------------------
    # Original circuits created and executed
    # 
    # circuit1.q0: ─[H]────────────
    #
    # circuit2.q0: ──●───●─────────
    #                │   $
    # circuit2.q1: ─[X]──$─────────
    #                    $
    # circuit3.q0: ─────[X]──[M]───
    #
    # Where $ represents the remote control of the gate
    # ---------------------------

    circuit1 = CunqaCircuit(1, id = "circuit1")
    circuit1.h(0)

    circuit2 = CunqaCircuit((2, 1), id = "circuit2")
    circuit2.cx(0,1)
    data_qubits2, link_qubits2 = circuit2.get_qubits()
    qsend(circuit2, data_qubits2[0], link_qubits2[0], [0, 1], recving_circuit="circuit3", tag="teledata")
    
    circuit3 = CunqaCircuit((1, 1), id = "circuit3")
    data_qubits3, link_qubits3 = circuit3.get_qubits()
    qrecv(circuit3, data_qubits3[0], link_qubits2[0], [0, 1], control_circuit = "circuit2", tag="teledata")
    
    circuit3.measure_all()


    # ---------------------------
    # Addition of circuits
    # 
    # added_circuit.q0: ─[H]──●───●───[M]───
    #                         │   $
    # added_circuit.q1: ─────[X]──$───[M]───
    #                             $
    # circuit3.q0:      ─────────[X]──[M]───
    #
    # Where $ represents the remoteness of the gate
    # ---------------------------
    added_circuit = add([circuit1, circuit2])
    #print(added_circuit.instructions)
    
    added_circuit.measure_all()

    qjobs = run([circuit3, added_circuit], qpus, shots = 1024)# non-blocking call
    results = gather(qjobs)

    for result in results:
            print(f"Result after split: {result.counts}")
        
except Exception as error:
    raise error
finally:
    # ---------------------------
    # Relinquishing resources
    # ---------------------------                                  
    qdrop(family)

