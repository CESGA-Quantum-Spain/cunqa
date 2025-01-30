from concurrent.futures import ThreadPoolExecutor
import os
import sys
import pickle, json
import time
from qiskit import QuantumCircuit
from circuit import qc_to_json
from transpile import transpiler

# path para acceder a los paquetes de c++
installation_path = os.getenv("INSTALL_PATH")
sys.path.append(installation_path)


# importamos api en C++
from python.qclient import QClient


class QJobError(Exception):
    """Exception for error during job submission to QPUs. """
    def __init__(self, mensaje):
        super().__init__(mensaje)

def divide(string, lengths):
    parts = []
    init = 0
    for length in lengths:
        parts.append(string[init:init + length])
        init += length
    return ' '.join(parts)



class Result():
    def __init__(self, result, registers = None):
        if type(result) == dict:
            self.result = result
        else:
            raise TypeError("Result format not supported, must be dict or list.")
            return

        for k,v in result.items():
            if k == "metadata":
                for i, m in v.items():
                    setattr(self, i, m)
            elif k == "results":
                for i, m in v[0].items():
                    if i == "data":
                        counts = m["counts"]
                    elif i == "metadata":
                        for j, w in m.items():
                            setattr(self,j,w)
                    else:
                        setattr(self, i, m)
            else:
                setattr(self, k, v)

        self.counts = {}
        for j,w in counts.items():
            if registers is None:
                self.counts[format( int(j, 16), '0'+str(self.num_clbits)+'b' )]= w
            elif isinstance(registers, dict):
                lengths = []
                for v in registers.values():
                    lengths.append(len(v))
                self.counts[divide(format( int(j, 16), '0'+str(self.num_clbits)+'b' ), lengths)]= w
                    
        
    def get_dict(self):
        return self.result

    def get_counts(self):
        return self.counts



class QJob():
    def __init__(self, QPU, circ, transpile, **run_parameters):

        self._QPU = QPU
        self._future = None
        self._result = None

        # compruebo si hay que realizar la transpilación

        if transpile:
            circt = transpiler( circ, QPU.backend )
        else:
            circt = circ

        # convierto el circuito a json

        if isinstance(circt, dict):
            circuit = circt
            
        elif isinstance(circt, QuantumCircuit):
            circuit = qc_to_json(circt)

        else:
            raise TypeError("Circuit format not valid, only json and <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'> are supported.")
        
        self._circuit = circuit
        # elaboro el run config
    
        run_config = {"shots":1024, "method":"statevector", "memory_slots":circuit["num_clbits"]}
        

        # añado parámetros relacionados con la simulación que el usuario intruduce como kwargs en el .run()

        if run_parameters == None:
            pass
        elif type(run_parameters) == dict:
            for k,v in run_parameters.items():
                run_config[k] = v
        
        try:
            instructions = circuit['instructions']
        except:
            raise TypeError("Circuit format not valid, only json and <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'> are supported.")# mirar bien!!!!

        # creo el string que se mandará
      
        self._execution_config = """ {{"config":{}, "instructions":{} }}""".format(run_config, instructions).replace("'", '"')
    


    def submit(self):
        if self._future is not None:
            raise QJobError("QJob has already been submitted.")
        print(self._execution_config)
        self._future = self._QPU._qclient.send_circuit(self._execution_config)

    def result(self):
        if self._future is not None and self._future.valid():
            if self._result is None:
                self._result = Result(json.loads(self._future.get()), registers=self._circuit['classical_registers'])
                #self._result = json.loads(self._future.get())
            return self._result

    def time_taken():
        if self._future is not None and self._future.valid():
            if self._result is not None:
                return self._result.get_dict()["results"][0]["time_taken"]
            else:
                raise QJobError("QJob is not finished.")
        else:
            raise QJobError("No QJob submited.")
        


        

def gather(qjobs):
    """
        Function to get result of several QJob objects, it also takes one QJob object.

        Args:
        ------
        qjobs (list of QJob objects or QJob object)

        Return:
        -------
        Result or list of results.
    """
      
        
               
        
