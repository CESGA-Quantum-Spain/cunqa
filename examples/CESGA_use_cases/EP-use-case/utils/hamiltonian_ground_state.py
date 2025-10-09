

from numpy.linalg import eigh
from utils import Ising_Hamiltonian
import os
import json



eigen_values = {"J=1, g=0.5":{}}
eigen_states = {"J=1, g=0.5":{}}

for n in range(3,7):

    print(f"Calculating for {n} qubits...")

    H = Ising_Hamiltonian(int(n), J=1, g=0.5)
    matrix = H.to_matrix()
    eigvals, eigvecs = eigh(matrix)
    print(eigvals[0:3])
    eigs = sorted(set(eigvals))
    int_gs = eigvals.tolist().index(eigs[0])
    int_g1 = eigvals.tolist().index(eigs[1])
    int_g2 = eigvals.tolist().index(eigs[2])
    int_g3 = eigvals.tolist().index(eigs[3])


    print("\tNumber of eigen values calculated: ", len(eigs))

    eigen_values["J=1, g=0.5"][n] = {"Ground State":eigs[0], "1st excited":eigs[1], "2nd excited":eigs[2], "3rd excited":eigs[3]}
    eigen_states["J=1, g=0.5"][n] = {"Ground State":list(eigvecs[:,int_gs]), "1st excited":list(eigvecs[:,int_g1]), "2nd excited":list(eigvecs[:,int_g2]), "3rd excited":list(eigvecs[:,int_g3])}

# print(eigen_states)
constants_path = "constants.json"

# Load existing data if file exists, else create new dict
if os.path.exists(constants_path):
    with open(constants_path, "r") as f:
        try:
            data = json.load(f)
        except json.JSONDecodeError:
            data = {}
else:
    data = {}

# Add or update the eigen_states dictionary
data["eigen_states"] = eigen_states
data["eigen_values"] = eigen_values

print(data)

# # Write back to the file
# with open(constants_path, "w") as f:
#     json.dump(data, f, indent=4)
