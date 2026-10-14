import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit

try:
    # 1. Deploy a vQPU with simulated time. The "timing" block of the backend gives each
    #    operation a duration (in seconds), independent of how long the simulation takes on the
    #    HPC: 1-qubit gates 0.05 us, 2-qubit gates 0.3 us, measurements 1 us. The times below are
    #    in us, while the result reports them in seconds.
    file_dir = os.path.dirname(os.path.abspath(__file__))
    backend_path = file_dir + "/timed_backend.json"

    family = qraise(1, "00:10:00", simulator="Aer", co_located=True, backend=backend_path)
except Exception as error:
    raise error

try:
    [qpu] = get_QPUs(co_located=True, family=family)

    # 2. Static circuit: operations are scheduled as soon as their qubits are free, so
    #    independent gates run in parallel.
    # ---------------------------------------------------------------
    #  q0   ─[H]──■──[M]─      H: [0, 0.05)   CX: [0.05, 0.35)  M: [0.35, 1.35)
    #             │
    #  q1   ─────[X]─[M]─      idles 0.05 us waiting for the CX, M: [0.35, 1.35)
    #
    #  q2   ─[H]─[M]─────      H: [0, 0.05)   M: [0.05, 1.05)
    # ---------------------------------------------------------------
    #  Expected: every shot lasts 1.35 us, idle_mean = [0, 0.05 us, 0]
    qc = CunqaCircuit(3)
    qc.h(0)
    qc.h(2)
    qc.cx(0, 1)
    qc.measure_all()

    result = run(qc, qpu, shots=1000).result
    print("Static circuit")
    print(f"\tCounts: {result.counts}")
    print(f"\tWall-clock time: {result.time_taken} s")
    print(f"\tSimulated time: {result.simulated_time}")

    # 3. Dynamic circuit: the X on q1 waits until the measurement of q0 is known.
    # ---------------------------------------------------------------
    #  q0   ─[H]─[M]─────────      H: [0, 0.05)  M: [0.05, 1.05) -> c0 ready at 1.05
    #              ‖
    #  q1   ──────[X]──[M]───      if c0 == 1: q1 idles 1.05 us, X: [1.05, 1.1),
    #                                          M: [1.1, 2.1) -> shot lasts 2.1 us
    #                              if c0 == 0: X is skipped, but q1 still waits for c0,
    #                                          M: [1.05, 2.05) -> shot lasts 2.05 us
    # ---------------------------------------------------------------
    #  Expected: min 2.05 us, max 2.1 us, mean ~2.075 us, idle_mean ~[0, 1.05 us, 0]
    qc = CunqaCircuit(2, 2)
    qc.h(0)
    qc.measure(0, 0)
    qc.cif(0)
    qc.x(1)
    qc.endcif()
    qc.measure(1, 1)

    result = run(qc, qpu, shots=1000).result
    print("Dynamic circuit")
    print(f"\tCounts: {result.counts}")
    print(f"\tWall-clock time: {result.time_taken} s")
    print(f"\tSimulated time: {result.simulated_time}")

    # 4. Relinquish resources
    qdrop(family)

except Exception as error:
    qdrop(family)
    raise error
