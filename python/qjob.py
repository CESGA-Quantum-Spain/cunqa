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







class QJob():
    def __init__(self, QPU, circ, **run_parameters):

        self._QPU = QPU
        self._future = None


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


        self._execution_config = """ {{"config":{}, "instructions":{} }}""".format(run_config, instructions).replace("'", '"')
    

    def submit(self, test = False):
        self._test  = test

        if self._future is not None:
            raise JobError("QJob has already been submitted.")

        client = QClient("$STORE" + "/.api_simulator/qpu.json")

        print(self._QPU.server_id)

        client.connect(self._QPU.server_id)

        if not test:
            print("Submitting QJob to ", self._QPU.server_id)
            self._future = client.send_circuit(self._execution_config)
            print("QJob submited to ", self._QPU.server_id)
        elif test:
            print("QJob sent! (mentira)")


    def result(self):
        if self._future is not None:
            if not self._test:
                return Result(json.loads(self._future.get()))
            elif self._test:
                print("Result gotten!(mentira)")

    def state(self):
        print("Method not avaliable.")
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
        

    
    


