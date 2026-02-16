"""
Code implementing the Iterative Quantum Phase Estimation (iQPE) algorithm with classical communications. To understand the algorithm without communications check:
    - Original paper (here referred to as Iterative Phase Estimation Algorithm): https://arxiv.org/abs/quant-ph/0610214
    - TalentQ explanation (in spanish): https://talentq-es.github.io/Fault-Tolerant-Algorithms/docs/Part_01_Fault-tolerant_Algorithms/Chapter_01_01_IPE_portada_myst.html
"""
import os, sys
# In order to import cunqa, we append to the search path the cunqa installation path.
# In CESGA, we install by default on the $HOME path as $HOME/bin is in the PATH variable
sys.path.append(os.getenv("HOME"))

from cunqa.qpu import get_QPUs, qraise, qdrop, run
from cunqa.circuit import CunqaCircuit
from cunqa.qjob import gather


try:

    # 1. QPU deployment

    family_name = qraise(n_qpus, "03:00:00", simulator=simulator, classical_comm=True, co_located = True, cores = cores_per_qpu, mem_per_qpu = mem_per_qpu)



except Exception as error:
    qdrop(family_name)
    raise error