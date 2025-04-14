import os
import sys
import json
import time

install_path = os.getenv("INSTALL_PATH")
sys.path.insert(0, install_path)
from cunqa.qclient import QClient

# Define the OpenQASM 3.0 circuit as a string
qasm = """OPENQASM 3.0;
include "stdgates.inc";

qubit[2] q;
bit[2] c;

h q[0];
cx q[0], q[1];
measure q -> c;
"""

msg = json.dumps(
    {
        "config": {
            "shots": 1024,
            "method": "density_matrix",
            "memory_slots": 7,
            "seed": 12342
        },
        "instructions": f"{qasm}"
    }, ensure_ascii=False).replace("'", '"')

print(msg)

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
future1 = client.send_circuit(msg)
future2 = client.send_circuit(msg)

print("Futures creados.")

resultado = future1.get()

hola = json.loads(resultado)
#print(hola)

print("GET DEL FUTURE 1:" + json.dumps(hola["time_taken"]))

print("GET DEL FUTURE 2:" + future2.get())
