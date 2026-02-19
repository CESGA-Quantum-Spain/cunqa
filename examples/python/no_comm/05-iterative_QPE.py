import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.circuit import CunqaCircuit  
from cunqa.qpu import qraise, get_QPUs, run, qdrop

import numpy as np

# 1. Circuit design
parametric_circuit = CunqaCircuit(3, 1)
parametric_circuit.h(0)
parametric_circuit.rx("x1", 1)
parametric_circuit.rx("x2", 2)
parametric_circuit.crz("theta", 0, 1)
parametric_circuit.crz("theta", 0, 2)
parametric_circuit.p("phase", 0)
parametric_circuit.h(0)
parametric_circuit.measure(0, 0)


N_QPUS = 10                  # Determines the number of bits of the phase that will be computed
PHASE_TO_COMPUTE = 1/2**5 
SHOTS = 1024
BIT_PRECISION = 30
SEED = 18                   # Set seed for reproducibility


# Parameters initial values
theta = 2 * np.pi * PHASE_TO_COMPUTE
x1 = np.pi
x2 = np.pi

try:
    # 1. Deploy vQPUs
    family = qraise(10, "00:10:00", simulator = "Munich", co_located = True)
    qpus  = get_QPUs(co_located = True, family = family)

    
    measure = []
    for k in range(BIT_PRECISION):

        # 2. Circuit design
        power = 2**(BIT_PRECISION - 1 - k)

        phase = 0
        for i in range(k):
            if measure[i]:
                phase = phase + (1 / (2**(k-i)))
        phase = -np.pi * phase

        params = {
            "theta": power * theta, 
            "phase": phase, 
            "x1": x1, 
            "x2": x2
        }
        
        # 3. Execution 
        result = run(parametric_circuit, qpus[k%10], params, shots = 2000, seed = SEED).result
        counts = result.counts

        zeros = counts.get("0", 0)
        ones = counts.get("1", 0)
        measure.append(0 if zeros > ones else 1) 
                

    # 4. Post processing results
    estimation = 0
    for j,l in enumerate(measure):
        estimation = estimation + (l/(2**(BIT_PRECISION - j)))

    print(f"Estimated angle: {estimation}")
    print(f"Real angle: {PHASE_TO_COMPUTE}")


    # 5. Release resources
    qdrop(family)

except Exception as error:
    qdrop(family)
    raise error