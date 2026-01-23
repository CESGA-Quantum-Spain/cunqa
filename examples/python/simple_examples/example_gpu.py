import os, sys
from time import sleep

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa.qutils import get_QPUs, qraise, qdrop
from cunqa.circuit import CunqaCircuit

#--------------------------------------------------------------------------
# qraise -n 1 -c 32 -t 00:10:00 --co-located --gpu --fam="simplegpu"
#--------------------------------------------------------------------------

family = qraise(1, "00:10:00", cores = 32, simulator = "Aer", co_located = True, gpu = True)
qpus  = get_QPUs(on_node = False, family = family)

qc = CunqaCircuit(5)
qc.h(0)
qc.cx(0, 1)
qc.measure_all()

qpu = qpus[0]
qjob = qpu.run(qc, transpile = False, shots = 10) # non-blocking call

counts = qjob.result.counts
time = qjob.time_taken

print(qjob.result)

########## Drop the deployed QPUs #
qdrop(family)