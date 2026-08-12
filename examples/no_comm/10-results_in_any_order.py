import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import qraise, get_QPUs, run, qdrop
from cunqa.qjob import gather
from cunqa.circuit import CunqaCircuit

green_txt = '\033[92m'; yellow_txt = '\033[93m'; reset = '\033[0m'

# All the jobs sent to the same vQPU share one connection, so the results of all of them arrive
# through the same channel. To know which result belongs to which job, every quantum task travels
# with the id of its circuit and the vQPU stamps that id back on the result. When the result that
# arrives is not the one being asked for, it is stored and handed over later to the job that owns
# it, so results can be asked for in any order, not only in the one in which they were submitted.

SHOTS = 1000
NUM_QUBITS = 3

try:
    # 1. Deploy a single vQPU, so that every job goes through the same connection
    # If GPU execution is desired, just add "gpu = True" as another qraise argument
    family = qraise(1, "00:10:00", co_located = True)
except Exception as error:
    raise error

try:
    [qpu] = get_QPUs(co_located = True, family = family)

    # 2. Design one circuit per number of qubits to excite, each one with a deterministic outcome
    # so that its result can be told apart just by looking at the counts:
    # -----------------------------------------------------
    #  2_excited.q0   ─[X]──[M]─
    #
    #  2_excited.q1   ─[X]──[M]─   ->  measured as "011" (2 excited qubits)
    #
    #  2_excited.q2   ──────[M]─
    # -----------------------------------------------------
    circuits = []
    for num_excited in range(1, NUM_QUBITS + 1):
        circuit = CunqaCircuit(NUM_QUBITS, id = f"{num_excited}_excited")
        for qubit in range(num_excited):
            circuit.x(qubit)
        circuit.measure_all()
        circuits.append(circuit)

    # 3. Send the circuits one after the other to the same vQPU. Since run is a non-blocking call,
    # the three quantum tasks end up queued at the vQPU, which simulates them in order
    qjobs = [run(circuit, qpu, shots = SHOTS) for circuit in circuits]

    # 4. Ask for the results in the OPPOSITE order to the one in which they were submitted. The
    # result of the last job is the last one to arrive, so the two results that come before it are
    # stored, and the two remaining jobs take them from there instead of reading the connection
    print(f"{yellow_txt}Results asked for in reverse order:{reset}")
    for qjob in reversed(qjobs):
        result = qjob.result

        # The bit string measured tells which circuit was really simulated: the amount of ones in
        # it must be the number of qubits that this circuit, and no other, excites
        bitstring = max(result.counts, key = result.counts.get)
        num_excited = int(result.id.split("_")[0])

        if bitstring.count("1") != num_excited:
            raise RuntimeError(f"The result of {result.id} got mixed up with the one of another "
                               f"job: {bitstring} was measured, but {num_excited} qubits were "
                               f"excited.")

        print(f"    {green_txt}{result.id}{reset}: {result.counts}")

    # 5. The same happens with gather, no matter the order of the list of jobs given to it
    qjobs = [run(circuit, qpu, shots = SHOTS) for circuit in circuits]
    results = gather(list(reversed(qjobs)))

    print(f"{yellow_txt}Results gathered from the reversed list of jobs:{reset}")
    for result in results:
        print(f"    {green_txt}{result.id}{reset}: {result.counts}")

except Exception as error:
    raise error
finally:
    # 6. Release classical resources
    qdrop(family)
