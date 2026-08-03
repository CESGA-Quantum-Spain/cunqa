"""
Currently, the GPU option is only available to the GPU architecture at CESGA.
For executing using GPUs, CUNQA must be compiled with the cmake flag -DAER_GPU=TRUE,
and with the specific GPU architecture. Check the installtion guide for details:
https://cesga-quantum-spain.github.io/cunqa/getting_started.html
"""
import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather
from cunqa.qc_protocols import qsend, qrecv

try:
    # 1. Deploy vQPUs and retrieve them using get_QPUs
    # The number of cores must match the ones given in the warning at https://cesga-docs.gitlab.io/ft3-user-guide/gpu_nodes.html#nvidia-a100
    # Number of cores should be modified if more QPUs are requested
    family = qraise(2, "00:10:00", cores_per_qpu = 8, simulator="Aer", quantum_comm=True,
                    co_located = True, gpu = True)
except Exception as error:
    raise error

try:
    qpus = get_QPUs(co_located = True, family = family)

    # 2. Design circuits with distributed instructions between them.
    #    A Bell pair is created locally in circuit1 and one of its halves is
    #    teleported to circuit2, so the entanglement ends up spread over both vQPUs.
    circuit1 = CunqaCircuit((2, 1), 2, id = "circuit1")
    data_1, comm_1 = circuit1.get_qubits()

    circuit1.h(data_1[0])
    circuit1.cx(data_1[0], data_1[1])

    # This qubit is teleported, and so it is reset at the sender
    qsend(circuit1, data_1[1], comm_1[0], [0, 1], recving_circuit="circuit2", tag="teledata")

    circuit1.measure(data_1[0], 0)

    circuit2 = CunqaCircuit((1, 1), 2, id = "circuit2")
    data_2, comm_2 = circuit2.get_qubits()

    qrecv(circuit2, data_2[0], comm_2[0], [0, 1], control_circuit="circuit1", tag="teledata")

    circuit2.measure(data_2[0], 0)

    # 3. Execute distributed circuits on QPUs with quantum communications and GPU enabled
    qjobs = run([circuit1, circuit2], qpus, shots = 100)

    # Collect the results
    results = gather(qjobs)

    # Print the counts. Both circuits are expected to show the same distribution of
    # '0' and '1', since they hold the two halves of the same Bell pair.
    for result in results:
        print(f"Counts is {result.counts}")

except Exception as error:
    raise error
finally:
    # 4. Relinquish resources: drop the deployed QPUs
    qdrop(family)
