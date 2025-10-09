import sys

from utils.utils import *
import time as time
import re
import pandas as pd
import logging
from cunqa import transpiler, get_QPUs, gather
from cunqa.mappers import QJobMapper, QPUCircuitMapper
import numpy as np
from multiprocessing.pool import Pool
from concurrent.futures import ThreadPoolExecutor
from qiskit_aer import AerSimulator
from qiskit import transpile
aersim = AerSimulator()

from qmiotools.integrations.qiskitqmio import FakeQmio
fqmio=FakeQmio(gate_error=True, readout_error=True)


from scipy.optimize import differential_evolution

def perturbate_params(init_params):
    param = []
    for par in init_params:
        mod = par - (2e-2) * np.random.random() + (2e-2) * np.random.random()
        if mod < -np.pi:
            param.append(mod+2*np.pi)
        elif mod > np.pi:
            param.append(mod-2*np.pi)
        else:
            param.append(mod)
    return param

class DifferentialEvolutionHVA:
    """
    Class to execute a VQE problem using the evolutive optimizator differential evolution and with Hamiltonian Variational Ansatz.
    """

    def __init__(self,
                 Hamiltonian = None,
                 layers = 2,
                 shots = 1024,
                 method = "shots",
                 backend = "aer"):
        
        self.Hamiltonian = Hamiltonian
        self.n_qubits = self.Hamiltonian.num_qubits
        self.layers = layers
        self.shots = shots
        self.evaluations = 0
        self.method = method
        self.backend = backend

        # defining problem parameters
        n = self.Hamiltonian.num_qubits

        # generating variational ansatz
        ansatz = hva(self.Hamiltonian, self.layers)

        # creating circuit
        qc = QuantumCircuit(n)

        # initial state to |-\^n
        for i in range(n):
            qc.h(i)
        
        qc.barrier()

        # compose with ansatz
        qc.compose(ansatz, inplace=True)

        self.qc = qc.copy()

        # create qcw circuits transpiled 

        mesurement_op=[]
        # obtenemos los QWC groups
        for groups in self.Hamiltonian.paulis.group_qubit_wise_commuting():
            op='I'*self.Hamiltonian.num_qubits
            for element in groups.to_labels():
                # para cada grupo de conmutación contruimos el operador como una cadena de pauli
                for n,pauli in enumerate(element):
                    if pauli !='I':
                        op=op[:n]+str(pauli)+op[int(n+1):]
            mesurement_op.append(op)
        
        # tomamos el ansatz (hay que comprobar que no afecten las barreras... con una funcion se quitan fácil)
        qc = self.qc.copy()

        circuit = []

        for paulis in mesurement_op:
            # para cada cadena de pauli
            qubit_op = self.Hamiltonian
            qp=QuantumCircuit(qubit_op.num_qubits)
            index=1
            for j in paulis: # dependiendo de la pauli que se quiera medir, hay que aplicar las rotaciones correspondientes
                if j=='Y':
                    qp.sdg(qubit_op.num_qubits-index)# se ponen al revez porque qiskit le da la vuelta a los qubits!
                    qp.h(qubit_op.num_qubits-index)
                if j=='X':
                    qp.h(qubit_op.num_qubits-index)
                index+=1
            circuit.append(qp)# guardamos los circuitos de medidas

        for i in circuit:
            i.compose(qc,inplace=True, front=True)# le metemos el ansatz antes a todos ellos (front = True)
            i.measure_all()

        # defino la lista de qwc circuits para que me quede más claro
        self.qwc_circuits = circuit


        if backend == "fakeqmio":
            print()
            print()
            print()
            print("Transpiling!!!!!!!")
            print()
            print()
            print()
            self.qwc_circuits = transpile(circuit, fqmio, seed_transpiler=1,optimization_level=2, initial_layout = [15,16,25,26])

        elif backend == "cunqa":
            self.qwc_circuits = []
            self.qpus = get_QPUs(local = True)
            for c in circuit:
                self.qwc_circuits.append(transpiler(c, self.qpus[0].backend,seed_transpiler=1,optimization_level=2, initial_layout = [15,16,25,26]))
            
        else:
            self.qwc_circuits = circuit

        self.paul=self.Hamiltonian.paulis.to_labels()
        self.coef=self.Hamiltonian.coeffs
        self.num_parameters = self.qc.num_parameters

    def opa(self,row, group):
        suma = 0.0
        for string in group:
            c = self.coef[self.paul.index(string.to_label())]
            st= re.sub(r"[X,Y,Z]", "1", string.to_label())
            st =  re.sub(r"I", "0", st)
            step = (-1)**sum([int(a) * int(b) for a,b in zip(st, row["state"])]) * row["probability"] * c
            #print(string, st, pdf["state"].iloc[0], step, c)
            suma = suma + step
        return suma.real

    def cost_sv(self,params):
        """
        Function that simulates the HVA associated with the given `hamiltonian` and estimates the energy of the resulting state.
        
        For now the circuit is initialized to state |->^n
        """

        qc = self.qc.decompose().decompose().assign_parameters(params)

        TICK = time.time()
        # at first we are goint to user statevector
        state = Statevector.from_instruction(qc)
        # print("\tStatevector: ",state)
        TACK = time.time()


        print(f"({time.strftime('%H:%M:%S')}) evaluation State Vector time = {TACK-TICK}")

        return state.expectation_value(self.Hamiltonian).real
    
    def cost_shots_aer(self,params):
        lista = []
        TICK = time.time()
        for n,q in enumerate(self.qwc_circuits):
            circuit_assembled=q.decompose().decompose().assign_parameters(params)
            result=aersim.run(circuit_assembled,shots=self.shots).result()
            self.evaluations += 1
            counts=result.get_counts()
            pdf = pd.DataFrame.from_dict(counts, orient="index").reset_index()
            pdf.rename(columns={"index":"state", 0: "counts"}, inplace=True)
            pdf["probability"] = pdf["counts"] / self.shots 
            pdf["enery"] = pdf.apply(lambda x: self.opa(x, self.Hamiltonian.paulis.group_qubit_wise_commuting()[n]), axis=1)
            #print(counts)
            lista.append(sum(pdf["enery"]).real)
        
        TACK = time.time()

        print(f"({time.strftime('%H:%M:%S')}) evaluation shots time = {TACK-TICK}")

        return(sum(lista).real)
    
    def cost_shots_fq(self,params):
        print("About to calculate cost!")
        lista = []
        TICK = time.time()
        for n,q in enumerate(self.qwc_circuits):
            circuit_assembled=q.assign_parameters(params)
            result=fqmio.run(circuit_assembled,shots=self.shots).result()
            self.evaluations += 1
            counts=result.get_counts()
            pdf = pd.DataFrame.from_dict(counts, orient="index").reset_index()
            pdf.rename(columns={"index":"state", 0: "counts"}, inplace=True)
            pdf["probability"] = pdf["counts"] / self.shots 
            pdf["enery"] = pdf.apply(lambda x: self.opa(x, self.Hamiltonian.paulis.group_qubit_wise_commuting()[n]), axis=1)
            #print(counts)
            lista.append(sum(pdf["enery"]).real)
        
        TACK = time.time()

        print(f"({time.strftime('%H:%M:%S')}) evaluation shots time = {TACK-TICK}")

        return(sum(lista).real)


    def cost_shots(self, results):
        """
        Function to estimate the cost value from the results of a simulation.
        """
        lista = []
        for n,r in enumerate(results):
            self.evaluations +=1
            counts=r.counts

            pdf = pd.DataFrame.from_dict(counts, orient="index").reset_index()
            pdf.rename(columns={"index":"state", 0: "counts"}, inplace=True)
            pdf["probability"] = pdf["counts"] / self.shots 
            pdf["enery"] = pdf.apply(lambda x: self.opa(x, self.Hamiltonian.paulis.group_qubit_wise_commuting()[n]), axis=1)
            #print(counts)
            lista.append(sum(pdf["enery"]).real)

        return(sum(lista).real)


    def optimize(self,
                 init_params = None,
                 mapper = None,
                 bounds = None,
                 max_iter = 1000,
                 defered=True,
                 pop=1,
                 seed=int(1),
                 tol=1e-12):
        
        pop = 1 if self.layers*2 > 5 else 2


        if mapper is None:
            workers = 1

            if self.method == "shots":
                if self.backend == "aer":
                    self.cost_fn = self.cost_shots_aer
                elif self.backend == "fakeqmio":
                    self.cost_fn = self.cost_shots_fq

            elif self.method == "statevector":
                self.cost_fn = self.cost_sv

        elif isinstance(mapper, QJobMapper) or isinstance(mapper, QPUCircuitMapper):

            workers = mapper

            if self.method == "shots":
                if self.backend == "aer":
                    self.cost_fn = self.cost_shots_aer
                elif self.backend == "fakeqmio":
                    self.cost_fn = self.cost_shots_fq
            
            elif self.method == "statevector":
                raise Exception("statevector not availabel with cunqa backend!")
            
        elif isinstance(mapper, Pool) or isinstance(mapper, ThreadPoolExecutor):

            workers = mapper.map

            print("mapper object created")

            if self.method == "shots":
                if self.backend == "aer":
                    self.cost_fn = self.cost_shots_aer
                elif self.backend == "fakeqmio":
                    self.cost_fn = self.cost_shots_fq

            elif self.method == "statevector":
                self.cost_fn = self.cost_sv
                

        if bounds is None:
            bounds=[]
            for i in range(0,self.qc.num_parameters):
                bounds.append((-np.pi,np.pi))

        if init_params is None:
            init = []
            for j in range(pop*self.qc.num_parameters):
                init.append(1e-2*np.random.random(self.qc.num_parameters)-2e-2)
        else:
            init = []
            init.append(init_params)
            for j in range(pop*self.qc.num_parameters-1):
                init.append(perturbate_params(init_params))
            

        opt_path = []; params_path = [init_params]

        def cb(xk, convergence = tol):
            print(f"({time.strftime('%H:%M:%S')})")
            cost = self.cost_fn(xk)
            params_path.append(list(xk))
            opt_path.append(cost)


        # print(f"({time.strftime('%H:%M:%S')}) Starting optimization...")
        tick = time.time()
        optimizer = differential_evolution(func = self.cost_fn,
                                            popsize=pop,
                                            strategy='best1bin',
                                            bounds=bounds,
                                            maxiter=max_iter,
                                            disp=defered,
                                            init="halton", # halton, x0
                                            polish=False,
                                            tol=tol,
                                            seed=seed,
                                            callback=cb,
                                            workers=workers,
                                            updating='deferred')


        tack = time.time()
        if i <= 300:
            best_cost = optimizer.fun
        else:
            best_cost = np.mean(opt_path[-100:])

        best_params = list(optimizer.x)
        time_taken = tack-tick

        result = {
            "optimizer":"DE",
            "method":self.method,
            "best_cost":best_cost,
            "best_params":best_params,
            "opt_path":opt_path,
            "params_path":params_path,
            "time_taken":time_taken,
            "n_evals":self.evaluations,
            "n_steps":len(opt_path)
        }

        return result