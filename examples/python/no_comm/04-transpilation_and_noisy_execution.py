import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, run, qraise, qdrop
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit
from cunqa.qiskit_deps.transpiler import transpiler

try:
    # 1. Deploy noisy vQPUs
    file_dir = os.path.dirname(os.path.abspath(__file__))
    noise_properties_path = file_dir + "/noise_properties_example.json"
    
    family = qraise(4, "00:10:00", simulator="Aer", co_located=True, noise_properties_path=noise_properties_path)
    qpus  = get_QPUs(co_located=True)

    # 2. Design circuit as any other execution
    qc = CunqaCircuit(num_qubits = 2)
    qc.h(0)
    qc.cx(0,1)
    qc.measure_all()

    # 3. Transpilation. Required for execution on noisy QPUs
    qc_transpiled = transpiler(qc, qpus[-1].backend, opt_level = 2, initial_layout = None, seed = None)

    # 4. Execution
    qcs = [qc_transpiled] * 4
    qjobs = run(qcs, qpus, shots = 1000)

    results = gather(qjobs)
    counts_list = [result.counts for result in results]

    for counts in counts_list:
        print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}

    # 5. Relinquish resources
    qdrop(family)

except Exception as error:
    # 5. Relinquish resources even if an error is raised
    qdrop(family)
    raise error

