import os, sys
import numpy as np

# path to access c++ files
sys.path.append(os.getenv("HOME"))

from cunqa import get_QPUs, gather
from cunqa.logger import logger
from cunqa.circuit import CunqaCircuit

circ1 = CunqaCircuit(3, 3, id = "First")
circ2 = CunqaCircuit(3, 3, id = "Second")

# Add gates to each of the two circuits
circ1.h(0)
circ1.cx(0,1)

circ2.rz(np.pi/2, 0)
circ2.x(2)

# Sum the circuits (operations of circ1 will be perfomed, then, those of circ2)
###############################################################################
circ_sum = circ1 + circ2
print("SUM -------------------------------------------------------------")
print(f"Instructions of circ1 are {circ1.instructions}")
print(f"Instructions of circ2 are {circ2.instructions}\n")
print(f"Instructions of the sum of the circuits are {circ_sum.instructions}\n")
print(f"Id of the sum of the circuits is {circ_sum._id}")

# Circuit sum interact in a delicate way with communications.
# If we sum two circuits that have distributed gates between them, the first circuit will wait indefinitely (in the sum) for the second circuit,
# which can only respond after the first has been executed. FATAL!
circ3=CunqaCircuit(1,1, id="Third")
circ4=CunqaCircuit(1,1, id="Fourth")

circ3.measure_and_send(0, "Fourth")
circ4.remote_c_if("x", 0, control_circuit="Third")

try:
    circ3 + circ4

except SystemExit as error:
    pass

# Another option to sum multiple circuits on a row is sum()
circ5 = CunqaCircuit(3, 3, id="Fifth")
circ5.rz(np.pi, 0)
circ5.rx(np.pi/2, 1)
circ5.ry(np.pi/4, 2)
long_sum = CunqaCircuit.sum(( list_circuits := [circ1, circ5, circ2, circ5, circ5]))
print(f"Use of CunqaCircuit.sum, to sum {[circ._id for circ in list_circuits]}")
print(f"Instructions of the long sum are:\n {long_sum.instructions}\n")




# We also have the union operator | that takes two circuits and merges them in a big circuit with all of their qubits
#####################################################################################################################
circ_union = circ1 | circ2
print("UNION -------------------------------------------------------------")
print(f"Instructions of circ1 are {circ1.instructions}")
print(f"Instructions of circ2 are {circ2.instructions}\n")
print(f"Instructions of the union of the circuits are {circ_union.instructions}\n")
print(f"Id of the union of the circuits is {circ_union._id}\n")

# Now if we union two circuits that talk to eachother it's not a problem, the distributed gates will be replaced by local ones
circ_union2 = circ3 | circ4
print(f"Instructions of circ3 are {circ3.instructions}")
print(f"Instructions of circ4 are {circ4.instructions}\n")
print(f"Instructions of the union of the circuits are {circ_union2.instructions}\n")

# Another option to union multiple circuits on a row is union()
big_union = CunqaCircuit.union((list2_circuits := [circ1, circ5, circ2]))
print(f"Use of CunqaCircuit.union, to union {list2_circuits}")
print(f"Instructions of the big union are:\n {big_union.instructions} \n")




# Additionally, we can divide a circuit into smaller parts 
# cutting vertically at a certain layer or horizontally at a certain qubit, returning two smaller circuits
#############################################################################################################
print("CIRCUIT DIVISION -------------------------------------------------------------")

circ1_v2, circ2_v2 = circ_sum.vert_split(1)
print(f"Vertical division at layer 1 (layers are 0-indexed) of circuit {circ_sum._id}\n")
print(f"Instructions of the first piece are {circ1_v2.instructions} ")
print(f"Instructions of the second piece are {circ2_v2.instructions} \n")
print(f"We note that vertical circuit division is the inverse of the sum. We get circ1 and circ2 back")
print("\n") 
# Problema las layers tiran las instructions hacia la izquierda con lo que no está asegurado casi nunca que sean inversas :(

circ1_v3, circ2_v3 = circ_union.hor_split(2)
print(f"Horizontal division after qubit 2 (qubits are 0-indexed) of circuit {circ_sum._id}\n")
print(f"Instructions of the first piece are {circ1_v3.instructions}")
print(f"Instructions of the second piece are {circ2_v3.instructions} \n")
print(f"We note that horizontal circuit division is the inverse of the union. We get circ3 and circ4 back")
# Problem with handling the displacement of certain gate qubits depending on where the gate falls w respect to the cut