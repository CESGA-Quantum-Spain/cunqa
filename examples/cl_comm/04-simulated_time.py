import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

# 1. QPU deployment with simulated time. Every vQPU uses the "timing" block of the backend (in
#    seconds): 1-qubit gates 0.05 us, measurements 1 us and 0.5 us of latency for each classical
#    message. The times below are in us, while the result reports them in seconds.
file_dir = os.path.dirname(os.path.abspath(__file__))
backend_path = file_dir + "/timed_backend.json"

family_name = qraise(2, "00:10:00", simulator="Aer", classical_comm=True, co_located=True,
                     family="qpus_timed_class_comms", backend=backend_path)

try:
    qpus = get_QPUs(family=family_name, co_located=True)

    # 2. Circuit design with classical communications directives.
    # ---------------------------------------------------------------
    #  circuit_1:  q0 ─[H]─[M]─ send ─>      H: [0, 0.05)  M: [0.05, 1.05), sent at 1.05 us
    #                                        -> every shot lasts 1.05 us
    #
    #  circuit_2:  recv ─> c0                arrives at 1.05 + 0.5 (latency) = 1.55 us
    #              q0 ──────[X]──[M]─        if c0 == 1: q0 idles 1.55 us, X: [1.55, 1.6),
    #                                                    M: [1.6, 2.6) -> shot lasts 2.6 us
    #                                        if c0 == 0: M: [0, 1), shot lasts 1.55 us
    # ---------------------------------------------------------------
    #  Expected for circuit_2: min 1.55 us, max 2.6 us, mean ~2.075 us, idle_mean ~[0.775 us, 0]
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
