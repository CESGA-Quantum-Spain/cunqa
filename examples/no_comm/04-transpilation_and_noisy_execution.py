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
    backend_path = file_dir + "/noisy_backend.json"
    
    family = qraise(1, "00:10:00", simulator="Aer", co_located=True, backend=backend_path)
except Exception as error:
    raise error

try:
    [qpu]  = get_QPUs(co_located=True)

    # 2. Design circuit as any other execution
    qc = CunqaCircuit(num_qubits = 2)
    qc.h(0)
    qc.cx(0,1)
    qc.measure_all()

    # 3. Transpilation. Required for execution on noisy QPUs
    qc_transpiled = transpiler(qc, qpu.backend, opt_level = 2, initial_layout = None, seed = None)
    
    # 4. Execution
    qjob = run(qc_transpiled, qpu, shots = 1000)

    print(f"Counts: {qjob.result.counts}" ) # Format: {'00':546, '11':454}
 
    # 5. Relinquish resources
    qdrop(family)

except Exception as error:
    # 5. Relinquish resources even if an error is raised
    qdrop(family)
    raise error

