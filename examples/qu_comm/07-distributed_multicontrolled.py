"""
Example of a distributed multicontrolled gate. The two control qubits live in the
first circuit and the target qubit in the second one. Two nested telegate blocks
share both controls onto the comm qubits of the second circuit, which then applies
the multicontrolled gate locally.
"""
import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather
from cunqa.qc_protocols import cat_entangler, cat_disentangler

try:
    file_dir = os.path.dirname(os.path.abspath(__file__))
    backend_path = file_dir + "/07-distributed_multicontrolled.json"
    # 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
    family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located=True, backend=backend_path)
except Exception as error:
    raise error

try:
    qpus = get_QPUs(co_located=True, family=family)

    # 2. Design circuits with distributed instructions between them.
    #    One comm qubit per shared control is needed on each side.
    #
    #  cc_1.data0   ─[H]──●───────
    #                     $
    #  cc_1.data1   ─[H]──●───────
    #                     $ 
    #  cc_2.data0   ─────[X]──[M]─
    #
    #  Where $ represents the remote control of the gate
    cc_1 = CunqaCircuit((2, 2), 2, id="First")
    data_1, comm_1 = cc_1.get_qubits()

    cc_2 = CunqaCircuit((1, 2), 2, id="Second")
    data_2, comm_2 = cc_2.get_qubits()

    cc_1.h(data_1[0])
    cc_1.h(data_1[1])

    # Each cat_entangler shares one control qubit of cc_1 onto one comm qubit of cc_2
    cat_entangler(
        [cc_1, cc_2],
        data_1[0],
        [comm_1[0], comm_2[0]],
        [0, 0],
        tag="telegate_0"
    )
    cat_entangler(
        [cc_1, cc_2],
        data_1[1],
        [comm_1[1], comm_2[1]],
        [1, 1],
        tag="telegate_1"
    )

    # Both controls are now available locally at cc_2, so the multicontrolled gate
    # is applied as any other local gate
    cc_2.mcx(comm_2[0], comm_2[1], data_2[0])

    # The blocks are closed in reverse order, each one with the comm qubit it opened
    cat_disentangler(
        [cc_1, cc_2],
        data_1[1],
        [comm_2[1]],
        [1],
        [1]
    )
    cat_disentangler(
        [cc_1, cc_2],
        data_1[0],
        [comm_2[0]],
        [0],
        [0]
    )

    cc_1.measure(data_1[0], 0)
    cc_1.measure(data_1[1], 1)
    cc_2.measure(data_2[0], 0)

    # 3. Execute distributed circuits on QPUs with quantum communications
    distr_jobs = run([cc_1, cc_2], qpus, shots=1000)

    # Collect the results
    result_list = gather(distr_jobs)

    # Print the counts. The first circuit gives the four outcomes with the same
    # frequency, and the second one gives '1' only when both controls are 1, that
    # is, in about a quarter of the shots.
    for i, result in enumerate(result_list):
        print(f"Counts {i} is {result.counts}")

except Exception as error:
    raise error
finally:
    # 4. Relinquish resources: drop the deployed QPUs
    qdrop(family)
