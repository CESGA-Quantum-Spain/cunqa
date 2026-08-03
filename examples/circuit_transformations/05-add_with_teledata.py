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
    # Original circuits created
    #
    # circuit1.data0: ─[H]────────────
    #
    # circuit2.data0: ──●─────────────
    #                   │
    # circuit2.data1: ─[X]──╌╌╌╌╌╌╌╌╌╌► teledata
    #                                   ╎
    # circuit3.data0: ◄╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╯──[M]───
    # ---------------------------

    circuit1 = CunqaCircuit(1, id = "circuit1")
    circuit1.h(0)

    # The circuits taking part in the teledata must reserve a comm qubit and the two
    # classical bits that carry the Bell-measurement corrections
    circuit2 = CunqaCircuit((2, 1), 2, id = "circuit2")
    circuit2.cx(0, 1)
    data_qubits2, comm_qubits2 = circuit2.get_qubits()
    qsend(circuit2, data_qubits2[1], comm_qubits2[0], [0, 1], recving_circuit="circuit3", tag="teledata")

    circuit3 = CunqaCircuit((1, 1), 2, id = "circuit3")
    data_qubits3, comm_qubits3 = circuit3.get_qubits()
    qrecv(circuit3, data_qubits3[0], comm_qubits3[0], [0, 1], control_circuit = "circuit2", tag="teledata")

    circuit3.measure(data_qubits3[0], 0)

    # ---------------------------
    # Addition of circuits
    #
    # added_circuit.data0: ─[H]──●─────────[M]───
    #                            │
    # added_circuit.data1: ─────[X]──╌╌╌╌╌╌╌╌╌╌►
    #                                           ╎
    # circuit3.data0:      ◄╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╯──[M]───
    # ---------------------------
    added_circuit = add([circuit1, circuit2])

    # add() concatenates instructions and data qubits only, so the comm qubits and
    # the communication metadata of the operand have to be carried over explicitly
    added_circuit.add_comm_qubits(circuit2.num_qubits[1])
    added_circuit.sending_to = set(circuit2.sending_to)
    added_circuit.is_dynamic = circuit2.is_dynamic

    added_circuit.measure(0, 0)

    qjobs = run([added_circuit, circuit3], qpus, shots = 1024) # non-blocking call
    results = gather(qjobs)

    # Half of the Bell pair built by the addition is teleported to circuit3, so both
    # results are expected to show the same distribution of '0' and '1'
    for result in results:
        print(f"Result after addition: {result.counts}")

except Exception as error:
    raise error
finally:
    # ---------------------------
    # Relinquishing resources
    # ---------------------------
    qdrop(family)
