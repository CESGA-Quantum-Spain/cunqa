from concurrent.futures import ThreadPoolExecutor
import os
import sys
import pickle, json
import time

# path para acceder a los paquetes de c++
installation_path = os.getenv("INSTALL_PATH")
sys.path.append(installation_path)

# path para acceder a la informacion sobre las qpus
info_path = os.getenv("INFO_PATH")

# importamos api en C++
from python.qclient import QClient

class Result():
    def __init__(self, result):
        if type(result) == dict:
            self.result = result
        else:
            print("Result format not supported, must be dict or list.")
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
            self.counts[format( int(j, 16), '0'+str(self.num_qubits)+'b' )]= w
        
    def get_dict(self):
        return self.result

    def get_counts(self):
        return self.counts


def _run(QPU_id, circ, run_parameters):
        """
            Class method to run a circuit in the QPU.

            Args:
            --------
            circ (json): circuit to be run in the QPU.
            **run_parameters : any simulation instructions such as shots, method, parameter_binds, meas_level, init_qubits, ...

            Return:
            --------
            Result in a dictionary
        """

        #if type(circ) == str:
        #    if circ.lstrip().startswith("OPENQASM"):
        #        circuit = qasm2_to_json(circ)
        #    else:
        #        circuito = None
                
        if isinstance(circ, dict):
            circuit = circ

        else:
            circuit = None
            
    
        run_config = {"shots":1024, "method":"statevector", "memory_slots":circ["num_clbits"]}
        
        if run_parameters == None:
            pass
        elif type(run_parameters) == dict:
            for k,v in run_parameters.items():
                run_config[k] = v
        
        try:
            instructions = circuit['instructions']
        except:
            raise ValueError("Circuit format not valid, only json is supported.")


        execution_config = """ {{"config":{}, "instructions":{} }}""".format(run_config, instructions).replace("'", '"')

    
        print("\t [",QPU_id,"]:\tSearching for QClient...")
        STORE = os.getenv("STORE")
        client = QClient(STORE + "/.api_simulator/qpu.json")
        print("\t [",QPU_id,"]:\tFound QClient: ", client)
        print(" ")
        print("\t [",QPU_id,"]:\tConecting to QPU ", QPU_id)
        client.connect(QPU_id)
        print("\t [",QPU_id,"]:\tSuccessfully conected to QPU ", QPU_id,".")
        print(" ")
        print("\t [",QPU_id,"]:\tSending data ...")
        client.send_data(execution_config)
        print("\t [",QPU_id,"]:\tData sent.")
        print(" ")
        print("\t [",QPU_id,"]:\tReading result...")
        result = client.read_result()
        print("\t [",QPU_id,"]:\tResult read.")
        print(" ")
        print("\t [",QPU_id,"]:\tShutting down QPU ", QPU_id,"...")
        client.send_data("CLOSE")

        return Result(json.loads(result))




class QJob():
    def __init__(self, QPU, circuit, **run_parameters):

        self._QPU = QPU
        self._circuit = circuit
        self._run_parameters = run_parameters
        self._executor = ThreadPoolExecutor(max_workers=4)
        self._future = None

    #def __str__(self):
        

    def submit(self):
        if self._future is not None:
            raise JobError("QJob has already been submitted.")
        print("Submitting QJob to ", self._QPU.server_id)
        self._future = self._executor.submit(_run, self._QPU.server_id, self._circuit, self._run_parameters)
        print("QJob submited to ", self._QPU.server_id)
        return self._future


    def result(self, timeout=None):
        return self._future.result(timeout=timeout)

    def state(self):
        if self._future is None:
            print("QJob not submited.")
            return None
        elif self._future.done():
            return "DONE"
        elif self._future.running():
            return "PENDING"
        else:
            raise Error("Future not found.")


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
    if isinstance(qjobs, QJob):
        return qjobs.result()
    elif type(qjobs) == list:
        return [qj.result() for qj in qjobs]
    else:
        raise ValueError("Format invalid, qjobs must be QJob objet or list of QJob objects.")
        
















    
    


