"""
Example of the communication noise model: entangled pairs come from a heralded generator after a
random number of attempts, and every qubit decoheres while it waits for them in simulated time.

The vQPUs are deployed with the timing and the communication model of 09-communication_noise.json,
and the ``comm_model`` run argument changes the model for a single run, so the generator can be
swept without deploying again.
"""
import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather
from cunqa.qc_protocols import qsend, qrecv

try:
    file_dir = os.path.dirname(os.path.abspath(__file__))
    backend_path = file_dir + "/09-communication_noise.json"
    # 1. Deploy vQPUs (allocates classical resources for the simulation job) and retrieve them using get_QPUs
    family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located = True, backend=backend_path)
except Exception as error:
    raise error


# CNOTs of 0.3 us before the state is sent: 9 us of local work, close to the mean generation time
# of the deployed generator (10 attempts of 1 us)
LOCAL_WORK = 30

def teleport_plus(prefetch=False):
    """|+> is teleported from the first circuit to the second, which measures it in the X basis:
    the probability of '0' is the fidelity of the teleported state."""
    cc_1 = CunqaCircuit((2, 1), 2, id="First")
    data_1, comm_1 = cc_1.get_qubits()
    cc_2 = CunqaCircuit((2, 1), 3, id="Second")
    data_2, comm_2 = cc_2.get_qubits()

    if prefetch:
        # The pair is requested before the local work, so that it is generated meanwhile
        cc_1.gen_ent(comm_1[0], "Second", "teledata")
        cc_2.gen_ent(comm_2[0], "First", "teledata")

    # |+> to be sent, and local work that leaves it unchanged (X|+> = |+>)
    cc_1.h(data_1[0])
    cc_1.h(data_1[1])
    for _ in range(LOCAL_WORK):
        cc_1.cx(data_1[1], data_1[0])

    qsend(cc_1, data_1[0], comm_1[0], [0, 1], recving_circuit="Second", tag="teledata", prefetched=prefetch)
    qrecv(cc_2, data_2[0], comm_2[0], [0, 1], control_circuit="First", tag="teledata", prefetched=prefetch)

    cc_2.h(data_2[0])
    cc_2.measure(data_2[0], 2)
    return [cc_1, cc_2]


def report(label, result):
    fidelity = result.counts.get("0", 0) / sum(result.counts.values())
    print(f"{label:<28} fidelity {fidelity:.3f}   "
          f"finished at {result.simulated_time['mean'] * 1e6:6.2f} us   "
          f"pair generated in {result.comm_stats['mean_generation_time'] * 1e6:6.2f} us")


try:
    qpus = get_QPUs(co_located=True, family = family)

    # 2. Deployed model: success probability 0.1 per 1 us attempt, requested on demand after the
    #    local work, so the data qubit waits for the whole generation
    results = gather(run(teleport_plus(), qpus, shots=2000))
    report("deployed model", results[1])

    # 3. Sweep of the success probability, changing only that parameter of the model
    for success_probability in [1.0, 0.1, 0.02]:
        model = {"epr": {"success_probability": success_probability}}
        results = gather(run(teleport_plus(), qpus, shots=2000, comm_model=model))
        report(f"success probability {success_probability}", results[1])

    # 4. Prefetching the pair overlaps its generation with the local work: the circuit finishes
    #    earlier and the data qubit waits less, while the pair waits in memory instead
    results = gather(run(teleport_plus(prefetch=True), qpus, shots=2000))
    report("prefetched pair", results[1])

except Exception as error:
    raise error
finally:
    # 5. Relinquish resources: drop the deployed QPUs
    qdrop(family)
