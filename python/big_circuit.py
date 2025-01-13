import json
import time
import qiskit
from qiskit import QuantumCircuit, ClassicalRegister, QuantumRegister, transpile, assemble
from qiskit.circuit.library import ZGate
from qiskit_aer import Aer
from translator import from_qc_to_json



def MCZ(n_qubits):
    if n_qubits == 1:
        return ZGate()
    else:
        return ZGate().control(n_qubits-1)
    

def regularOracle(qubits, target):
    
    # First, create the circuit, then add the appropriate X gates according to the target state, then add DMCZ, and again X gates

    circ = QuantumCircuit(qubits)


    # This is to apply the X gates to the appropriate register
    for (i,t) in enumerate(target):
        if t=='0':     
            circ.x(i)
    circMCZ = MCZ(qubits)
    circ=circ&circMCZ
    for (i,t) in enumerate(target):
        if t=='0':     
            circ.x(i)
    return circ

def regularDiffuser(qubits):
    
    # First, create the circuit, then add the appropriate H gates according to the target state, then add DMCZ, and again H gates

    circ = QuantumCircuit(qubits)
    
    # This is to apply the H gates to all data qubits 
    for i in range(qubits):
        circ.h(i)
    circMCZ = MCZ(qubits)
    circ=circ&circMCZ
    for i in range(qubits):
        circ.h(i)
        
    return circ


def regularGrover(target, num_layers=None, verbose=True):
    
    num_qubits = len(target)    
    
    # If not specified, use Nielsen & Chuang estimation for optimal number of layers'?
    if num_layers==None:
        num_layers = int(np.floor((np.pi / 4) * np.sqrt(2 ** num_qubits) - 0.5))
        
    if verbose==True:        
        print('Number of layers: ',num_layers)


    # First, create the circuit, start in the state |-> in the data qubits, then add for each layer oracle+diffuser 

    circ = QuantumCircuit(num_qubits,num_qubits)
 
    
    for i in range(num_qubits):
        circ.x(i)    
        circ.h(i)    
            
    oracle = regularOracle(num_qubits, target)
    
    diffuser = regularDiffuser(num_qubits)
    
    for layer in range(num_layers):
        circ = circ&oracle
       
        circ = circ&diffuser

        
    # Measure the data qubits
    for i in range(num_qubits):
        circ.measure(i,i)
    


    return circ

num_qubits = 25
num_layers = 50

target = ""
target = target.join(['0' for i in range(num_qubits)])



cc0 = regularGrover(target,num_layers=num_layers, verbose=False)

backend = Aer.get_backend('aer_simulator')

start_time = time.time()

tc0 = transpile(cc0, backend, basis_gates = {'h','x','y','z','cx','cy','cz','rx','ry','rz'})
print(tc0.count_ops())

end_time = time.time()

print(f"Tiempo en transpilar circuito: {end_time - start_time:.6f} segundos")


start_time = time.time()

json_data = from_qc_to_json(tc0)

end_time = time.time()

print(f"Tiempo en convertir a json: {end_time - start_time:.6f} segundos")



json_circuit = json_data["circuit"]

#with open('json_circuit.txt', 'w') as file:
#    json.dump(json_circuit, file)



