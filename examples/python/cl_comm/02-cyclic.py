import os, sys
import numpy as np
import matplotlib.pyplot as plt

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather

try:
    # 1. QPU deployment

    NUM_NODES = 3

    # If GPU execution is desired, just add "gpu = True" as another qraise argument
    family_name = qraise(NUM_NODES,"00:10:00", simulator="Maestro", classical_comm=True, co_located = True)
    qpus = get_QPUs(co_located = True, family = family_name)


    # 2. Circuit design

    # We want to achieve the following scheme:
    # --------------------------------------------------------
    #                         ══════════════════
    #                         ‖                 ‖
    #  circuit0.q0: ─────────[X]──────[M]─      ‖
    #                                           ‖
    #  circuit0.q1: ───[H]───[M]─────────       ‖
    #                         ‖                 ‖
    #                         ‖                 ‖
    #  circuit1.q0: ─────────[X]──────[M]─      ‖
    #                                           ‖
    #  circuit1.q1: ───[H]───[M]──────────      ‖
    #                         ‖                 ‖
    #                         :                 ‖
    #                         :                 ‖
    #                         ‖                 ‖
    #  circuitn.q0: ─────────[X]──────[M]─      ‖
    #                                           ‖
    #  circuitn.q1: ───[H]───[M]──────────      ‖
    #                         ‖                 ‖
    #                         ══════════════════
    # ----------------------------------------------------

    classcal_comms_circuits = []

    for i in range(NUM_NODES):

        circuit = CunqaCircuit(2,2, id = str(i))

        # Here we prepare a superposition state at qubit 1, we measure and send its result to the next circuit
        circuit.h(1)
        circuit.measure(1,1)
        circuit.send(1, recving_circuit = str(i+1) if (i+1) != NUM_NODES else str(0))

        # Here we recieve the bit sent by the prior circuit and use it for conditioning an x gate at qubit 0
        circuit.recv(0, sending_circuit = str(i-1) if (i-1) != -1 else str(NUM_NODES-1))
        with circuit.cif(clbits = 0) as cgates:
            cgates.x(0)

        # Adding final measurement of que qubit after the x gate
        circuit.measure(0,0)

        classcal_comms_circuits.append(circuit)


    # 3. Execution

    # The output bitstrings are "switched" in orther, clbit 0 corresponds to the last bit of the bitstring.
    # We expect then for the first bit of a circuit's result to be equal to the last of the next circuit.

    # If we set the execution to have more shots, this have to be checked as:
    #
    #   If, for circuit0 we have {'(0)0': 5, '10': 4, '11': 1}, we see that there are 5 cases in which the first
    #   qubit is `0`.
    #
    #   We spect output at circuit1 to have a total of 5 cases in which the second qubit is `0`, therefore:
    #   {'0(0)': 1, '01': 3, '1(0)': 4, '11': 2} is correct since 1+4 = 5 .

    distr_jobs = run(classcal_comms_circuits, qpus, shots=1)

    results_list = gather(distr_jobs)

    for i, result in enumerate(results_list):
        print(f"For circuit {i}: ", result.counts)


    # 4. Release classical resources
    qdrop(family_name)

except Exception as error:
    qdrop(family_name)
    raise error
