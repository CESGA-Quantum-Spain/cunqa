import os, sys
import numpy as np

# In order to import cunqa, we append to the search path the cunqa installation path
sys.path.append(os.getenv("HOME")) # HOME as install path is specific to CESGA

from cunqa.circuit import CunqaCircuit
from cunqa.qpu import qraise, get_QPUs, run, qdrop


###################################
########## SAVING STATES ##########
###################################

# Create a circuit that saves the state at certain step
circ = CunqaCircuit(3)
circ.h(0)
circ.cx(0, 1)
circ.rz(np.pi/4, 2)
# Saves the state. The state depends on the simulation method: it can be statevector, density matrix, ...
circ.save_state() 
circ.measure_all()

# Deploy QPUs with qraise, connect to them with get_QPUs 
family = qraise(3, "00:10:00", co_located = True) #Important: Aer simulator
qpus   = get_QPUs(co_located = True)

# Execute the circuit with statevector and density_matrix simulation methods and get their results
qjob_vec     = run(circ, qpus[-1], method="statevector")
qjob_densmat = run(circ, qpus[-2], method="density_matrix")

statevec = qjob_vec.result.statevector
densmat  = qjob_densmat.result.density_matrix

# Print the statevector and density matrix
print(f"\nWe obtained the statevector \n{statevec} \nand the density matrix \n{densmat}")


########## The state can be recovered at multiple times of the simulation! ##########

# Create a circuit with several save states
# Each save_state SHOULD HAVE A DIFFERENT LABEL (otherwise an error will be raised)
circ_sev = CunqaCircuit(2)
circ.h(0)
circ.cx(0, 1)
# First save state
circ_sev.save_state(label="After bell pair") 
circ.rz(np.pi/4, 0)
circ.rxx(np.pi/6, 0, 1)
# Second save state
circ_sev.save_state(label="Final state") 
circ_sev.measure_all()

# Execute the circuit with statevector and density_matrix simulation methods
qjob_vec     = run(circ, qpus[-1], method="statevector")
qjob_densmat = run(circ, qpus[-2], method="density_matrix")

# Now a dictionary with keys the save_state labels and values the corresponding states will be returned
statevecs = qjob_vec.result.statevector
densmats  = qjob_densmat.result.density_matrix

# Print the statevectors and density matrices
print(f"\nWe obtained the statevectors \n{statevecs} \nand the density matrices \n{densmats}")


##############################################
########## EXTRACTING PROBABILITIES ##########
##############################################

# Probabilities can be extracted from the Result object of a QJob
# If a state has been saved, exact probabilities are extracted from the state, otherwise they are estimated as the counts frequencies
# Let us compare the probabilities extracted on the three ways (from statevector, from density matrix or estimated from counts)

# Create circuit with no state for estimating the state from counts. Same circuit as the beginning
circ_no_state = CunqaCircuit(3)
circ_no_state.h(0)
circ_no_state.cx(0, 1)
circ_no_state.rz(np.pi/4, 2)
circ_no_state.measure_all()

# Execute it and extract probabilities from its result and the state ones
qjob_estimate = run(circ_no_state, qpus[-3])
qjob_estimate = run(circ_no_state, qpus[-3])
qjob_estimate = run(circ_no_state, qpus[-3])

probs_vec      = qjob_vec.result.probabilities()
probs_densmat  = qjob_densmat.result.probabilities()
probs_estimate = qjob_estimate.result.probabilities()

# Visualize
# Note that the deafult probabilities are the bitstring probabilities, ie the probability of obtaining a result '00101011' for example
# They are ordered in binary ascending fashion 
print(f"\nProbabilities with statevector are {probs_vec}")
print(f"\nProbabilities with density_matrix are {probs_densmat}")
print(f"\nEstimated probabilities are {probs_estimate}")

# Additionally, the function .probabilities() has two options for modifying the output
# These are partial = list[int] and per_qubit = True/False

# per_qubit = True gives the marginalized probabilities of obtaining '0' or '1' for each of the qubits, instead of having probabilities per bitstring
print(f"Per qubit probabilities with statevector: {   qjob_vec.result.probabilities(per_qubit=True)}\n")
print(f"Per qubit probabilities with density_matrix: {qjob_densmat.result.probabilities(per_qubit=True)}\n")
print(f"Per qubit probabilities with estimation: {    qjob_estimate.result.probabilities(per_qubit=True)}\n")

# Partial = [qubits_to_keep] reduces the bitstrings to just the selected indexes. Good for getting rid of ancillas
print(f"Partial [0, 1] probabilities with statevector: {   qjob_vec.result.probabilities(partial = [0, 1])}\n")
print(f"Partial [0, 1] probabilities with density_matrix: {qjob_densmat.result.probabilities(partial = [0, 1])}\n")
print(f"Partial [0, 1] probabilities with estimation: {    qjob_estimate.result.probabilities(partial = [0, 1])}\n")

# Partial + per_qubit returns the probabilites per_qubit of only the selcted qubits
print(f"Partial [0, 1] per qubit probabilities with statevector: {   qjob_vec.result.probabilities(per_qubit=True, partial = [0, 1])}\n")
print(f"Partial [0, 1] per qubit probabilities with density_matrix: {qjob_densmat.result.probabilities(per_qubit=True, partial = [0, 1])}\n")
print(f"Partial [0, 1] per qubit probabilities with estimation: {    qjob_estimate.result.probabilities(per_qubit=True, partial = [0, 1])}\n")

qdrop(family)