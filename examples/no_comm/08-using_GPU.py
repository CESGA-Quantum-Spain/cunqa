import os, sys
from time import sleep

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import qraise, get_QPUs, run, qdrop
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit

# 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
# If GPU execution is desired, just add "gpu = True" as another qraise argument
family = qraise(1, "00:10:00", cores = 32, simulator = "Aer", co_located = True, gpu = True, gpu_name = "t4", partition = "viz")

try:
    [qpu] = get_QPUs(co_located = True, family = family)

    # 2. Design circuit:
    # ---------------------------
    #  qc.q0   ─[H]───●────[M]─
    #                 |      
    #  qc.q1   ──────[X]───[M]─
    # ---------------------------
    qc = CunqaCircuit(2)
    qc.h(0)
    qc.cx(0, 1)
    qc.measure_all()

    # 3. Execute the same circuit on both deployed QPUs
    qjob = run(qc, qpu, shots = 1000) # non-blocking call

    # Getting the counts
    print(f"Counts: {qjob.result.result}")
    
except Exception as error:
    raise error
finally:
    # 4. Release classical resources
    qdrop(family)
