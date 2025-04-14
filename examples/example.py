import os
import sys
import json
import time

install_path = os.getenv("INSTALL_PATH")
sys.path.insert(0, install_path)
from python.qclient import QClient

circuit = """
{
    "config": {
        "shots": 1024,
        "method": "statevector",
        "memory_slots": 7
    },
    "instructions": [
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
""" 

qc = """ { 
    
    "config": {
        "shots": 1024,
        "method": "automatic",
        "memory_slots": 7
    },
    "instructions": [
    {"name": "rz", "qubits": [2], "params": [1.5707963267948966]}, 
    {"name": "sx", "qubits": [2], "params": []}, 
    {"name": "rz", "qubits": [1], "params": [-3.141592653589793]}, 
    {"name": "sx", "qubits": [1], "params": []}, 
    {"name": "rz", "qubits": [1], "params": [-3.141592653589793]}, 
    {"name": "ecr", "qubits": [2, 1], "params": []}, 
    {"name": "sx", "qubits": [2], "params": []}, 
    {"name": "sx", "qubits": [2], "params": []}, 
    {"name": "measure", "qubits": [2], "memory": [0]}, 
    {"name": "measure", "qubits": [1], "memory": [1]}
    ] 

} """

params = """ {
    "parameters":[1.11111, 2.22222, 3.3333]
} """

STORE = os.getenv("STORE")
conf_file = STORE + "/.cunqa/qpus.json"

with open(conf_file, 'r', encoding='utf-8') as archivo:
    datos = json.load(archivo)

if isinstance(datos, dict):
    claves_primer_nivel = list(datos.keys())

#for clave in claves_primer_nivel:
client = QClient(conf_file)
print("Cliente instanciado")

client.connect(claves_primer_nivel[0])
print("Cliente conectado")

print("Cliente: " + claves_primer_nivel[0])
future1 = client.send_circuit(qc)
#future2 = client.send_circuit(qc)
#future_param = client.send_parameters(params)

print("Futures creados.")

print("GET DEL FUTURE 1:" + future1.get())
#print("GET DEL FUTURE 2:" + future2.get())
#print("RESULT DEL PARAMETERS:" + future_param.get())