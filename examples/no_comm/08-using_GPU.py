import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import qraise, get_QPUs, run, qdrop
from cunqa.circuit import CunqaCircuit

# 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
# The GPU is requested with "gpu = True"; "gpu_name" and "partition" select the GPU node to run on
family = qraise(1, "00:10:00", cores_per_qpu = 32, simulator = "Aer", co_located = True,
                gpu = True, gpu_name = "t4", partition = "viz")

try:
    [qpu] = get_QPUs(co_located = True, family = family)

    # 2. Design circuit:
    # ---------------------------
    #  qc.q0   ─[H]───●────[M]─
    #                 |
    #  qc.q1   ──────[X]───[M]─
    # ---------------------------
    qc = CunqaCircuit(2)
    qc.h(0)
    qc.cx(0, 1)
    qc.measure_all()

    # 3. Execute the circuit on the deployed QPU
    qjob = run(qc, qpu, shots = 1000) # non-blocking call

    # Getting the counts
    print(f"Counts: {qjob.result.counts}")

except Exception as error:
    raise error
finally:
    # 4. Release classical resources
    qdrop(family)
