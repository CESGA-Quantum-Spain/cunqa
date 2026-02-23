import os, sys

# Adding path to access CUNQA module
sys.path.append(os.getenv("HOME")) 
# For custom CUNQA installation path, for instance if you are in an HPC center other than CESGA, use
# sys.path.append("</your/cunqa/installation/path>")   

try: 
    
    # 1. Deploy 3 virtual QPUs
    from cunqa.qpu import qraise
    family = qraise(3, "00:10:00", simulator="Aer", co_located=True)

    #    Connect to the raised vQPUs
    from cunqa.qpu import get_QPUs
    qpus  = get_QPUs(co_located=True)

    # 2. Design a circuit to run in your vQPUs
    from cunqa.circuit import CunqaCircuit

    qc = CunqaCircuit(num_qubits = 2)
    qc.h(0)
    qc.cx(0,1)
    qc.measure_all()

    # 3. Execute the same circuit on all 3 vQPUs
    from cunqa.qpu import run
    qjobs = run([qc, qc, qc] , qpus, shots = 1000)

    #    Gather results
    from cunqa.qjob import gather
    results = gather(qjobs)

    #    Printing the counts
    for result in results:
        print(f"Counts: {result.counts}" ) # Format: {'00':546, '11':454}
        
    # 4. Relinquishing resources
    from cunqa.qpu import qdrop
    qdrop(family)

except Exception as error:
    # 4. Relinquishing resources even if an error occurs
    from cunqa.qpu import qdrop
    qdrop(family)

    raise error