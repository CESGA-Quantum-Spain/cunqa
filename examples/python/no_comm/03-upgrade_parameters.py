import os, sys
# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit

import numpy as np

circ_upgrade = CunqaCircuit(3)
circ_upgrade.rx("cos(x)", 0)
circ_upgrade.rx("y", 1)
circ_upgrade.rx("z", 2)
circ_upgrade.measure_all()

# If GPU execution is desired, just add "gpu = True" as another qraise argument
family = qraise(1, "00:10:00",  co_located = True)
qpu = get_QPUs(co_located = True, family = family)

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

qdrop(family)
