import os
import zmq
import subprocess
from qmiotools.integrations.qiskitqmio import FakeQmio
from qiskit import transpile, QuantumCircuit
import pickle, json
import zlib
import numpy as np

from cluster import *

class QPU:  #Hay dos clases que se llaman QPU, llamarle a esta de otra forma: QPUServer?
    def __init__(self):
        pass

    def execute_circuit(self, qasm, parameters=None):
        fakeqmio=FakeQmio(gate_error = True, readout_error = True)
        qc = QuantumCircuit.from_qasm_str(qasm)
        if parameters is not None:
            initial_layout, shots, repetition_period = parameters
            qc_transpiled=transpile(qc, fakeqmio, optimization_level = 1, initial_layout = initial_layout)
            resultados= fakeqmio.run(qc_transpiled, shots = shots, repetition_period = repetition_period).result()
            return resultados
        else:
            qc_transpiled=transpile(qc, fakeqmio)
            resultados= fakeqmio.run(qc_transpiled).result()
            return resultados

    def start_server(self):
        context = zmq.Context()
        socket = context.socket(zmq.REP)
        
        repo_path = os.getenv("REPO_PATH")
        result = subprocess.run([repo_path + '/tmp/src/netinfo'], check=True, text=True, capture_output=True)

        socket.bind("tcp://" + result.stdout)
        
        print(f"Servidor ZMQ escuchando en el endpoint {result.stdout}")

        while True:
            # Esperar un mensaje del cliente
            message = socket.recv_string()
            print(f"Mensaje recibido: {message}")

            # recibo los demás argumentos para execute_circuit()
            si_o_no = socket.recv() == b'\x01'

            print("Recibido: ", si_o_no)

            if si_o_no:
                data = socket.recv()
                initial_layout = [int(p) for p in np.frombuffer(data, dtype=int)]
                print("initial_layout: ", initial_layout)
                data2= socket.recv()
                n_shots = int(np.frombuffer(data2, dtype=int)[0])
                print("n_shots: ", n_shots)
                data3= socket.recv()
                repetition_period =float(np.frombuffer(data3, dtype=float)[0])
                print("repetition_period: ", repetition_period)

                parameters = initial_layout, n_shots, repetition_period
                
                results = self.execute_circuit(message, parameters)
                
            else:
                results = self.execute_circuit(message)

            #if data is not None:
            #    parameters = [ np.frombuffer(d, dtype=np.float32) for d in data] 
            #    results = self.execute_circuit(message, parameters)
            #else:
            #pickle_results = pickle.dumps(results, -1)
            #zip_results = zlib.compress(results)
            socket.send_pyobj(results)


    def _start_server(self):
        context = zmq.Context()
        socket = context.socket(zmq.REP)
        
        repo_path = os.getenv("REPO_PATH")
        result = subprocess.run([repo_path + '/tmp/src/netinfo'], check=True, text=True, capture_output=True)

        socket.bind("tcp://" + result.stdout)
        
        print(f"Servidor ZMQ escuchando en el endpoint {result.stdout}")

        while True:
            config_dict = socket.recv_string()
            print("Llego el diccionario de configuracion al servidor")
            socket.send_string("Vuelta desde el servidor")


#############VERSION ANTIGUA################
            #circ_str = socket.recv_string()
            #print("Circuito en formato qasm, recibido")
            #circ = 0 #OJO: PASAR DE QASM A QUANTUMCIRCUIT (O NO. EN ESTE CASO, MODIFICAR backend._run PARA QUE ACEPTE CIRCUITOS EN QASM)
            #back_dict_ser = socket.recv_string()
            #print("Diccionario serializado con informacion sobre el backend, recibido")
            #run_dict_ser = socket.recv_string()
            #print("Diccionario serializado con los run arguments, recibido")
            
            #back_dict = json.loads(back_dict_ser)
            #print("Diccionario con la informacion sobre el backend, deserializado")
            #run_dict = json.loads(run_dict_ser)
            #print("Diccionario con los run arguments, deserializado")

            #if name == 'fakeqmio':
            #    backend = FakeQmio()
            #elif name == 'aer':
            #    backend = AerSimulator()
            #else:
            #    print("Backend name not supported")

            #res = backend.run(circ, **run_dict)

            #Hay que pasar el objeto res a diccionario y llamarle res_dict
            #res_dict_ser = json.dumps(res_dict)
            
            #socket.send_string(res_dict)


if __name__ == "__main__":
    #QPU().start_server()
    QPU()._start_server()

