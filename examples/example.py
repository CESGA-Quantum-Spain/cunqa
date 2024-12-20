import os
import sys
import json

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


STORE = os.getenv("STORE")
conf_file = STORE + "/.api_simulator/qpu.json"

with open(conf_file, 'r', encoding='utf-8') as archivo:
    datos = json.load(archivo)

if isinstance(datos, dict):
    claves_primer_nivel = list(datos.keys())

for clave in claves_primer_nivel[1:]:
    print(clave)
    client = QClient(conf_file)
    client.connect(clave)
    future = client.send_circuit(circuit)

    print(future.get())
