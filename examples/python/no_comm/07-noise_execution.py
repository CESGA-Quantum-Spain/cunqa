import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, run, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

try:
    # 1. Deploy noisy vQPUs
    #noise_path = "complete/path/to/Brisbane.json"
    #family = qraise(4, "00:10:00", simulator="Aer", co_located=True, noise_path=noise_path)
    family = qraise(4, "00:10:00", simulator="Aer", co_located=True, fakeqmio=True)
    qpus  = get_QPUs(co_located=True)

    # 2. Design circuit as any other execution
    qc = CunqaCircuit(num_qubits = 2)
    qc.h(0)
    qc.cx(0,1)
    qc.measure_all()

    # 3. Execute circuit on noisy QPUs
    qcs = [qc] * 4
    qjobs = run(qcs , qpus, shots = 1000)

    results = gather(qjobs)
    counts_list = [result.counts for result in results]

    for counts in counts_list:
        print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}

    # 4. Relinquish resources
    qdrop(family)

except Exception as error:
    # 4. Relinquish resources even if an error is raised
    qdrop(family)
    raise error
