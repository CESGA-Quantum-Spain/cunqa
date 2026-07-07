import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

import numpy as np

try:
    # 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
    # If GPU execution is desired, just add "gpu = True" as another qraise argument
    family = qraise(1, "00:10:00", simulator="Qulacs", co_located = True)
except Exception as error:
    raise error

try:
    qpus = get_QPUs(co_located=True, family = family)

    # 2. Design circuit:
    # ---------------------------
    #  qc.q0   ─[H]───●────[M]─
    #                 |      
    #  qc.q1   ──────[X]───[M]─
    # ---------------------------
    INV_SQRT_2 = 0.7071067811865475
    H = np.array([[INV_SQRT_2+0j, INV_SQRT_2+0j],
                    [INV_SQRT_2+0j, -INV_SQRT_2+0j]], dtype=complex)

    qc = CunqaCircuit(2, 2, id="qc")
    qc.unitary(H, 0)
    qc.cx(0, 1)
    qc.measure_all()

    # 3. Execute circuit and get the results 
    qresult = run(qc, qpus, shots=1024).result
    print(f"Result: {qresult}")

    # 4. Relinquish resources
    qdrop(family)

except Exception as error:
    qdrop(family)
    raise error