
from qiskit.quantum_info import SparsePauliOp
import os
import sys
import re
installation_path = os.getenv("HOME")
sys.path.append(installation_path)

from qiskit import QuantumCircuit
from qiskit.circuit import Parameter

import numpy as np, warnings

from cunqa import gather

import pandas as pd

np.seterr(divide='raise', invalid='raise')



def Ising_Hamiltonian(n, J):
    """
    Ising Hamiltonian of the form: H= J \sum Z_iZ_i+1, where n is the number of qubits.
    """
    Id = Id = "I"*n
    Hzz = 0
    for i in range(n):
        if i != n-1:
            Hzz += J*SparsePauliOp(Id[:i]+"ZZ"+Id[i+1:-1])
        else: # adding closed cc
            Hzz += J*SparsePauliOp("Z"+Id[:(n-2)]+"Z")
    return Hzz


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
    qc.measure_all()
    return qc


def opa(obs, row, group):
    paul=obs.paulis.to_labels()
    coef=obs.coeffs
    suma = 0.0
    for string in group:
        c = coef[paul.index(string.to_label())]
        st= re.sub(r"[X,Y,Z]", "1", string.to_label())
        st =  re.sub(r"I", "0", st)
        step = (-1)**sum([int(a) * int(b) for a,b in zip(st, row["state"])]) * row["probability"] * c
        suma = suma + step
    return suma.real


def estimate_observable(obs, counts):

    total_counts = sum(list(counts.values()))

    pdf = pd.DataFrame.from_dict(counts, orient="index").reset_index()
    pdf.rename(columns={"index":"state", 0: "counts"}, inplace=True)
    pdf["probability"] = pdf["counts"] /  total_counts
    pdf["enery"] = pdf.apply(lambda x: opa(obs, x, obs.paulis.group_qubit_wise_commuting()[0]), axis=1)

    return sum(pdf["enery"]).real



