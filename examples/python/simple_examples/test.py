import os, sys
import numpy as np
# path to access c++ files
sys.path.append(os.getenv("HOME"))
from qiskit.quantum_info import SparsePauliOp

import qiskit_aer

print(qiskit_aer.__version__)

from cunqa import get_QPUs


import logging

from qmiotools.integrations.qiskitqmio import FakeQmio
from qiskit_aer import AerSimulator


backend = FakeQmio("/opt/cesga/qmio/hpc/calibrations/2025_04_02__12_00_02.json",statevector_parallel_threshold=30, gate_error=True, thermal_relaxation=True, readout_error = True, logging_level = logging.CRITICAL)
backend = AerSimulator()
logging.getLogger("stevedore").setLevel(logging.CRITICAL)

# --------------------------------------------------
# Key difference between cloud and HPC
# example: local = True. By default it is the case.
# This allows to look for QPUs out of the node where 
# the work is executing.
# --------------------------------------------------
qpus  = get_QPUs(local = False, family = "ideal")[0]

from qiskit import QuantumCircuit
from qiskit.circuit import Parameter

def hardware_efficient_ansatz(num_qubits, num_layers):
    qc = QuantumCircuit(num_qubits)
    param_idx = 0
    for _ in range(num_layers):
        for qubit in range(num_qubits):
            phi = Parameter(f'phi_{param_idx}_{qubit}')
            lam = Parameter(f'lam_{param_idx}_{qubit}')
            qc.ry(phi, qubit)
            qc.rz(lam, qubit)
        param_idx += 1
        for qubit in range(num_qubits - 1):
            qc.cx(qubit, qubit + 1)
    return qc

qc = hardware_efficient_ansatz(12,2)

def Heisenberg_Hamiltonian(n, Jx=0, Jy=0, Jz=0, h=0):
    """
    Heisenberg Hamiltonian of the form: H= Jx \sum X_iX_i+1+Jy \sum Y_iY_i+1+Jz \sum Z_iZ_i+1 +h \sum Z_i, where n is the number of qubits.
    """
    Id = Id = "I"*n
    Hzz = 0; Hxx = 0; Hyy = 0; Hz = 0
    for i in range(n):
        if i != n-1:
            Hzz += Jz*SparsePauliOp(Id[:i]+"ZZ"+Id[i+1:-1])
            Hxx += Jx*SparsePauliOp(Id[:i]+"XX"+Id[i+1:-1])
            Hyy += Jy*SparsePauliOp(Id[:i]+"YY"+Id[i+1:-1])
        else: # adding closed cc
            Hzz += Jz*SparsePauliOp("Z"+Id[:(n-2)]+"Z")
            Hxx += Jx*SparsePauliOp("X"+Id[:(n-2)]+"X")
            Hyy += Jy*SparsePauliOp("Y"+Id[:(n-2)]+"Y")

        Hz+= h*SparsePauliOp(Id[:i]+"Z"+Id[i:-1])

    return Hzz+Hyy+Hxx+Hz


def qwc_circuits(circuit, hamiltonian):

    mesurement_op=[]
    # obtenemos los QWC groups
    for groups in hamiltonian.paulis.group_qubit_wise_commuting():
        op='I'*hamiltonian.num_qubits
        for element in groups.to_labels():
            # para cada grupo de conmutación contruimos el operador como una cadena de pauli
            for n,pauli in enumerate(element):
                if pauli !='I':
                    op=op[:n]+str(pauli)+op[int(n+1):]
        mesurement_op.append(op)


    circuits = []

    for paulis in mesurement_op:
        # para cada cadena de pauli
        qubit_op = hamiltonian
        qp=QuantumCircuit(qubit_op.num_qubits)
        index=1
        for j in paulis: # dependiendo de la pauli que se quiera medir, hay que aplicar las rotaciones correspondientes
            if j=='Y':
                qp.sdg(qubit_op.num_qubits-index)# se ponen al revez porque qiskit le da la vuelta a los qubits!
                qp.h(qubit_op.num_qubits-index)
            if j=='X':
                qp.h(qubit_op.num_qubits-index)
            index+=1
        circuits.append(qp)# guardamos los circuitos de medidas

    for i in circuits:
        i.compose(circuit,inplace=True, front=True)# le metemos el ansatz antes a todos ellos (front = True)
        i.measure_all()

    return circuits, hamiltonian.paulis.group_qubit_wise_commuting()



hamil = Heisenberg_Hamiltonian(12,1,1,1,0)

qwc,obs = qwc_circuits(qc, hamil)

from cunqa.converters import convert
from qiskit import transpile
import numpy as np

import qiskit_aer

print(qiskit_aer.__version__)

for qc,ob in zip(qwc,obs):
    qc = qc.assign_parameters([ 0.37627908,  3.07972291, -0.47817286,  0.23895691,  1.55164616,
        0.93409257, -0.64274698, -2.29803965, -0.37568121,  2.77144145,
        1.4183926 , -1.9548902 ,  0.15096985, -0.50812735,  2.53281864,
       -0.30671748, -1.71855476, -2.36063203,  1.51097799, -2.93520292,
        3.05894104, -0.81063625, -0.44274662, -1.90014919, -0.49700511,
        0.6635194 , -1.91406693, -2.42191071,  1.41754333,  0.10576312,
       -0.29135486, -0.77353926,  1.16110796,  1.81993466, -2.2110745 ,
       -2.48571872,  1.56350766, -1.21643614,  1.4156097 ,  2.01423711,
        2.78811598, -0.61840992,  2.41506109,  2.22707238, -2.6849232 ,
        2.94885996, -1.29565719,  2.7079315 ])

    # print(ob)

    shots = 9600000

    print("SHOTS: ", shots)

    Tqc = transpile(qc, backend)
    # print(Tqc.data)

    json_circuit = convert(Tqc, "dict")

    Tqc = convert(json_circuit, "QuantumCircuit")

    print("cunqa: ", qpus.run(Tqc, transpile=False, method = "statevector", shots = shots, seed = 34).result.result["results"][0]["time_taken"])
    print()
    result_qiskit = backend.run(Tqc, method = "statevector", shots = shots, seed_simulator = 34).result()
    print("qiskit: ", result_qiskit.to_dict()["results"][0]["time_taken"])

    # print(result_qiskit)

    print("------------")

    break