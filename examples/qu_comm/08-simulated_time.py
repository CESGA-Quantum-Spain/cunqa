"""
Example of simulated time with quantum communications: entanglement generation synchronizes
the clocks of the vQPUs involved, and classical messages arrive after the modeled latency.
"""
import os, sys

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

try:
    # 1. Deploy vQPUs with simulated time. Every vQPU must define the "timing" block, since they
    #    share entangled states and messages. In seconds: 1-qubit gates 0.05 us, measurements and
    #    resets 1 us, entanglement generation 2 us and 0.5 us of latency per classical message.
    #    The times below are in us, while the result reports them in seconds.
    file_dir = os.path.dirname(os.path.abspath(__file__))
    backend_path = file_dir + "/08-simulated_time.json"

    family = qraise(2, "00:10:00", simulator="Aer", quantum_comm=True, co_located=True, backend=backend_path)
except Exception as error:
    raise error

try:
    qpus = get_QPUs(co_located=True, family=family)

    # 2. Design circuits. Bob's comm qubit is busy with a reset when Alice asks for the Bell pair,
    #    so the entanglement generation starts once both comm qubits are free (t = 1 us) and
    #    the pair is ready on both vQPUs at the same time (t = 3 us). Then Alice measures her
    #    half of the pair and sends the outcome, and Bob flips his data qubit if it is 1.
    # -------------------------------------------------------------------------------------------
    #  Alice.data  ─[H]─[M]──────────────────────────  H: [0, 0.05)  M: [0.05, 1.05)
    #
    #  Alice.comm  ────────╌ gen_ent ╌─[M]─ send ─>    idles 1 us waiting for Bob's comm qubit,
    #                          ╎                       pair: [1, 3)  M: [3, 4),
    #                          ╎                       sent at 4 us -> every shot lasts 4 us
    #  Bob.comm    ─[reset]─╌ gen_ent ╌─[M]──────      reset: [0, 1)  pair: [1, 3)
    #                                                  M: [3, 4)
    #  Bob recv    <─ arrives at 4 + 0.5 = 4.5 us
    #  Bob.data    ─────────────────────[X]─[M]──      if bit == 1: idles 4.5 us, X: [4.5, 4.55),
    #                                                               M: [4.55, 5.55) -> 5.55 us
    #                                                  if bit == 0: M: [0, 1), the shot ends when
    #                                                               the message arrives -> 4.5 us
    # -------------------------------------------------------------------------------------------
    #  Expected (idle_mean is [data, comm]):
    #    Alice: min = max = mean = 4 us, idle_mean = [0, 1 us]
    #    Bob:   min 4.5 us, max 5.55 us, mean ~5.025 us, idle_mean ~[2.25 us, 0]
    alice = CunqaCircuit((1, 1), 2, id="Alice")
    a_data, a_comm = alice.get_qubits()

    bob = CunqaCircuit((1, 1), 3, id="Bob")
    b_data, b_comm = bob.get_qubits()

    alice.h(a_data[0])
    alice.gen_ent(a_comm[0], "Bob", tag="epr")
    alice.measure(a_comm[0], 0)
    alice.send(0, recving_circuit="Bob")
    alice.measure(a_data[0], 1)

    bob.reset(b_comm[0])
    bob.gen_ent(b_comm[0], "Alice", tag="epr")
    bob.measure(b_comm[0], 0)
    bob.recv(1, sending_circuit="Alice")
    bob.cif(1)
    bob.x(b_data[0])
    bob.endcif()
    bob.measure(b_data[0], 2)

    # 3. Execute distributed circuits on QPUs with quantum communications
    distr_jobs = run([alice, bob], qpus, shots=1000)
    results = gather(distr_jobs)

    # Bob's two saved bits are always equal ('00' or '11'): his comm qubit is entangled with
    # Alice's, and his data qubit copies the bit she measured.
    for result, name in zip(results, ["Alice", "Bob"]):
        print(name)
        print(f"\tCounts: {result.counts}")
        print(f"\tWall-clock time: {result.time_taken} s")
        print(f"\tSimulated time: {result.simulated_time}")

except Exception as error:
    raise error
finally:
    # 4. Relinquish resources
    qdrop(family)
