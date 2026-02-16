import os, sys
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, run, qraise, qdrop
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

try:
    #noise_path = "complete/path/to/Brisbane.json"
    #family = qraise(4, "00:10:00", simulator="Aer", co_located=True, noise_path=noise_path)
    family = qraise(4, "00:10:00", simulator="Aer", co_located=True, fakeqmio=True)
    qpus  = get_QPUs(co_located=True)

    qc = CunqaCircuit(num_qubits = 2)
    qc.h(0)
    qc.cx(0,1)
    qc.measure_all()

    qcs = [qc] * 4
    qjobs = run(qcs , qpus, shots = 1000)

    results = gather(qjobs)
    counts_list = [result.counts for result in results]

    for counts in counts_list:
        print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}

    qdrop(family)
except Exception as error:
    qdrop(family)
    raise error
