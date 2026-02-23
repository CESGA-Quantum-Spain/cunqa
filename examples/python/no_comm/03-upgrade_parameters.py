import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit

import numpy as np

try:
    # 1. Deploy QPU
    # If GPU execution is desired, just add "gpu = True" as another qraise argument
    family = qraise(1, "00:10:00",  co_located = True)
    qpu = get_QPUs(co_located = True, family = family)

    # ---------------------------
    # 2. Design circuit:
    #  circ_upgrade.q0   ─[RX(cos(x))]─────[M]─
    #                        
    #  circ_upgrade.q1   ─[RX(y)]──────────[M]─
    #
    #  circ_upgrade.q2   ─[RX(z)]──────────[M]─
    # ---------------------------
    circ_upgrade = CunqaCircuit(3)
    circ_upgrade.rx("cos(x)", 0)
    circ_upgrade.rx("y", 1)
    circ_upgrade.rx("z", 2)
    circ_upgrade.measure_all()

    # 3. Execute circuit
    qjob = run(circ_upgrade, qpu, param_values={"x": np.pi, "y": 0, "z": 0}, shots=1024)
    print(f"Result 0: {qjob.result.counts}")

    # Upgrade with dicts
    qjob.upgrade_parameters({"x": 0, "y": 0, "z": 0})
    print(f"Result 1: {qjob.result.counts}")

    # Upgrade with a dict with only some of the Variables (previous values are preserved)
    qjob.upgrade_parameters({"x": 0, "y": np.pi})
    print(f"Result 2: {qjob.result.counts}")

    # Now with a list
    qjob.upgrade_parameters([0, 0, np.pi])
    print(f"Result 3: {qjob.result.counts}")

    # 4. Relinquish resources
    qdrop(family)

except Exception as error:
    qdrop(family)
    raise error
