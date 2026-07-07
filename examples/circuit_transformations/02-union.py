import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.qc_protocols import cat_entangler, cat_disentangler
from cunqa.circuit import CunqaCircuit
from cunqa.circuit.transformations import union
from cunqa.qjob import gather

# ---------------------------
# Acquiring resources
# ---------------------------
family_separated = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located=True)
qpus_separated = get_QPUs(co_located=True, family=family_separated)

family_union = qraise(1, "00:10:00", simulator="Aer", co_located=True)
[qpu_union] = get_QPUs(co_located=True, family=family_union)

try:
    # ---------------------------
    # Communicated circuits created and executed.
    # The remote control of the gate (a remote CX) is implemented with the
    # cat-entangler / cat-disentangler telegate protocol.
    #
    # circuit1.data: ─[H]──●──[M]─
    #                      $
    # circuit2.data: ─────[X]─[M]─
    # Where $ represents the remote control of the gate
    # ---------------------------
    circuit1 = CunqaCircuit((1, 1), 1, id="circuit1") # (data qubits, comm qubits), clbits
    circuit2 = CunqaCircuit((1, 1), 1, id="circuit2")

    data_1, comm_1 = circuit1.get_qubits()
    data_2, comm_2 = circuit2.get_qubits()

    circuit1.h(data_1[0])

    cat_entangler(
        [circuit1, circuit2],
        data_1[0],
        [comm_1[0], comm_2[0]],
        [0, 0],
        tag="telegate"
    )

    circuit2.cx(comm_2[0], data_2[0])

    cat_disentangler(
        [circuit1, circuit2],
        data_1[0],
        [comm_2[0]],
        [0],
        [0]
    )

    circuit1.measure(data_1[0], 0)
    circuit2.measure(data_2[0], 0)

    qjobs = run([circuit1, circuit2], qpus_separated, shots=1024)
    results = gather(qjobs)

    for i, result in enumerate(results):
        print(f"\nResult before union (circuit{i + 1}): {result.counts}")

    # ---------------------------
    # Take the union of the circuits and execute it on a single QPU.
    # The communication directives (gen_ent/send/recv) are replaced by local
    # operations, yielding a single equivalent circuit.
    #
    # union_circuit.q0: ─[H]──●──[M]─
    #                         |
    # union_circuit.q1: ─────[X]─[M]─
    # ---------------------------
    union_circuit = union([circuit1, circuit2])

    qjob = run(union_circuit, qpu_union, shots=1024) # non-blocking call
    results = qjob.result

    print(f"\nResult after union: {results.counts}\n")

except Exception as error:
    raise error
finally:
    # ---------------------------
    # Relinquishing resources
    # ---------------------------
    qdrop(family_union)
    qdrop(family_separated)
