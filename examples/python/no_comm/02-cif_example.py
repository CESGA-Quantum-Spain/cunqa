import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path.
# In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit


try:
    family = qraise(1, "01:00:00",  co_located = True)
    [qpu] = get_QPUs(co_located = True, family = family)


    # ---------------------------
    # Circuit:
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

    qjob = run(qc, qpu, shots = 1024)
    counts = qjob.result.counts

    print("Counts: ", counts)

    qdrop(family)

except Exception as error:
    qdrop(family)
    raise error