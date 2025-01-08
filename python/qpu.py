import os
import sys
import pickle, json

# path para acceder a los paquetes de c++
installation_path = os.getenv("INSTALL_PATH")
sys.path.append(installation_path)

# path para acceder a la informacion sobre las qpus
info_path = os.getenv("INFO_PATH")

# importamos api en C++
from python.qclient import QClient
# importamos la clase Backend
from backend import Backend
from qjob import QJob, gather
# importamos funciones para transformar circuitos a json
from circuit import qasm2_to_json, qc_to_json


class QPU():
    """
        Class to define a QPU.

        Class methods defined here:
        -----------
    """
    
    def __init__(self, id_=None, server_id=None, backend=None):
        """
        Initializes th QPU class.

        Attributes:
        -----------
        id_ (int): Id assigned to the qpu, simply a int from 0 to n_qpus-1.

        server_endpoint (str): Ip and port route that identifies the server that represents the qpu itself which the worker will
            communicate with.
            
        backend (str): Name of the configuration file for the FakeQmio() class that the server will instanciate. By default is None,
            in that case the server will estabish a default configuration /opt/cesga/qmio/hpc/calibrations/2024_11_04__12_00_02.json.

        """

        # id
        if id_ == None:
            print("QPU id not provided.")
            return
        elif type(id_) == int:
            self.id_ = id_
        else:
            print("QPU id must be int.")
            return

        # server_id
        if server_id == None:
            print("QPU server not assigned.")
            return
        elif type(server_id) == str:
            self.server_id = server_id
        else:
            print("Invalid server id.")
            return

        # backend
        if backend == None:
            print("QPU has no backend info.")
            return
        elif isinstance(backend, Backend):
            self.backend = backend
        else:
            print("backend type must be class Backend.")
            return


    def run(self, circ, **run_parameters):
        """
            Class method to run a circuit in the QPU.

            Args:
            --------
            circ (json): circuit to be run in the QPU.
            **run_parameters : any simulation instructions such as shots, method, parameter_binds, meas_level, init_qubits, ...

            Return:
            --------
            Result object.
        """
        qjob = QJob(self, circ, **run_parameters)
        qjob.submit(test = True)
        return qjob

    
        

    def _run(self, circ, **run_parameters):
        """
            Class method to run a circuit in the QPU.

            Args:
            --------
            circ (json): circuit to be run in the QPU.
            circ (Quantumcircuit or QASM or json): circuit to be run in the QPU.
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
            
    
        run_config = {"shots":1024, "method":"statevector", "memory_slots":7}
        
        if len(run_parameters) == 0:
            pass
        else:
            for k,v in run_parameters.items():
                run_config[k] = v
        
        try:
            instructions = circuit['instructions']
        except:
            raise ValueError("Circuit format not valid, only json is supported.")


        execution_config = """ {{"config":{}, "instructions":{} }}""".format(run_config, instructions).replace("'", '"')
        
        STORE = os.getenv("STORE")
        client = QClient(STORE + "/.api_simulator/qpu.json")
        
        client.connect(self.id_)
        client.send_data(execution_config)
        result = client.read_result()
        client.send_data("CLOSE")
        return Result(json.loads(result))


def getQPUs():
    """
        Global function to get the QPU objects corresponding (......)

        Return:
        ---------
        List of QPU objects.
    
    """

    with open(info_path, "r") as qpus_json:
        dumps = json.load(qpus_json)

    qpus = []
    i = 0
    for k, v in dumps.items():# instancio nuestra clase backend pasándole directamente el diccionario
        qpus.append(  QPU(id_ = i, server_id = k, backend = Backend(v['backend'])  )  )
        i+=1

    return qpus




        
