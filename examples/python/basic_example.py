import os, sys

# Adding path to access CUNQA module
sys.path.append(os.getenv("HOME"))

try: 
    
    # Raising the QPUs
    from cunqa.qpu import qraise

    family = qraise(2, "00:10:00", simulator="Aer", co_located=True)

    # Gettting the raised QPUs
    from cunqa.qpu import get_QPUs

    qpus  = get_QPUs(co_located=True)

    # Creating a circuit to run in our QPUs
    from cunqa.circuit import CunqaCircuit

    qc = CunqaCircuit(num_qubits = 2)
    qc.h(0)
    qc.cx(0,1)
    qc.measure_all()

    # Submitting the same circuit to all vQPUs
    from cunqa.qpu import run

    qcs = [qc] * 4
    qjobs = run(qcs , qpus, shots = 1000)

    # Gathering results
    from cunqa.qjob import gather

    results = gather(qjobs)

    # Getting and printing the counts
    counts_list = [result.counts for result in results]

    for counts in counts_list:
        print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}
        
    # Relinquishing the resources
    from cunqa.qpu import qdrop
    
    qdrop(family)

except Exception as error:
    qdrop(family)
    raise error