import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit


try:
    # 1. Deploy vQPUs
    family = qraise(1, "01:00:00", simulator = "Aer", co_located = True)
except Exception as error:
    raise error

try:
    [qpu] = get_QPUs(co_located = True, family = family)


    # 2. Design circuit:
    # ---------------------------
    #  qc.q0   ─[H]───[M]───[M]─
    #                  ‖     
    #  qc.q1   ───────[X]───[M]─
    # ---------------------------
    qc = CunqaCircuit(5, 5)
    qc.x(0)
    qc.measure(0, 0)
    qc.measure(1, 1)

    # "and" option: gate is applied if all bits match the condition (=1)
    # as bit 1 is zero, this gate won't be applied (standard MULTICONTROLLED)
    with qc.cif([0,1], operation="and") as cgates:
        cgates.x(2)

    # "or" option: gate is applied if any bit matches the condition (=1)
    # bit 0 is set to one, the gate will be applied
    with qc.cif([0,1], operation="or") as cgates:
        cgates.x(3)

    # "xor" option: gate is applied if there is an odd number of matching (=1) bits
    # only bit 0 is one, the gate will be applied
    with qc.cif([0,1], operation="xor") as cgates:
        cgates.x(4)

    # any of the options work with condition=0
    # bit 0 is one, so not all are zero, so it won't be applied
    with qc.cif([0,1], operation="and", condition=0) as cgates:
        cgates.x(2)

    qc.measure(0,0)
    qc.measure(1,1)
    qc.measure(2,2)
    qc.measure(3,3)
    qc.measure(4,4)

    # 3. Execute circuit on vQPU
    qjob = run(qc, qpu, shots = 1024)
    counts = qjob.result.counts

    print("Counts: ", counts)

    # 4. Relinquish resources
    qdrop(family)

except Exception as error:
    qdrop(family)
    raise error