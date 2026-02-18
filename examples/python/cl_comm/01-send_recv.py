import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

try:
    # 1. QPU deployment

    # If GPU execution is desired, just add "gpu = True" as another qraise argument
    family_name = qraise(2, "01:00:00", classical_comm=True, co_located=True, family = "qpus_class_comms")
    qpus = get_QPUs(family = family_name, co_located=True)

    # 2. Circuit design with classical communications directives

    circuit_1 = CunqaCircuit(2, 2, id="circuit_1")
    circuit_2 = CunqaCircuit(1, 1, id="circuit_2")

    circuit_1.h(0)
    circuit_1.measure(0,0)

    circuit_1.send(0, recving_circuit = "circuit_2")
    circuit_1.measure(1,1)

    circuit_2.recv(0, sending_circuit = "circuit_1")

    with circuit_2.cif(0) as cgates:
        cgates.x(0)

    circuit_2.measure(0,0)

    # 3. Execution

    distributed_qjobs = run([circuit_1, circuit_2], qpus, shots=1000)
    results = gather(distributed_qjobs)
    counts_list = [result.counts for result in results]

    for counts, qpu in zip(counts_list, qpus):

        print(f"Counts from vQPU {qpu.id}: {counts}")

    # 4. Release classical resources
    qdrop(family_name)

except Exception as error:
    # 4. Release resources even if an error is raised
    qdrop(family_name)
    raise error
