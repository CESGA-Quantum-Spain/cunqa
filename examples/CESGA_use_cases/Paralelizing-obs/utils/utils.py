
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

def Ising_Hamiltonian(n, J, g):
    """
    Ising Hamiltonian of the form: H= J \sum Z_iZ_i+1 +g \sum X_i, where n is the number of qubits.
    """
    Id = Id = "I"*n
    Hx = 0; Hzz = 0
    for i in range(n):
        if i != n-1:
            Hzz += J*SparsePauliOp(Id[:i]+"ZZ"+Id[i+1:-1])
        else: # adding closed cc
            Hzz += J*SparsePauliOp("Z"+Id[:(n-2)]+"Z")
        Hx += g*SparsePauliOp([Id[:i]+"X"+Id[i:-1]])
    return Hzz+Hx


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


def opa(hamil, row, group):
    coef = hamil.coeffs
    paul = hamil.paulis.to_labels()
    suma = 0.0
    for string in group:
        c = coef[paul.index(string.to_label())]
        st= re.sub(r"[X,Y,Z]", "1", string.to_label())
        st =  re.sub(r"I", "0", st)
        step = (-1)**sum([int(a) * int(b) for a,b in zip(st, row["state"])]) * row["probability"] * c
        suma = suma + step
    return suma.real


def estimate_observable(hamil,obs, counts):

    total_counts = sum(list(counts.values()))

    pdf = pd.DataFrame.from_dict(counts, orient="index").reset_index()
    pdf.rename(columns={"index":"state", 0: "counts"}, inplace=True)
    pdf["probability"] = pdf["counts"] /  total_counts
    pdf["enery"] = pdf.apply(lambda x: opa(hamil,x,obs), axis=1)

    return sum(pdf["enery"]).real

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

