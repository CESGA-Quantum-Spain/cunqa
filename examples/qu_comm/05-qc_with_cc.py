"""
Example combining quantum communications (teledata) and classical communications
(send/recv) between the same pair of circuits.
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
    backend_path = file_dir + "/05-qc_with_cc.json"
    # 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
    # vQPUs raised with quantum communications also accept classical communication directives
    family = qraise(2, "00:10:00", simulator="Munich", quantum_comm=True, co_located=True, backend=backend_path)
except Exception as error:
    raise error

try:
    qpus = get_QPUs(co_located=True, family=family)

    # 2. Design circuits with distributed instructions between them
    # First circuit: teleports its data qubit 0 and, afterwards, sends the classical
    # outcome of measuring its data qubit 1
    cc_1 = CunqaCircuit((2, 1), 2, id="First")
    data_1, comm_1 = cc_1.get_qubits()

    cc_1.h(data_1[0])
    qsend(cc_1, data_1[0], comm_1[0], [0, 1], recving_circuit="Second", tag="teledata")

    cc_1.h(data_1[1])
    cc_1.measure(data_1[1], 0)
    cc_1.send(0, recving_circuit = "Second")

    # Second circuit: receives the teleported state, entangles it with a local qubit
    # and then applies an x gate conditioned on the classically received bit
    cc_2 = CunqaCircuit((3, 1), 3, id="Second")
    data_2, comm_2 = cc_2.get_qubits()

    qrecv(cc_2, data_2[0], comm_2[0], [0, 1], control_circuit="First", tag="teledata")
    cc_2.cx(data_2[0], data_2[1])

    cc_2.recv(2, sending_circuit = "First")
    cc_2.cif(2)
    cc_2.x(data_2[2])
    cc_2.endcif()

    cc_2.measure(data_2[0], 0)
    cc_2.measure(data_2[1], 1)
    cc_2.measure(data_2[2], 2)

    # 3. Execute distributed circuits on QPUs with quantum communications
    distr_jobs = run([cc_1, cc_2], qpus, shots=1024)

    # Collect the results
    result_list = gather(distr_jobs)

    # Print the counts. In the second circuit, the two least significant bits are
    # correlated ('00'/'11') because of the teleported state, and the most
    # significant one mirrors the bit classically received from the first circuit.
    for i, result in enumerate(result_list):
        print(f"Counts {i} is {result.counts}")

except Exception as error:
    raise error
finally:
    # 4. Relinquish resources: drop the deployed QPUs
    qdrop(family)
