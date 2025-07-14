# -*- coding: utf-8 -*-
"""
 Title: EMCZ2 multivariate model Qiskit simulation
 Description: this is a python module to simulate with Qiskit the Quantum Recurrent Neural Network 
 model called "EMCZ2 model". Only available for training.

Created on Tue Mar 21 10:32:11 2023
@author: jdviqueira
"""

from qiskit import QuantumRegister, ClassicalRegister, QuantumCircuit, transpile, assemble #, execute
from qiskit.circuit import Parameter
from qiskit.result import marginal_counts
#from qiskit.providers.aer import AerSimulator
from qiskit_aer import AerSimulator
#from qiskit.providers import Basic
import numpy as np

class emcz:
    """
    Class to define the EMCZ2 multivariate model circuit with Qiskit.
    
    Exchange-Memory CZ 2 model is a QRNN with a multilayer ansatz, in which a layer is composed by
    a column of Rx-Rz gates over each qubit plus a ladder of CZs acting on both registers. Finally,
    Rx over the E register.
    Encoding is done over E register with Ry(x_t). Measurement is applied over E register
    after having applied the ansatz. Output is the Z-expectation value times a scaling factor.
    
    See circuit_draft.txt to visualize the circuit structure.
    """
    
    def __init__(self,nT,nA,nB,nlayers,indeg, backend=None):
        """
        

        Parameters
        ----------
        nT :  integer
            number of exchanges, i.e. number of time frames
        nA : integer
            number of qubits in E register.
        nB: integer
            number of qubits in M register.
        nlayers: integer
            number of ansatz layers
        indeg : integer
            Degree of input function by repetition of encoding gates and intermediate 3-parameter
            rotation gates. The real degree is indeg+1. You can set 0.
            
        Returns
        -------
        None.

        """
        
        qA = QuantumRegister(nA,'qA')
        qB = QuantumRegister(nB,'qB')
        cr = ()
        for i in range(nT):
            cri = ClassicalRegister(nA,'cT'+str(i))
            cr += (cri,)
    
        qrnc = QuantumCircuit(qA,qB,*cr)
        
        
        # parameters for data re-uploading:
        al = [Parameter('al_'+str(i)+'_'+str(li)+'_'+str(co)+'_'+str(t))  for t in range(nT)
              for i in range(nA) for li in range(indeg) for co in range(2)]
        
        # parameters for multilayer recombination:
        th = [Parameter('th_'+str(li)+'_'+str(i)+'_'+str(co)+'_'+str(t)) for t in range(nT)
              for li in range(nlayers) for i in range(nA+nB) for co in range(2)]
        
        # parameters for final recombination before measurement:
        be = [Parameter('be_'+str(i)+'_'+str(co)+'_'+str(t)) for t in range(nT)
              for i in range(nA) for co in range(1)]
        
        # input data as rotations:
        xt = [Parameter('xt_'+str(i)+'_'+str(li)+'_'+str(t)) for t in range(nT)
              for i in range(nA) for li in range(indeg+1)]
        
        
        for t in range(nT):
            
            for j in range(nA):
                qrnc.reset(j)
                idx = (indeg+1)*nA*t + (indeg+1)*j
                qrnc.ry(xt[idx],j)
                for repi in range(indeg):
                    idx2 = 2*(indeg*nA*t + indeg*j + repi)
                    qrnc.rx(al[idx2],j)
                    qrnc.rz(al[idx2+1],j)
                    qrnc.ry(xt[idx+repi+1],j)
            
            qrnc.barrier()
    
            for li in range(nlayers):
                for i in range(nA):
                    idx = 2*((nB+nA)*nlayers*t + (nB+nA)*li + i)
                    qrnc.rx(th[idx],i)
                    qrnc.rz(th[idx+1],i)
    
                for i in range(nA,nA+nB):
                    idx = 2*((nB+nA)*nlayers*t + (nB+nA)*li + i)
                    qrnc.rx(th[idx],i)
                    qrnc.rz(th[idx+1],i)
                
                qrnc.barrier()
                for i in range(nA+nB-1):
                    qrnc.cz(i,i+1)
                qrnc.barrier()
            
            for i in range(nA):
                idx = 1*(nA*t + i)
                qrnc.rx(be[idx],i)
            
            for i in range(nA):
                qrnc.measure(i,nA*t+i) # CHECK
            
            qrnc.barrier()
            
            
            self.circ = qrnc
            self.transcirc = transpile(qrnc, backend=backend)
            self.params_alpha = al
            self.params_theta = th
            self.params_beta  = be
            self.params_inputs= xt
            
            self.nA = nA
            self.nB = nB
            self.nT = nT
            self.nlayers = nlayers
            self.indeg = indeg
            
            if backend == None:
                self.device = AerSimulator()
            else:
                self.device = backend
            
            
        
    
    
    def get_expectZ(self,result):
        """
        Subroutine to evaluate Z expectation value for each measurement after circuit is run.

        Parameters
        ----------
        result : Qiskit result object
            Result from simulation.

        Returns
        -------
        expectZ : list
            List with Z-expectation values from intermediate measurements, i.e. the QRNN outputs.

        """

        expectZ = []
        for i in range(self.nT):
            result_of_qubit = marginal_counts(result, indices=[meas for meas in range(self.nA*i,self.nA*(i+1))]).get_counts()
            #print({key: value/200000 for key,value in result_of_qubit.items()})
            signs = [(-1)**key.count('1') for key in result_of_qubit.keys()]
            counts = result_of_qubit.values()
            countt = sum(counts)
            expZi = sum([signi*(counti/countt) for signi,counti in zip(signs,counts)]) # correction!
            expectZ.append(expZi)
        return expectZ
    
    

    def draw(self,mode='text', fold=-1):
        """
        Draw the complete circuit. Many time frames would lead to the collapse of the visualization
        .

        Parameters
        ----------
        mode : str, optional
            Mode of visualization. The default is 'text'.

        Returns
        -------
        matplotlib.draw, print
            Sketch of the circuit.

        """
        return self.circ.draw(mode, fold=fold)
    
    

    def run(self,x,theta):
        """
        Execute the circuit.

        Parameters
        ----------
        x: input
        theta: 3-D array divided in alpha,beta, and each item is a 2-angles list
               or, alternatively, 1-D flat array
        

        Returns
        -------
        solution : list
            Outputs (predictions) of the circuit.
        
        For more information, see AerSimulator documentation:
        https://qiskit.org/documentation/stubs/qiskit_aer.AerSimulator.html

        """
        
        self.inputs = x
        
        bind_params = {}
        
        theta = np.array(theta)
        if len(theta.shape)==3:
            al_vals = theta[0].flatten(); th_vals = theta[1].flatten(); be_vals = theta[2].flatten()
        
        group1 = 2*self.nA*self.indeg
        group2 = 2*(self.nA+self.nB)*self.nlayers
        group3 = 1*self.nA
        if len(theta.shape)==1:
            al_vals = theta[:group1]
            th_vals = theta[group1:group1+group2]
            be_vals = theta[group1+group2:group1+group2+group3]
        
        for t in range(self.nT):
            for i in range(group1):
                bind_params[self.params_alpha[group1*t+i]] = al_vals[i]
        
        for t in range(self.nT):
            for i in range(group2):
                bind_params[self.params_theta[group2*t+i]] = th_vals[i]
        
        for t in range(self.nT):
            for i in range(group3):
                bind_params[self.params_beta[group3*t+i]] = be_vals[i]
        
        for t in range(self.nT):
            for j in range(self.nA):
                for gi in range(self.indeg+1):
                    bind_params[self.params_inputs[(self.indeg+1)*self.nA*t+
                                                   (self.indeg+1)*j+gi]] = x[t][j]
                    
        
        qobj = assemble(self.transcirc.bind_parameters(bind_params))
        result = self.device.run(qobj, shots=self.device.options.shots).result()
        
        return result



    
    def grad_psr(self,theta):
        """
        (DEVELOPING...) Execute the 1st order Parameter Shift Rule for computing the Gradient.

        Parameters
        ----------
        theta: 3-D array divided in alpha,theta,beta, and each item is a 3-angles list
               or, alternatively, 1-D flat array
        

        Returns
        -------
        solution : list
            Outputs (predictions) of the circuit.
        
        For more information, see AerSimulator documentation:
        https://qiskit.org/documentation/stubs/qiskit_aer.AerSimulator.html

        """
        #sim = AerSimulator(method=method, device=device, precision=precision, max_parallel_shots=max_parallel_shots)
        #tc = transpile(self.circ(), sim)
        
        def run_wshift(theta,shift,ksh,ish):
        
            bind_params = {}
            
            theta = np.array(theta)
            if len(theta.shape)==3:
                al_vals = theta[0].flatten(); th_vals = theta[1].flatten(); be_vals = theta[2].flatten()
            
            group1 = 3*self.nA*self.indeg
            group2 = 3*(self.nA+self.nB)*self.nlayers
            group3 = 3*self.nA
            if len(theta.shape)==1:
                al_vals = theta[:group1]
                th_vals = theta[group1:group1+group2]
                be_vals = theta[group1+group2:group1+group2+group3]
            
            for t in range(self.nT):
                for i in range(group1):
                    bind_params[self.params_alpha[group1*t+i]] = al_vals[i]
            
            for t in range(self.nT):
                for i in range(group2):
                    bind_params[self.params_theta[group2*t+i]] = th_vals[i]
            
            for t in range(self.nT):
                for i in range(group3):
                    bind_params[self.params_beta[group3*t+i]] = be_vals[i]
            
            for t in range(self.nT):
                for j in range(self.nA):
                    for gi in range(self.indeg+1):
                        #print(t,j,gi)
                        bind_params[self.params_inputs[(self.indeg+1)*self.nA*t+
                                                       (self.indeg+1)*j+gi]] = self.inputs[t][j]
                        
            
            qobj = assemble(self.transcirc.bind_parameters(bind_params))
            
            result = self.device.run(qobj, shots=self.device.options.shots).result()
            
            return result
        
        
        
#        for ish in self.params_alpha:
#            for ksh in self.nT:
                # call run_wshift for +pi/2
                # call run_wshift for -pi/2
                
#        for ish in self.params_beta:
#            for ksh in self.nT:
                # call run_wshift for +pi/2
                # call run_wshift for -pi/2
        
#        for ish in self.params_gamma:
#            for ksh in self.nT:
                # call run_wshift for +pi/2
                # call run_wshift for -pi/2
        
#        return result