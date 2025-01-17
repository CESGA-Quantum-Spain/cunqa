import os
import sys
import json
import time

install_path = os.getenv("INSTALL_PATH")
sys.path.insert(0, install_path)

#print(os.getenv("PATH"))

from python.qclient import QClient
#from python.cluster import QPU, from_qc_to_json, qasm2_to_json




#client = QClient(STORE + "/.api_simulator/qpu.json")


#qc = QuantumCircuit(2)
#qc.h(0)
#qc.cx(0,1)
#qc.measure_all()

#qc_json = from_qc_to_json
#print("QuantumCircuit en json", qc_json)

#qpu = QPU(id_=0)

#back = qpu.backend
#print("Backend", back)

#se = qpu.server_endpoint
#print("Server endpoint", se)





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
        "method": "density_matrix",
        "memory_slots": 7
    },
    "instructions": [
    {"name": "rz", "qubits": [0], "params": [1.5707963267948966]}, 
    {"name": "sx", "qubits": [0], "params": []}, 
    {"name": "rz", "qubits": [1], "params": [-3.141592653589793]}, 
    {"name": "sx", "qubits": [1], "params": []}, 
    {"name": "rz", "qubits": [1], "params": [-3.141592653589793]}, 
    {"name": "ecr", "qubits": [0, 1], "params": []}, 
    {"name": "sx", "qubits": [0], "params": []}, 
    {"name": "sx", "qubits": [0], "params": []}, 
    {"name": "measure", "qubits": [0], "memory": [0]}, 
    {"name": "measure", "qubits": [1], "memory": [1]}
    ] 
}"""



#result = qpu.c_run(circuit, shots=199)

STORE = os.getenv("STORE")
conf_file = STORE + "/.api_simulator/qpu.json"

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
future2 = client.send_circuit(qc)

print("Futures creados.")

print("GET DEL FUTURE 1:" + future1.get())
print("GET DEL FUTURE 2:" + future2.get())

""" for fut in future:
    print(fut.get()) """

""" result_dict = json.loads(result)

counts = result_dict['results'][0]['data']['counts']

print(counts) """
