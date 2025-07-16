# -*- coding: utf-8 -*-

"""
 Title: QRNN Density Matrix Emulation.
 Description: This module contains the class emulator, which inherited with classes from loos_fs, models and readout, contains the methods for density matrix emulation of the QRNN model.

Created 06/06/2024
@author: jdviqueira

Copyright 2024 José Daniel Viqueira
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.
"""

import numpy as np

from qurecnets.qualgebra import Rx,Ry,Rz

from qurecnets.models import CZme3, CZladder2p1, encodeP3, encodeP2
from qurecnets.loss_fs import mse
from qurecnets.readout import expectZ


class emulator:
    """Class for emulation of QRNN using operator-sum representation.
    Contains methods for evaluation, gradients and hessian computation. Evaluation must receive input data and trainable parameters and 
    return a nT-length array with the expectation value of some observable each time step.
    This class must be inherited with others defining the following methods:
        * For encoding, a class is a model with 'encode' and 'encode_Nparams' methods.
        * For the ansatz (unitary that evolves the state each time step, excluding encoding), a class is a model with 'evolve' and 'evolve_Nparams' methods.
        * For the expectation value, a class defines the desired expectation value with 'readout' method.
        * For Loss function, a class defines 'loss' and 'loss_deriv' methods.
    All classes share the same __init__.
    """

    def __init__(self,nT,nE,nM,nL,nx, shots=0, rseed=None):
        """Class constructor to define emulator's attributes.

        Args:
            nT (int): Number of time steps of the time series.
            nE (int): Number of qubits in register E (Exchange).
            nM (int): Number of qubits in register M (Memory).
            nL (int): Number of layers in the evolution ansatz unitary.
            nx (int): Number of data re-uploadings in the encoding unitary.
            shots (int, optional): Fixed number of shots for sampling noise. Defaults to 0.
                                   Sampling noise is stochastic gaussian noise added to the expectation value, with a standard deviation given by quantum mechanics postulates.
            rseed (None or int, optional): Fixed seed for sampling noise. Defaults to None.
        """
        self.nT = nT
        self.nE = nE
        self.nM = nM
        self.nL = nL
        self.nx = nx

        self.NE = 2**nE
        self.NM = 2**nM

        self.shots = shots
        np.random.seed(rseed)
    


    def check_params(self, params):
        """Check whether the number of parameters is appropriate, according to the models used for encoding and evolution operators.

        Args:
            params (numpy.array): Trainable parameters.
        """
        try:
            assert len(params) == self.encode_Nparams() + self.evolve_Nparams()
            print('Number of parameters is appropriate.')
        except:
            print('Number of parameters may lead to future errors.')



    def evaluate(self, theta,x, shots=None,rseed=None):
        """Emulation of QRNN. It takes multidimensional data for each time step and this method returns the expectation values every
        time step after Density Matrix emulation. This method uses an Operator-Sum Representation for reducing memory and time requirements.

        Args:
            theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Order: see 'encode' and 'evolve' methods.
            x (numpy.array): Input data. Its shape must be (nT,nE)
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                                   If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.

        Returns:
            numpy.array: An array of expectation values for every time step.
        """
        Ut = self.evolve(theta)

        probs = []

        rhoB = np.zeros([self.NM,self.NM]); rhoB[0,0] = 1.

        for ib in range(len(x)):
            sA = self.encode(x[ib], theta)
            rhoB_tm1 = np.copy(rhoB)
            
            ### create Eks
            Eks = np.zeros([self.NE,self.NM,self.NM], dtype=np.complex128)
            for sAi,Ui in zip(sA,Ut):
                Eks += sAi*Ui
            EksD = np.array([np.conjugate(Eki.T) for Eki in Eks])
            ###
            
            rhoB = np.zeros([self.NM,self.NM], dtype=np.complex128)
            probs_ib = []
            for Ek,EkD in zip(Eks,EksD):
                Ek_rhoB_EkD = Ek@rhoB_tm1@EkD
                probs_ib.append(np.trace(Ek_rhoB_EkD).real)
                rhoB += Ek_rhoB_EkD
            
            probs.append(probs_ib)

        return self.readout(np.array(probs), shots=shots, rseed=rseed)
    


    def psr1(self, theta, x, ish, shots=None, rseed=None):
        """Compute the partial derivative with respect to the parameter theta_ish using the Parameter Shift Rule.

        Args:
            theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Order: see 'encode' and 'evolve' methods.
            x (numpy.array): Input data. Its shape must be (nT,nE).
            ish (int): Index of the parameter to compute the partial derivative.
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                                   If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.

        Returns:
            numpy.array: nT-length array with the partial derivative of the observable every time step.
        """

        Ut = self.evolve(theta)

        theta_ps = np.copy(theta); theta_ps[ish] += np.pi/2.
        theta_ms = np.copy(theta); theta_ms[ish] -= np.pi/2.


        def evaluate_sh(U,Ush,theta,thetash,tsh):
            probs = []
            rhoB = np.zeros([self.NM,self.NM]); rhoB[0,0] = 1.

            for ib in range(len(x)):
                if ib!=tsh:
                    A = U
                    sA = self.encode(x[ib], theta)
                else:
                    A = Ush
                    sA = self.encode(x[ib], thetash)

                rhoB_tm1 = np.copy(rhoB)
                
                ### create Eks
                Eks = np.zeros([self.NE,self.NM,self.NM], dtype=np.complex128)
                for sAi,Ui in zip(sA,A):
                    Eks += sAi*Ui
                EksD = np.array([np.conjugate(Eki.T) for Eki in Eks])
                ###
                
                rhoB = np.zeros([self.NM,self.NM], dtype=np.complex128)
                probs_ib = []
                for Ek,EkD in zip(Eks,EksD):
                    Ek_rhoB_EkD = Ek@rhoB_tm1@EkD
                    probs_ib.append(np.trace(Ek_rhoB_EkD).real)
                    rhoB += Ek_rhoB_EkD
                
                probs.append(probs_ib)
            
            return self.readout(np.array(probs), shots=shots, rseed=rseed)
        
        if ish<self.encode_Nparams():
            deriv = np.sum(np.array([evaluate_sh(Ut,Ut,theta,theta_ps,t) for t in range(self.nT)]), axis=0)
            deriv -= np.sum(np.array([evaluate_sh(Ut,Ut,theta,theta_ms,t) for t in range(self.nT)]), axis=0)
        else:
            UtP = self.evolve(theta_ps)
            UtM = self.evolve(theta_ms)
            deriv = np.sum(np.array([evaluate_sh(Ut,UtP,theta,theta_ps,t) for t in range(self.nT)]), axis=0)
            deriv -= np.sum(np.array([evaluate_sh(Ut,UtM,theta,theta_ms,t) for t in range(self.nT)]), axis=0)

        return deriv/2.



    def deriv_BL_fd(self, params, xin, yin, Nwout, i, eps=1.e-7):
        """Evaluation of the Loss with a shift in parameter i to compute the paartial derivative of Loss function with 
        respect to the parameter theta_i, using the forward difference method.
        The Loss is computed with the 'loss' method. The Loss function compares the last points of the output series with those of the target series yin.
        The output values are y = b + expectation_value, where b is an extra trainable parameter (biased linear - BL).

        Args:
            params (numpy.array): A bias plus trainable parameters for encoding and evolution unitaries. params = [b] + [theta] (order matters). 
            xin (numpy.array): Input data. Its shape must be (nT,nE)
            yin (numpy.array): Target output values. Its shape must be (nT,)
            Nwout (int): Number of points for prediction. Loss is actually computed by (y[-Nwout:]-yin[-Nwout:]) differences, instead of the full series.
            i (int): Index of the parameter to compute the partial derivative.
            eps (float, optional): Absolute step size of numerical approximation of the derivative. Defaults to 1.e-7.

        Returns:
            float: Loss after QRNN evaluation using [params_ps], being params_ps the original set params with params[i]+eps.
        """
        
        # one-point finite difference with respect to params_i
        # PARAMS_PS
        params_ps = np.copy(params); params_ps[i] += eps
        # evaluate with params_ps
        evalu = self.evaluate(params_ps[1:],xin,shots=0)
        yb = np.array([params_ps[0]]*len(evalu)) + evalu
        cost_ps = self.loss(yb,yin,Nwout)
        return cost_ps
    


    def deriv_fd(self, theta,xin,i,eps=1.e-7):
        """Evaluate the circuit with a shift in parameter i to compute the partial derivative of the expectation values with 
        respect to parameter theta_i, using the forward difference method.

        Args:
            theta (numpy.array): Trainable parameters.
            xin (numpy.array): Input data. Its shape must be (nT,nE)
            i (int): Index of the parameter to compute the partial derivative.
            eps (float, optional): Absolute step size of numerical approximation of the derivative. Defaults to 1.e-7.

        Returns:
            float: QRNN evaluation using [theta_ps], being theta_ps the original set parameters with theta[i]+eps.
        """

        # one-point finite difference with respect to theta_i
        # THETA_PS
        theta_ps = np.copy(theta); theta_ps[i] += eps
        # evaluate with theta_ps
   
        return self.evaluate(theta_ps,xin,shots=0)



    def grad_fd(self, theta,xin, client=None, eps=1.e-7):
        """Gradient of circuit outputs using the forward differences method.

        Args:
            theta (numpy.array): A bias plus trainable parameters for encoding and evolution unitaries. params = [b] + [theta] (order matters).
            xin (numpy.array): Input data. Its shape must be (nT,nE)
            client (distributed.client.Client, optional): Dask client, must be initialized before running the python program. Defaults to None.
                                                          If None, no distribution of gradient components is accomplished.
            eps (float, optional): Absolute step size of numerical approximation of the derivative. Defaults to 1.e-7.

        Returns:
            numpy.array: Gradient of the circuit expectation values every time step.
        """

        if client is not None:
            grads_arr = [client.submit(self.deriv_fd, theta,xin,j,eps, pure=False) for j in range(len(theta))]
            cost_ps = client.gather(grads_arr)
        else:
            cost_ps = [self.deriv_fd(theta, xin, j, eps) for j in range(len(theta))]

        evalu = self.evaluate(theta,xin, shots=0)
        return np.transpose((cost_ps-evalu)/eps)
    


    def deriv_psr(self, theta,x,ish, shots=None, rseed=None):
        """Compute the partial derivative with respect to the parameter theta_ish using the Parameter Shift Rule.

        Args:
            theta (numpy.array): Trainable parameters for encoding and evolution unitaries. Order: see 'encode' and 'evolve' methods.
            x (numpy.array): Input data. Its shape must be (nT,nE).
            ish (int): Index of the parameter to compute the partial derivative.
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                                   If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.

        Returns:
            numpy.array: nT-length array with the partial derivative of the observable every time step.
        """
        return self.psr1(theta,x,ish, shots=None, rseed=None)



    def grad_psr(self, theta, xin, client=None, shots=None,rseed=None):
        """Gradient of circuit outputs using the Parameter Shift Rule.

        Args:
            theta (numpy.array): Trainable parameters.
            xin (numpy.array): Input data. Its shape must be (nT,nE)
            client (distributed.client.Client, optional): Dask client, must be initialized before running the python program. Defaults to None.
                                                          If None, no distribution of gradient components is accomplished.
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                        If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.

        Returns:
            numpy.array: Gradient of the circuit expectation values for every time step. shape (nT,Nparams)
        """


        if client is not None:
            grads_arr = [client.submit(self.psr1, theta ,xin,j,shots=shots, rseed=rseed, pure=False) for j in range(0,len(theta))]
            grad1 = client.gather(grads_arr)

        
        else:
            grad1 = [self.psr1(theta, xin, j, shots=shots, rseed=rseed) for j in range(0,len(theta))]

        grads = np.array(grad1)

        return np.transpose(grads)



    def deriv_BL_psr(self, params, xin,yin, Nwout, i, shots=None, rseed=None):
        """Partial derivative of Loss function with respect to parameter theta_i, using the Parameter Shift Rule.
        The Loss is computed with the 'loss' method. The Loss function compares the output series with the target series yin.
        The output values are y = b + expectation_value, where b is an extra trainable parameter (biased linear - BL).

        Args:
            theta (numpy.array): A bias plus trainable parameters for encoding and evolution unitaries. params = [b] + [theta] (order matters).
            xin (numpy.array): Input data. Its shape must be (nT,nE)
            yin (numpy.array): Target output values. Its shape must be (nT,)
            Nwout (int): Number of predicted items. Loss is actually computed by (y[-Nwout:]-yin[-Nwout:]) differences, instead of the full series.
            i (int): Index of the parameter to compute the partial derivative.
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                                   If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.

        Returns:
            float: Derivative of Loss function with respect to parameter i.
        """

        py = self.psr1(params[1:],xin,i-1, shots=shots,rseed=rseed)

        evalu = self.evaluate(params[1:],xin, shots=shots,rseed=rseed)
        yb = np.array([params[0]]*len(evalu)) + evalu
        
        return self.loss_deriv(yb,yin,py,Nwout)
    



    def grad_BL_fd(self, params, xin, yin, Nwout, client=None, eps=1.e-7, savegrad=False, printgradN=False):
        """Gradient of Loss function using the forward differences method.
        The Loss is computed with the 'loss' method. The Loss function compares the output series with the target series yin.
        The output values are y = b + expectation_value, where b is an extra trainable parameter (biased linear - BL).

        Args:
            params (numpy.array): A bias plus trainable parameters for encoding and evolution unitaries. params = [b] + [theta] (order matters).
            xin (numpy.array): Input data. Its shape must be (nT,nE)
            yin (numpy.array): Target output values. Its shape must be (nT,)
            Nwout (int): Number of predicted items. Loss is actually computed by (y[-Nwout:]-yin[-Nwout:]) differences, instead of the full series.
            client (distributed.client.Client, optional): Dask client, must be initialized before running the python program. Defaults to None.
                                                          If None, no distribution of gradient components is accomplished.
            eps (float, optional): Absolute step size of numerical approximation of the derivative. Defaults to 1.e-7.
            savegrad (bool or string, optional): If string, gradients are stored in a file with the string name. Defaults to False.
            printgradN (bool, optional): If True, the norm of the gradient is printed. Defaults to False.

        Returns:
            numpy.array: Gradient of the BL Loss function.
        """

        params = np.array(params)
        xin = np.array(xin)
        yin = np.array(yin)


        if client is not None:
            grads_arr = [client.submit(self.deriv_BL_fd, params,xin,yin,Nwout,j,eps, pure=False) for j in range(len(params))]
            cost_ps = client.gather(grads_arr)
        
        else:
            cost_ps = [self.deriv_BL_fd(params, xin, yin, Nwout, j, eps) for j in range(len(params))]

        evalu = self.evaluate(params[1:],xin,shots=0)
        yb = np.array([params[0]]*len(evalu)) + evalu
        cost = (1./Nwout)*sum([(ybi-yi)**2 for (ybi,yi) in zip(yb[-Nwout:],yin[-Nwout:])])

        grad = np.array([(cost_psi-cost)/eps for cost_psi in cost_ps])

        if savegrad:
            np.savetxt(savegrad, grad)

        if printgradN:
            gradN = np.linalg.norm(grad)
            print('|grad|= ', gradN)

        return grad
    


    def grad_BL_psr(self, params, xin, yin, Nwout, client=None, shots=None, rseed=None, savegrad=False, printgradN=False):
        """Gradient of Loss function using the Parameter Shift Rule.
        The Loss is computed with the 'loss' method. The Loss function compares the last points of the output series with those of the target series yin.
        The output values are y = b + expectation_value, where b is an extra trainable parameter (biased linear - BL).


        Args:
            params (numpy.array): A bias plus trainable parameters for encoding and evolution unitaries. params = [b] + [theta] (order matters).
            xin (numpy.array): Input data. Its shape must be (nT,nE)
            yin (numpy.array): Target output values. Its shape must be (nT,)
            Nwout (int): Number of predicted items. Loss is actually computed by (y[-Nwout:]-yin[-Nwout:]) differences, instead of the full series.
            client (distributed.client.Client, optional): Dask client, must be initialized before running the python program. Defaults to None.
                                                          If None, no distribution of gradient components is accomplished.
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                        If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.
            savegrad (bool or string, optional): If string, gradients are stored in a file with the string name. Defaults to False.
            printgradN (bool, optional): If True, the norm of the gradient is printed. Defaults to False.

        Returns:
            numpy.array: Gradient of the BL Loss function.
        """

        params = np.array(params)
        xin = np.array(xin)
        yin = np.array(yin)

        if client is not None:
            grads_arr = [client.submit(self.psr1, params[1:],xin,j,shots=shots, rseed=rseed, pure=False) for j in range(0,len(params)-1)]
            grad1 = client.gather(grads_arr)

        
        else:
            grad1 = [self.psr1(params[1:], xin, j, shots=shots, rseed=rseed) for j in range(0,len(params)-1)]

        evalu = self.evaluate(params[1:],xin,shots=shots,rseed=rseed)
        yb = np.array([params[0]]*len(evalu)) + evalu

        ymyb = yb[-Nwout:] - yin[-Nwout:]

        grad1 = [(1./Nwout)*np.sum(2.*ymyb*py[-Nwout:]) for py in grad1]
        grad0 = (1./Nwout)*np.sum(2.*(yb[-Nwout:]-yin[-Nwout:]))
        grad = np.array([grad0]+grad1)

        if savegrad:
            np.savetxt(savegrad, grad)

        if printgradN:
            gradN = np.linalg.norm(grad)
            print('|grad|= ', gradN)

        return grad
    


    def deriv2_fd(self, theta,x, i,j, eps=1.e-5):
        """Evaluate the circuit with a shift in parameter i and j to compute the partial derivative of the Loss function with 
        respect to parameters theta_i and theta_j, using the forward difference method.

        Args:
            theta (numpy.array): Trainable parameters.
            x (numpy.array): Input data. shape (nT,nE)
            i (int): Number of parameter 1 to derive
            j (int): Number of parameter 2 to derive
            eps (float, optional): Absolute step size of numerical approximation of the derivative. Defaults to 1.e-5.

        Returns:
            numpy.array, numpy.array, numpy.array: circuits outputs for evaluation after shifts
        """
        theta_ps_ps = np.copy(theta); theta_ps_ps[i] += eps; theta_ps_ps[j] += eps
        theta_ps_ns = np.copy(theta); theta_ps_ns[i] += eps
        theta_ns_ps = np.copy(theta); theta_ns_ps[j] += eps
        solu_ps_ps = self.evaluate(theta_ps_ps,x, shots=0)
        solu_ps_ns = self.evaluate(theta_ps_ns,x, shots=0)
        solu_ns_ps = self.evaluate(theta_ns_ps,x, shots=0)
        return solu_ps_ps, solu_ps_ns, solu_ns_ps



    def hess_fd(self, theta,x, eps=1.e-5):
        """Evaluate the Hessian with the forward differences method.

        Args:
            theta (numpy.array): Trainable parameters.
            x (numpy.array): Input data. shape (nT,nE)
            eps (float, optional): Absolute step size of numerical approximation of the derivative. Defaults to 1.e-5.
        
        Returns:
            numpy.array: Hessian for each time step circuit output. shape (nT,Nparams,Nparams)
        """

        Nparams = self.encode_Nparams() + self.evolve_Nparams()
        #print(self.nT,Nparams,Nparams)
        #print(type(Nparams))
        hess_matrix = np.zeros((self.nT,Nparams,Nparams))

        solu_ns_ns = self.evaluate(theta,x,shots=0)
        for i in range(Nparams):
            for j in range(i+1):
                solu_ps_ps, solu_ps_ns, solu_ns_ps = self.deriv2_fd(theta,x,i,j,eps)
                deriv2 = (1./(eps**2))*(solu_ps_ps-solu_ps_ns-solu_ns_ps+solu_ns_ns)
                for t in range(self.nT):
                    hess_matrix[t,i,j] = deriv2[t]
                    hess_matrix[t,j,i] = deriv2[t]
        return hess_matrix
    


    def evaluate_w2shift_k_neq_l(self,theta,x,i,j,k,l,sh1,sh2, shots=None, rseed=None):
        """Subroutine for Hessian evaluation.
        Evaluate circuit with two parameter shifts in different blocks.

        Args:
            theta (numpy.array): Trainable parameters.
            x (numpy.array): Input data. 
            i (int): Position for shift1.
            j (int): Position for shift2.
            k (int): Block for shift1.
            l (int): Block for shift2.
            sh1 (float): Value of shift1.
            sh2 (float): Value of shift2.
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                        If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.

        Returns:
            numpy.array: Output expectation value for every time step after circuit emulation. shape (nT,NE)
        """
        
        assert j<=i
        assert l!=k
        
        if l<k:
            lsh = l; jsh = j; shift2 = sh2
            ksh = k; ish = i; shift1 = sh1
        
        else:
            lsh = k; jsh = i; shift2 = sh1
            ksh = l; ish = j; shift1 = sh2
        
        lsh = min([l,k])
        ksh = max([l,k])
        
        jsh = list([j,i])[np.argmin([l,k])]
        ish = list([j,i])[np.argmax([l,k])]
        
        shift2 = list([sh2,sh1])[np.argmin([l,k])]
        shift1 = list([sh2,sh1])[np.argmax([l,k])]
        
        barrier = self.encode_Nparams()

        Ut = self.evolve(theta)
        
        def subroutine(rhoB_tm1,sA,Uans):
            # single time step evaluation
            ### create Eks
            Eks = np.zeros([self.NE,self.NM,self.NM], dtype=np.complex128)
            for sAi,Ui in zip(sA,Uans):
                Eks += sAi*Ui
            EksD = np.array([np.conjugate(Eki.T) for Eki in Eks])
            ###
            rhoB = np.zeros([self.NM,self.NM], dtype=np.complex128)
            probs_ib = []
            for Ek,EkD in zip(Eks,EksD):
                Ek_rhoB_EkD = Ek@rhoB_tm1@EkD
                probs_ib.append(np.trace(Ek_rhoB_EkD).real)
                rhoB += Ek_rhoB_EkD
            return rhoB, probs_ib
        
        
        # 2/ Running circuit
        rhoB = np.zeros([self.NM,self.NM]); rhoB[0,0] = 1.
        probs_t = []
        for ib in range(lsh):
            sA = self.encode(x[ib], theta)
            rhoB, prob = subroutine(rhoB,sA,Ut)
            probs_t += [prob]
        # shift2:
        theta_sh = np.copy(theta); theta_sh[jsh] += shift2
        if jsh<barrier:
            sA = self.encode(x[lsh], theta_sh)
            Uans = Ut
        else:
            sA = self.encode(x[lsh], theta)
            Uans = self.evolve(theta_sh)
        rhoB, prob = subroutine(rhoB,sA,Uans)
        probs_t += [prob]
        #::
        for ib in range(lsh+1,ksh):
            sA = self.encode(x[ib], theta)
            rhoB, prob = subroutine(rhoB,sA,Ut)
            probs_t += [prob]
        # shift1:
        theta_sh = np.copy(theta); theta_sh[ish] += shift1
        if ish<barrier:
            sA = self.encode(x[ksh], theta_sh)
            Uans = Ut
        else:
            sA = self.encode(x[ksh], theta)
            Uans = self.evolve(theta_sh)
        rhoB, prob = subroutine(rhoB,sA,Uans)
        probs_t += [prob]
        #::
        for ib in range(ksh+1,self.nT):
            sA = self.encode(x[ib], theta)
            rhoB, prob = subroutine(rhoB,sA,Ut)
            probs_t += [prob]
        
        return self.readout(np.array(probs_t), shots=shots, rseed=rseed)



    def evaluate_w2shift_k_eq_l(self,theta,x,i,j,k,l,sh1,sh2, shots=None,rseed=None):
        """Subroutine for Hessian evaluation.
        Evaluate circuit with two shifts in the same block.

        Args:
            theta (numpy.array): Trainable parameters.
            x (numpy.array): Input data. 
            i (int): Position for shift1.
            j (int): Position for shift2.
            k (int): Block for shift1.
            l (int): Block for shift2.
            sh1 (float): Value of shift1.
            sh2 (float): Value of shift2.
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                        If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.

        Returns:
            numpy.array: Output expectation value for every time step after circuit emulation. shape (nT,NE)
        """
        
        assert j<=i
        assert l==k
        
        lsh = l; jsh = j; shift2 = sh2
        ksh = k; ish = i; shift1 = sh1
        
        Ut = self.evolve(theta)
        
        def subroutine(rhoB_tm1,sA,Uans):
            # single time step evaluation
            ### create Eks
            Eks = np.zeros([self.NE,self.NM,self.NM], dtype=np.complex128)
            for sAi,Ui in zip(sA,Uans):
                Eks += sAi*Ui
            EksD = np.array([np.conjugate(Eki.T) for Eki in Eks])
            ###
            rhoB = np.zeros([self.NM,self.NM], dtype=np.complex128)
            probs_ib = []
            for Ek,EkD in zip(Eks,EksD):
                Ek_rhoB_EkD = Ek@rhoB_tm1@EkD
                probs_ib.append(np.trace(Ek_rhoB_EkD).real)
                rhoB += Ek_rhoB_EkD
            return rhoB, probs_ib
        
        
        # 2/ Running circuit
        rhoB = np.zeros([self.NM,self.NM]); rhoB[0,0] = 1.
        probs_t = []
        for ib in range(lsh):
            sA = self.encode(x[ib], theta)
            rhoB, probs = subroutine(rhoB,sA,Ut)
            probs_t += [probs]
        # 2shift:
        theta_2sh = np.copy(theta); theta_2sh[jsh] += shift2; theta_2sh[ish] += shift1
        sA = self.encode(x[l], theta_2sh)
        Uans = self.evolve(theta_2sh)
        rhoB, probs = subroutine(rhoB,sA,Uans)
        probs_t += [probs]
        #::
        for ib in range(l+1,self.nT):
            sA = self.encode(x[ib], theta)
            rhoB, probs = subroutine(rhoB,sA,Ut)
            probs_t += [probs]
        
        return self.readout(np.array(probs_t), shots=shots, rseed=rseed)



    def psr2(self,theta,x,client=None,shots=None,rseed=None):
        """Routine to compute the 2nd order Parameter Shift Rule to later compute the Hessian.

        Args:
            theta (numpy.array): Trainable parameters.
            x (numpy.array): Input data. shape (nT,nE)
            client (distributed.client.Client, optional): Dask client. Not supported in this version. Defaults to None.
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                        If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.


        Returns:
            (dict,dict,dict,dict): O_ps_ps, O_ps_ms, O_ms_ps, O_ms_ms being O the dictionaries {ksh,ish,lsh,jsh: evaluation}.
            Keys indicate the shifts in the couple of parameters ish,jsh in blocks ksh and lsh, respectively.
            ps and ms indicate the sign of the shifts: Plus and Minus.
        """

        Nparams = self.encode_Nparams() + self.evolve_Nparams()
        O_ps_ps = {}; O_ps_ms = {}; O_ms_ps = {}; O_ms_ms = {}
        ps = +np.pi/2.; ms = -np.pi/2.
        
        if client is None:
            pass
        else:
            print('DASK parallelization not supported for this method.')

        ## For i!=j:
        for ish in range(len(theta)):
            for jsh in range(ish):
                for ksh in range(self.nT):
                    for lsh in range(self.nT):
                        if ksh!=lsh: ## for k!=l
                            O_ps_ps[ksh,ish,lsh,jsh] = self.evaluate_w2shift_k_neq_l(theta,x,ish,jsh,ksh,lsh,ps,ps)
                            O_ps_ms[ksh,ish,lsh,jsh] = self.evaluate_w2shift_k_neq_l(theta,x,ish,jsh,ksh,lsh,ps,ms)
                            O_ms_ps[ksh,ish,lsh,jsh] = self.evaluate_w2shift_k_neq_l(theta,x,ish,jsh,ksh,lsh,ms,ps)
                            O_ms_ms[ksh,ish,lsh,jsh] = self.evaluate_w2shift_k_neq_l(theta,x,ish,jsh,ksh,lsh,ms,ms)
                        else: ## for k=l
                            O_ps_ps[ksh,ish,ksh,jsh] = self.evaluate_w2shift_k_eq_l(theta,x,ish,jsh,ksh,ksh,ps,ps)
                            O_ps_ms[ksh,ish,ksh,jsh] = self.evaluate_w2shift_k_eq_l(theta,x,ish,jsh,ksh,ksh,ps,ms)
                            O_ms_ps[ksh,ish,ksh,jsh] = self.evaluate_w2shift_k_eq_l(theta,x,ish,jsh,ksh,ksh,ms,ps)
                            O_ms_ms[ksh,ish,ksh,jsh] = self.evaluate_w2shift_k_eq_l(theta,x,ish,jsh,ksh,ksh,ms,ms)
        
        ## For i=j:
        evaluation = self.evaluate(theta,x,shots,rseed)
        for ish in range(len(theta)):
            for ksh in range(self.nT):
                for lsh in range(ksh): ## for k!=l, symmetry in t
                    O_ps_ps_ikl = self.evaluate_w2shift_k_neq_l(theta,x,ish,ish,ksh,lsh,ps,ps); O_ps_ps[ksh,ish,lsh,ish] = O_ps_ps_ikl; O_ps_ps[lsh,ish,ksh,ish] = O_ps_ps_ikl
                    O_ps_ms_ikl = self.evaluate_w2shift_k_neq_l(theta,x,ish,ish,ksh,lsh,ps,ms); O_ps_ms[ksh,ish,lsh,ish] = O_ps_ms_ikl; O_ps_ms[lsh,ish,ksh,ish] = O_ps_ms_ikl
                    O_ms_ps_ikl = self.evaluate_w2shift_k_neq_l(theta,x,ish,ish,ksh,lsh,ms,ps); O_ms_ps[ksh,ish,lsh,ish] = O_ms_ps_ikl; O_ms_ps[lsh,ish,ksh,ish] = O_ms_ps_ikl
                    O_ms_ms_ikl = self.evaluate_w2shift_k_neq_l(theta,x,ish,ish,ksh,lsh,ms,ms); O_ms_ms[ksh,ish,lsh,ish] = O_ms_ms_ikl; O_ms_ms[lsh,ish,ksh,ish] = O_ms_ms_ikl
                
                lsh = ksh        
                O_ps_ps[ksh,ish,ksh,ish] = self.evaluate_w2shift_k_eq_l(theta,x,ish,ish,ksh,ksh,ps,ps)
                O_ps_ms[ksh,ish,ksh,ish] = evaluation
                O_ms_ps[ksh,ish,ksh,ish] = evaluation
                O_ms_ms[ksh,ish,ksh,ish] = self.evaluate_w2shift_k_eq_l(theta,x,ish,ish,ksh,ksh,ms,ms)
        
        return O_ps_ps, O_ps_ms, O_ms_ps, O_ms_ms



    def hess_psr(self, theta,x,client=None,shots=None,rseed=None):
        """Compute the analytical Hessian with the Parameter Shift Rule.

        Args:
            theta (numpy.array): Trainable parameters.
            x (numpy.array): Input data. shape (nT,nE)
            client (distributed.client.Client, optional): Dask client. Not supported in this version. Defaults to None.
            shots (int, optional): Number of shots for simulated sampling noise. Defaults to None.
                        If None, the number is defined in the constructor; else, the argument value is considered.
            rseed (None or int, optional): seed for sampling noise. Defaults to None.
                                           If None, the number is defined in the constructor; else, the argument value is considered.

        Returns:
            numpy.array: Hessian matrix. shape: (Ntheta,Ntheta)
        """
        
        Nparams = self.encode_Nparams() + self.evolve_Nparams()

        O_ps_ps, O_ps_ms, O_ms_ps, O_ms_ms = self.psr2(theta,x,client,shots,rseed)
        
        
        hess = {}
        for i in range(len(theta)):
            for j in range(i+1):
                for t in range(self.nT):
                    hess[i,j] = 0
        
        for key,value in O_ps_ps.items():
            k,i,l,j = key
            hess[i,j] += 0.25*value

        
        for key,value in O_ms_ms.items():
            k,i,l,j = key
            hess[i,j] += 0.25*value
            
        
        for key,value in O_ps_ms.items():
            k,i,l,j = key
            hess[i,j] -= 0.25*value
        
        for key,value in O_ms_ps.items():
            k,i,l,j = key
            hess[i,j] -= 0.25*value


        hess_matrix = np.zeros((self.nT,Nparams,Nparams))
        for i in range(Nparams):
            for j in range(i+1):
                for t in range(self.nT):
                    hess_matrix[t,i,j] = hess[i,j][t]
                    hess_matrix[t,j,i] = hess[i,j][t]

        return hess_matrix




class EMCZ2(emulator, encodeP2, CZladder2p1, expectZ, mse):
    """Class for emulation of QRNN using operator-sum representation.
    Qubits are divided in register E (exchange: encoding+measurement) and M (memory).
    Rotation gates are 2-parameters groups |Rx|-|Rz|. Encoding operator includes for data re-uploading and evolution operator is a multi-layer Hardware-Efficient ansatz.
    A layer consists of rotations and a ladder of controlled-Z (CZ) gates. A final layer of Rx is applied before reg. E measurement.
    The output every time step is y_(t) = b + <Z>, where b is a bias, an extra trainable parameter, and <Z> is the Z-expectation value from measurement of register E.
    The Loss function is the Mean Squared Error.
    The class contains methods for evaluation, gradients and hessian computation. Evaluation must receive input data and trainable parameters and 
    return a nT-length array with the expectation value of some observable each time step.
    This class has the following main methods:
        * For encoding, 'encode' and 'encode_Nparams'.
        * For the ansatz (unitary that evolves the state each time step, excluding encoding), 'evolve' and 'evolve_Nparams'.
        * For the expectation value, the desired expectation value is computed with the 'readout'.
        * For Loss function, 'loss' and 'loss_deriv'.
    """
    pass




class EMCZ3(emulator, encodeP3, CZme3, expectZ, mse):
    """Class for emulation of QRNN using operator-sum representation.
    Qubits are divided in register E (exchange: encoding+measurement) and M (memory).
    Rotation gates are 3-parameter groups |Rx|-|Rz|-|Rx|. Encoding operator includes for data re-uploading and evolution operator is a multi-layer Hardware-Efficient ansatz.
    A layer consists of rotations and controlled-Z (CZ) gates, entangling every qubit in reg. E with every qubit in reg. M. A final layer of |Rx|-|Rz|-|Rx| is applied before reg. E measurement.
    The output every time step is y_(t) = b + <Z>, where b is a bias, an extra trainable parameter, and <Z> is the Z-expectation value from measurement of register E.
    The Loss function is the Mean Squared Error.
    The class contains methods for evaluation, gradients and hessian computation. Evaluation must receive input data and trainable parameters and 
    return a nT-length array with the expectation value of some observable each time step.
    This class has the following main methods:
        * For encoding, 'encode' and 'encode_Nparams'.
        * For the ansatz (unitary that evolves the state each time step, excluding encoding), 'evolve' and 'evolve_Nparams'.
        * For the expectation value, the desired expectation value is computed with the 'readout'.
        * For Loss function, 'loss' and 'loss_deriv'.
    """
    pass
