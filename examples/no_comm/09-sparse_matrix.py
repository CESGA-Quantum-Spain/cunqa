import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit

import numpy as np

try:
    # 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
    # The sparsematrix instruction is supported by the Qulacs simulator
    family = qraise(1, "00:10:00", simulator="Qulacs", co_located = True)
except Exception as error:
    raise error

try:
    [qpu] = get_QPUs(co_located=True, family = family)

    # 2. Design circuit:
    # ---------------------------------
    #  qc.q0   ─[H]──┬─[SWAP]─┬──[M]─
    #                |        |
    #  qc.q1   ──────┴─[SWAP]─┴──[M]─
    # ---------------------------------
    # A SWAP gate is a permutation unitary: most of its entries are zero, so it is
    # a natural candidate for the sparsematrix instruction. We provide it as a dense
    # 4x4 numpy array; "sparse" here refers to its structure, not the storage type.
    SWAP = np.array([[1, 0, 0, 0],
                     [0, 0, 1, 0],
                     [0, 1, 0, 0],
                     [0, 0, 0, 1]], dtype=complex)

    qc = CunqaCircuit(2, 2, id="qc")
    qc.h(0)
    qc.sparsematrix(SWAP, 0, 1)
    qc.measure(0, 0)
    qc.measure(1, 1)

    # 3. Execute circuit and get the results
    # H puts q0 in superposition; the SWAP moves that superposition onto q1. We expect
    # two basis states with roughly equal probability (~50% each over 1024 shots).
    qresult = run(qc, qpu, shots=1024).result
    print(f"Result: {qresult}")

except Exception as error:
    raise error
finally:
    # 4. Relinquish resources
    qdrop(family)
