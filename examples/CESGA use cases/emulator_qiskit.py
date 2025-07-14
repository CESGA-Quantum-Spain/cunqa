# -*- coding: utf-8 -*-

import numpy as np
import sys

# Qiskit:
# required qiskit 0.40.0
#from qurecnets.emc.qiskit_emc2 import emcz2
from qiskit_EMCZ2 import emcz
from qiskit_aer import AerSimulator

print('Qiskit modules loaded')


np.random.seed(1)

nT = 10; nE = 1; nM = 2; nL = 2; nx = 1
param0 = np.random.random(2*(nE*nx + (nE+nM)*nL) + nE)
print(param0)
xin = np.sin(np.arange(0,nT*nE)).reshape(nT,nE)
print(xin)
yin = np.random.random(nT)


print('a',len(param0.shape))


################################################################
print('Qiskit')
QRNN = emcz(nT,nE,nM,nL,nx, backend=AerSimulator(shots=1000))

result = QRNN.run(xin, param0)
yout = QRNN.get_expectZ(result)
print(yout)