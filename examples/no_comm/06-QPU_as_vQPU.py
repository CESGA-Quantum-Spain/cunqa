import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import  qraise, get_QPUs, run, qdrop
from cunqa.circuit import CunqaCircuit

# 1. Deploy the QMIO quantum computer as if it were another vQPU
family = qraise(1, "00:10:00", qmio = True, family = "qmio_job")

try:
    [qmio] = get_QPUs(co_located = True, family = family)

    # 2. Design circuit:
    # ------------------------------------------
    #  circuit.q0   ─[H]────●───[RZ(1.555)]─[M]─
    #                       |
    #  circuit.q1   ───────[X]──────────────[M]─
    # ------------------------------------------
    circuit = CunqaCircuit(2)
    circuit.h(0)
    circuit.cx(0,1)
    circuit.rz(1.555, 0)
    circuit.measure_all()

    # 3. Execute circuit on QMIO.
    #    Both jobs are submitted without blocking. Opposite to the results of a vQPU, which are
    #    identified by the id of their circuit and can be retrieved in any order, the ones coming
    #    from QMIO carry no id, so they must be retrieved in the same order in which the jobs
    #    were submitted.
    qjob0 = run(circuit, qmio, shots = 100)
    qjob1 = run(circuit, qmio, shots = 100)

    result0 = qjob0.result
    result1 = qjob1.result

    print(f"Result from QMIO: {result0}")
    print(f"Result from QMIO: {result1}")

except Exception as error:
    raise error
finally:
    # 4. Relinquish resources
    qdrop(family)
