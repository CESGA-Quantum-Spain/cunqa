import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

# 1. QPU deployment with simulated time. Every vQPU uses the "timing" block of the backend:
#    1-qubit gates 50 ns, measurements 1000 ns and 500 ns of latency for each classical message.
file_dir = os.path.dirname(os.path.abspath(__file__))
backend_path = file_dir + "/timed_backend.json"

family_name = qraise(2, "00:10:00", simulator="Aer", classical_comm=True, co_located=True,
                     family="qpus_timed_class_comms", backend=backend_path)

try:
    qpus = get_QPUs(family=family_name, co_located=True)

    # 2. Circuit design with classical communications directives.
    # ---------------------------------------------------------------
    #  circuit_1:  q0 ─[H]─[M]─ send ─>      H: [0, 50)  M: [50, 1050), sent at 1050 ns
    #                                        -> every shot lasts 1050 ns
    #
    #  circuit_2:  recv ─> c0                arrives at 1050 + 500 (latency) = 1550 ns
    #              q0 ──────[X]──[M]─        if c0 == 1: q0 idles 1550 ns, X: [1550, 1600),
    #                                                    M: [1600, 2600) -> shot lasts 2600 ns
    #                                        if c0 == 0: M: [0, 1000), shot lasts 1550 ns
    # ---------------------------------------------------------------
    #  Expected for circuit_2: min 1550, max 2600, mean ~2075, idle_mean ~[775, 0]
    circuit_1 = CunqaCircuit(1, 1, id="circuit_1")
    circuit_2 = CunqaCircuit(1, 1, id="circuit_2")

    circuit_1.h(0)
    circuit_1.measure(0, 0)
    circuit_1.send(0, recving_circuit="circuit_2")

    circuit_2.recv(0, sending_circuit="circuit_1")
    circuit_2.cif(0)
    circuit_2.x(0)
    circuit_2.endcif()
    circuit_2.measure(0, 0)

    # 3. Execution
    distributed_qjobs = run([circuit_1, circuit_2], qpus, shots=1000)
    results = gather(distributed_qjobs)

    for result, qpu in zip(results, qpus):
        print(f"vQPU {qpu.id}")
        print(f"\tCounts: {result.counts}")
        print(f"\tWall-clock time: {result.time_taken} s")
        print(f"\tSimulated time: {result.simulated_time}")

except Exception as error:
    raise error
finally:
    # 4. Release classical resources
    qdrop(family_name)
