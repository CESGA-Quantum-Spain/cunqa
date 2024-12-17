from qiskit import QuantumCircuit
from qiskit.circuit import Parameter
import random
import sys



#qc = QuantumCircuit(2,2)
#qc.h(0)
#qc.cx(0,1)
#qc.x(1)
#qc.measure(1,0)
#qc.measure(0,1)
#qc.measure_all()

num_qubits = 35
qc = QuantumCircuit(num_qubits, num_qubits)

# Generar valores flotantes concretos para las rotaciones
theta_values = [random.uniform(0, 2 * 3.14159) for _ in range(num_qubits)]
phi_values = [random.uniform(0, 2 * 3.14159) for _ in range(num_qubits)]

# Añadir puertas soportadas al circuito
for qubit in range(num_qubits):
    qc.h(qubit)  # Hadamard
    qc.rx(theta_values[qubit], qubit)  # Rotación Rx con valor flotante
    qc.rz(phi_values[qubit], qubit)  # Rotación Rz con valor flotante
    qc.id(qubit)  # Puerta identidad
    if qubit % 3 == 0:
        qc.z(qubit)  # Puerta Z cada 3 qubits
    elif qubit % 3 == 1:
        qc.x(qubit)  # Puerta X
    elif qubit % 3 == 2:
        qc.y(qubit)  # Puerta Y

# Añadir entrelazamiento con puertas CX, CY, y CZ
for qubit in range(0, num_qubits - 1, 2):
    qc.cx(qubit, qubit + 1)  # Entrelazamiento con CX
    if qubit + 2 < num_qubits:
        qc.cy(qubit, qubit + 2)  # Entrelazamiento con CY
    if qubit + 3 < num_qubits:
        qc.cz(qubit, qubit + 3)  # Entrelazamiento con CZ

# Añadir medidas intermedias en algunos qubits
for qubit in range(0, num_qubits, 7):  # Medimos cada 7 qubits
    qc.measure(qubit, qubit)

# Añadir medidas finales en todos los qubits
qc.measure(range(num_qubits), range(num_qubits))




def from_qc_to_json(qc):
    json_data = {
        
        "qubits":0,
        "bits":0,
        "circuit":[]
    }
    for i in range(len(qc.data)):
        if qc.data[i].name == "barrier":
            pass
        elif qc.data[i].name != "measure":
            json_data["circuit"].append({"name":qc.data[i].name, 
                                              "qubits":[qc.data[i].qubits[j]._index for j in range(len(qc.data[i].qubits))],
                                              "params":qc.data[i].params
                                             })
        else:
            json_data["circuit"].append({"name":qc.data[i].name, 
                                              "qubits":[qc.data[i].qubits[j]._index for j in range(len(qc.data[i].qubits))],
                                              "memory":[qc.data[i].clbits[j]._index for j in range(len(qc.data[i].clbits))]
                                             })

    return json_data




qc_js = from_qc_to_json(qc)
print(qc_js)