"""
Example of circuits reserving more than one comm qubit. Two teledata protocols,
distinguished by their tags, run over two different comm qubits of the same pair
of circuits, moving a whole Bell pair from one vQPU to the other.
"""
import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather
from cunqa.qc_protocols import qsend, qrecv

try:
    file_dir = os.path.dirname(os.path.abspath(__file__))
    backend_path = file_dir + "/06-several_comm_qubits.json"
    # 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
    family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located = True, backend=backend_path)
except Exception as error:
    raise error

try:
    qpus = get_QPUs(co_located=True, family = family)

    # 2. Design circuits with distributed instructions between them.
    #    Each circuit reserves two comm qubits, one per teledata protocol, through
    #    the tuple form of the constructor: (num_data_qubits, num_comm_qubits).
    cc_1 = CunqaCircuit((2, 2), 4, id="First")
    data_1, comm_1 = cc_1.get_qubits()

    cc_2 = CunqaCircuit((2, 2), 4, id="Second")
    data_2, comm_2 = cc_2.get_qubits()

    # A Bell pair is prepared locally in the first circuit ...
    cc_1.h(data_1[0])
    cc_1.cx(data_1[0], data_1[1])

    # ... and both halves are teleported to the second circuit. Each protocol uses
    # its own comm qubit, its own pair of classical bits and its own tag, which is
    # what links each qsend with its matching qrecv.
    qsend(cc_1, data_1[0], comm_1[0], [0, 1], recving_circuit="Second", tag="teledata_0")
    qsend(cc_1, data_1[1], comm_1[1], [2, 3], recving_circuit="Second", tag="teledata_1")

    qrecv(cc_2, data_2[0], comm_2[0], [0, 1], control_circuit="First", tag="teledata_0")
    qrecv(cc_2, data_2[1], comm_2[1], [2, 3], control_circuit="First", tag="teledata_1")

    # The teleported qubits are left in |0> at the sender
    cc_1.measure(data_1[0], 0)
    cc_1.measure(data_1[1], 1)

    cc_2.measure(data_2[0], 0)
    cc_2.measure(data_2[1], 1)

    # 3. Execute distributed circuits on QPUs with quantum communications
    distr_jobs = run([cc_1, cc_2], qpus, shots=1024)

    # Collect the results
    result_list = gather(distr_jobs)

    # Print the counts. The first circuit is expected to give '00' always, while the
    # second one holds the Bell pair and gives '00' and '11' with the same frequency.
    for i, result in enumerate(result_list):
        print(f"Counts {i} is {result.counts}")

except Exception as error:
    raise error
finally:
    # 4. Relinquish resources: drop the deployed QPUs
    qdrop(family)
