import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather
from cunqa.qc_protocols import qrecv, qsend

# 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located=True)

try:
    qpus = get_QPUs(co_located=True, family = family)

    # 2. Design circuits with distributed instructions between them.
    #    Circuits that take part in a quantum communication protocol must reserve
    #    comm qubits, which is done with the tuple form of the constructor:
    #    CunqaCircuit((num_data_qubits, num_comm_qubits), num_clbits, id=...)
    #
    #  cc_1.data0   ─[H]──╌╌╌ teledata ╌╌╌╌►
    #                                       ╎
    #  cc_2.data0   ◄╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╯──●───[M]─
    #                                           |
    #  cc_2.data1   ────────────────────────── [X]──[M]─

    # First circuit: prepares |+> on its data qubit and teleports it to the second circuit
    cc_1 = CunqaCircuit((1, 1), 2, id="First")
    data_1, comm_1 = cc_1.get_qubits()

    cc_1.h(data_1[0])
    # qsend(circuit, data_qubit, comm_qubit, clbits, recving_circuit, tag)
    qsend(cc_1, data_1[0], comm_1[0], [0, 1], recving_circuit="Second", tag="teledata")

    # The teleported qubit is left in |0> at the sender, as the protocol resets it
    cc_1.measure(data_1[0], 0)

    # Second circuit: receives the state and entangles it with a local qubit
    cc_2 = CunqaCircuit((2, 1), 2, id="Second")
    data_2, comm_2 = cc_2.get_qubits()

    # qrecv(circuit, data_qubit, comm_qubit, clbits, control_circuit, tag)
    qrecv(cc_2, data_2[0], comm_2[0], [0, 1], control_circuit="First", tag="teledata")

    cc_2.cx(data_2[0], data_2[1])
    cc_2.measure(data_2[0], 0)
    cc_2.measure(data_2[1], 1)

    # 3. Execute distributed circuits on QPUs with quantum communications
    distr_jobs = run([cc_1, cc_2], qpus, shots=1024)

    # Collect the results
    result_list = gather(distr_jobs)

    # Print the counts. The second circuit is expected to show '00' and '11' with
    # roughly the same frequency, since it holds the teleported |+> state.
    for i, result in enumerate(result_list):
        print(f"Counts {i} is {result.counts}")

except Exception as error:
    raise error
finally:
    # 4. Relinquish resources: drop the deployed QPUs
    qdrop(family)
