import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit


try:
    # 1. Deploy vQPUs
    family = qraise(1, "01:00:00",  co_located = True)
    [qpu] = get_QPUs(co_located = True, family = family)


    # 2. Design circuit:
    # ---------------------------
    #  qc.q0   ─[H]───[M]───[M]─
    #                  ‖     
    #  qc.q1   ───────[X]───[M]─
    # ---------------------------
    qc = CunqaCircuit(2, 2)
    qc.h(0)
    qc.measure(0, 0)

    with qc.cif(0) as cgates:
        cgates.x(1)

    qc.measure(0,0)
    qc.measure(1,1)

    # 3. Execute circuit on vQPU
    qjob = run(qc, qpu, shots = 1024)
    counts = qjob.result.counts

    print("Counts: ", counts)

    # 4. Relinquish resources
    qdrop(family)

except Exception as error:
    qdrop(family)
    raise error