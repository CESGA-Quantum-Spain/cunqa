import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import  qraise, get_QPUs, run, qdrop
from cunqa.circuit import CunqaCircuit

try:
    # 1. Deploy QMIO QPU
    family = qraise(1, "00:10:00", qmio = True)
    qpus = get_QPUs(co_located = True)
    qmio = qpus[0]


    # ---------------------------
    # 2. Design circuit:
    #  circuit.q0   ─[H]────●───[RZ(1.555)]─[M]─
    #                  |     
    #  circuit.q1   ───────[X]──────────────[M]─
    # ---------------------------
    circuit = CunqaCircuit(2, 4)
    circuit.h(0)
    circuit.cx(0,1)
    circuit.rz(1.555, 0)
    circuit.measure_all()

    # 3. Execute circuit on QMIO
    qjob0 = run(circuit, qmio, shots = 100)
    qjob1 = run(circuit, qmio, shots = 100)

    result0 = qjob0.result
    result1 = qjob1.result

    print(f"Result from QMIO: {result0}")
    print(f"Result from QMIO: {result1}")

    # 4. Relinquish resources
    qdrop(family)

except Exception as error:
    # 4. Relinquish resources even if an error is raised
    qdrop(family)
    raise error