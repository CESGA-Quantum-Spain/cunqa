import os, sys
from time import sleep

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import qraise, get_QPUs, run, qdrop
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit

try:
    # 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
    # If GPU execution is desired, just add "gpu = True" as another qraise argument
    family = qraise(2, "00:10:00", simulator = "Aer", co_located = True)

    qpus  = get_QPUs(co_located = True, family = family)

    # ---------------------------
    # 2. Design circuit:
    #  qc.q0   ─[H]───●────[M]─
    #                 |      
    #  qc.q1   ──────[X]───[M]─
    # ---------------------------
    qc = CunqaCircuit(5)
    qc.h(0)
    qc.cx(0, 1)
    qc.measure_all()

    # 3. Execute the same circuit on both deployed QPUs
    qjobs = run([qc, qc], qpus, shots = 10) # non-blocking call


    results = gather(qjobs)

    # Getting the counts
    counts_list = [result.counts for result in results]

    # Printing the counts
    for counts in counts_list:
        print(f"Counts: {counts}")

    # 4. Relinquish resources
    qdrop(family)
    
except Exception as error:
    # 4. Relinquish resources even if an error is raised
    qdrop(family)
    raise error

