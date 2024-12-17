import os
import sys

install_path = os.getenv("INSTALL_PATH")
sys.path.insert(0, install_path)
sys.path.insert(0, "/mnt/netapp1/Store_CESGA/home/cesga/mlosada/api/api-simulator/python")

from qpu import QPU, getQPUs

lista = getQPUs()

print("QPUs we are going to work with: ")
print(" ")
for q in lista:
    print("QPU backend info:")
    print(" ")
    q.backend.info()


# we define a circuit, first as a json

circuit = {"instructions": [
    {
        "name": "h",
        "qubits": [0]
    },
    {
        "name": "cx",
        "qubits": [0, 1]
    },
    {
        "name": "measure",
        "qubits": [0],
        "memory": [0]
    },
    {
        "name": "measure",
        "qubits": [1],
        "memory": [1]
    }
    ]
}

result = lista[0].run(circ = circuit, shots=222)

print(result)
print(result.get_counts())







