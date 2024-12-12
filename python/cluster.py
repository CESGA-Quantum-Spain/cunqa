
import os
#import zmq
import zlib
import pickle, json
#from qiskit import * # qasm2, QuantumCircuit, transpile
#from qiskit_aer.backends.aer_simulator import *
#from python.backend_options import default_backends, allowed_backend_options, aer_default_configuration, all_allowed_backend_options, allowed_fakeqmio_options

#import dask.distributed
#import numpy as np
#from qmiotools.integrations.qiskitqmio import FakeQmio

from python.qclient import QClient



class Cluster():
    """
        Class to define a dask cluster
        The Cluster object has atributes related to the variables we need to submit a SLURM job, adapted to lauch a dask cluster.

        Class methods defined here:
        -----------
    """

    def __init__(self,n_qpus=1,time="08:00:00",modules=None,conda_env=None):
        """
            Initiates the Cluster with the SLURM job configuration. By default, Cluster will have 1 qpu and be up for 8 hours.

            Attributes:
            -----------
            n_qpus (int): Number of qpus that our virtual distributed quantum system will have. Note that the number of tasks for the
                slurm job will be equal to the number of qpus, that will correspond to the number of workers, plus 1 more
                task for the dask Scheduler (n_tasks=n_qpus+1).
                
            time (str): Time for the SLURM job. Note that when the time ends, the cluster will fall, therefore user will not be able to
                connect with the Client. It is also importat to have in mind the type of queue our job will be submited to: short (less
                than 8 hours), medium (from 8 to 24 hours) and long (from 24 hours up to 72 hours). The time must be written in a string
                as "(D-)HH:MM:SS".
                
            modules (str): name of modules the user wants to load. Note that we must load the same modules in our Cluster as the modules
                we will use in the job we send to the Client.
                
            conda_env (str): Name of the conda environment the user wants to activate. If the user is going to use such environment in
                their job, they must activate it as well for the cluster.
        """
        
        script=None
        if script is None:

            # defino el numero de tareas del trabajo de slurm y el tiempo
            self.slurm_ntasks=n_qpus+1; self.time=time

            header=f"""#!/bin/bash

            #SBATCH -n {self.slurm_ntasks} # Number of tasks
            #SBATCH -c 2 # Total number of core for one task
            #SBATCH --mem-per-cpu=4G
            #SBATCH -t {self.time}

            MEMORY_PER_TASK=$(( $SLURM_CPUS_PER_TASK*$SLURM_MEM_PER_CPU ))

            # Number of tasks 
            echo SLURM_NTASKS: $SLURM_NTASKS
            echo SLURM_NTASKS_PER_NODE: $SLURM_NTASKS_PER_NODE
            echo SLURM_CPUS_PER_TASK: $SLURM_CPUS_PER_TASK 
            echo SLURM_NNODES: $SLURM_NNODES
            echo SLURM_MEM_PER_CPU: $SLURM_MEM_PER_CPU
            echo MEMORY_PER_TASK: $MEMORY_PER_TASK
            """

            # cargo módulos necesarios
            if modules is not None:
                modules_load=""
                for m in modules:
                    modules_load.append(f"\nmodule load {m}")

            if conda_env is not None:
                conda_load=f"""
                module load qmio/hpc miniconda3/22.11.1-1
                conda init bash
                conda activate {conda_env}"""


            execute="""
            rm -f scheduler_info.json
            rm -f ssh_command.txt

            srun -n $SLURM_NTASKS \\
                -c $SLURM_CPUS_PER_TASK \\
                --resv-ports=$SLURM_NTASKS -l \\
                --mem=$MEMORY_PER_TASK \\
                python ./dask_cluster.py \\
                    -local $LUSTRE_SCRATCH \\
                    --dask_cluster \\
                    --ib
                        """
            

            with open('/dask-simulator/python/cluster/dask_submit_cluster.sh', 'w') as archivo:
                archivo.write(header)
                try:
                    archivo.write(modules_load)
                except:
                    pass
                try:
                    archivo.write(conda_load)
                except:
                    pass
                archivo.write(execute)


class QPU():
    """
        Class to define a QPU associated to a dask worker.

        Class methods defined here:
        -----------
    """
    
    def __init__(self, id_=None, server_endpoint=None, net="ib0", backend=None):
        """
        Initializes th QPU class.

        Attributes:
        -----------
        id_ (int): Id assigned to the qpu, simply a int from 0 to n_qpus-1.

        worker_endpoint (str): Ip and port route given by the dask Client for the worker assigned to the given qpu.

        server_endpoint (str): Ip and port route that identifies the server that represents the qpu itself which the worker will
            communicate with.
            
        backend (str): Name of the configuration file for the FakeQmio() class that the server will instanciate. By default is None,
            in that case the server will estabish a default configuration /opt/cesga/qmio/hpc/calibrations/2024_11_04__12_00_02.json.

        """

        if id_ is not None:
            self.id=id_
        else:
            raise ValueError("id not correct")

        # asignamos server
        if server_endpoint is not None and type(server_endpoint) is str:
            self.server_endpoint=server_endpoint
        else:
            try:
                STORE = os.getenv("STORE")
                with open(STORE + "/.api_simulator/qpu.json") as net_data:
                    net_json = json.load(net_data)
                    serv_end = net_json["{}".format(self.id)]["IPs"][net] + net_json["{}".format(self.id)]["port"]
                    self.server_endpoint = serv_end
            except:
                print("Problem when assigning server endpoint")

        # asignamos backend, será el nombre del archivo de configuración para FakeQmio
        if backend is None:
            self.backend = Backend(name = "ideal_aer")
        else:
            self.backend = Backend().from_json(name="Default_name" , config_json = backend)


    def run(self, circ, **run_parameters):
        if self.server_endpoint == None:
            print("QPU has no server endpoint")
            return 0
        else:
            context = zmq.Context()
            socket = context.socket(zmq.REQ)
            try:
                socket.connect(self.server_endpoint)
            except:
                dask.distributed.print("Server not found.")

            if type(circ) is str:
                circ_str=circ
            elif type(circ)  == QuantumCircuit:
                circ_str=qasm2.dumps(circ)
                print("Circuito pasado a qsam")
            else:
                print("Circuit format not valid: introduce a QuantumCircuit ot OpenQASM string.")
            """
        	Hay que trabajar los datos de entrada de la funcion para enviar un
		solo diccionario:
            """
            backend_dict = self.backend.get_dict()
            print("Creado el diccionario del backend: ", backend_dict)
            config_dict = _get_dict(circ_str, backend_dict, run_parameters)
            print("Creado el diccionario de configuracion: ", config_dict)
            config_dict_ser = json.dumps(config_dict)
            print("Serializado el diccionario de configuracion")
            socket.send_string(config_dict_ser)
            print("Enviado al server el diccionario de configuracion")

            result_str = socket.recv_string()
            #result_dict = json.loads(result_str)
            print(result_str)
            #return result_dict

    def c_run(self, circ, **run_parameters):
        if self.server_endpoint == None:
            print("QPU has no server endpoint")
            pass
        
        #if isinstance(circ, QuantumCircuit):
        #    circ_json = from_qc_to_json(circ)
        #elif isinstance(circ, str):
        circ_json = json.loads(circ) #qasm2_to_json(circ)
        circ_str = str(circ_json["instructions"])
        #else:
        #    print("Circuit format not valid")

        #execution_config = {"config":run_parameters, "instructions":circ_json["instructions"]}
        execution_config = {"config":run_parameters, "instructions":circ_str}
        execution_config_str = json.dumps(execution_config)
        #execution_config_str = json.dumps(execution_config)

        c_client = QClient()
        c_client.connect(self.id)
        c_client.send_data(execution_config_str)
        result = c_client.read_result()
        
        #c_client.send_data(close)
        #out = c_client.read_result()
       
        return result

def from_qc_to_json(qc):
    json_data = {
        "instructions":[]
    }
    for i in range(len(qc.data)):
        if qc.data[i].name == "barrier":
            pass
        elif qc.data[i].name != "measure":
            json_data["instructions"].append({"name":qc.data[i].name, 
                                              "qubits":[qc.data[i].qubits[j]._index for j in range(len(qc.data[i].qubits))],
                                              "params":"{}".format(qc.data[i].params)
                                             })
        else:
            json_data["instructions"].append({"name":qc.data[i].name, 
                                              "qubits":[qc.data[i].qubits[j]._index for j in range(len(qc.data[i].qubits))],
                                              "memory":[qc.data[i].clbits[j]._index for j in range(len(qc.data[i].clbits))]
                                             })

    return json_data
        




def qasm2_to_json(qasm_str):
    """
    Convierte un circuito en formato QASM a JSON estructurado.
    """
    lines = qasm_str.splitlines()
    json_data = {
        "qasm_version": None,
        "includes": [],
        "registers": [],
        "instructions": []
    }
    
    for line in lines:
        line = line.strip()
        if line.startswith("OPENQASM"):
            json_data["qasm_version"] = line.split()[1].replace(";", "")
        elif line.startswith("include"):
            json_data["includes"].append(line.split()[1].replace(";", "").strip('"'))
        elif line.startswith("qreg") or line.startswith("creg"):
            parts = line.split()
            register_type = parts[0]
            name, size = parts[1].split("[")
            size = int(size.replace("];", ""))
            json_data["registers"].append({"type": register_type, "name": name, "size": size})
        else:
            if line:  # Procesar operaciones cuanticas
                parts = line.split()
                operation_name = parts[0]
                if operation_name != "measure":
                    if "," not in parts[1]:
                        qubits = parts[1].lstrip("q[").rstrip("];")
                        json_data["instructions"].append({"name":operation_name, "qubits":[qubits]})

                    #qubits = parts[1].replace(";", "").split(",")
                    else:
                        parts_split = parts[1].rstrip(";").split(",")
                        q_first = parts_split[0].split("[")[1].rstrip("]")
                        q_second = parts_split[1].split("[")[1].rstrip("]")
                        json_data["instructions"].append({"name":operation_name, "qubits":[q_first, q_second]})
                else:
                    qubits = parts[1].split("[")[1].rstrip("]")
                    memory_aux = parts[3].rstrip(";")
                    if "meas" in memory_aux:
                        memory = memory_aux.lstrip("meas[").rstrip("]")
                    else:
                        memory = memory_aux.lstrip("c[").rstrip("]")
                    json_data["instructions"].append({"name":operation_name, "qubits":[qubits], "memory":memory})

    return json_data



class Backend():
    """
        Class to describe the backend to assign to a QPU.
    """
    


    def __init__(self, name="aer", config_json=None, **kwargs):

        """
        Parameters:
        ------------
        config_json (str): Name for the json file that already holds all the information needed for the backend to be defined. Default is None,
            so configuration will be described by the kwargs provided.
        
        name (str): the name will define the type of backend, for the first version, user can choose between "fakeqmio" or "aer", as well as
            "deafult fakeqmio" or "default aer" for default configurations.
            
        **kwargs : any arguments that <class 'qiskit_aer.backends.aer_simulator.AerSimulator'> takes.
        """

        self.name = name

        if self.name == "fakeqmio":
            
            if "calibration_file" in kwargs.keys():
                print("Creating FakeQmio from calibration file")
                with open("{}".format(kwargs["calibration_file"]),"r") as calibration_file:
                    calibrations_dict = json.load(calibration_file)
                    _FQ_dict = FakeQmio(calibration_file = calibration_dict, **kwargs).__dict__["_options"].__dict__
                    _FQ_dict["noise_model"] = _FQ_dict["noise_model"].to_dict()
                    self._backend_dict = _FQ_dict
            else:
                print("Creating default FakeQmio with the given arguments")
                _FQ_dict = FakeQmio(calibration_file = "default_qmio_calibration.json", **kwargs).__dict__["_options"].__dict__
                _FQ_dict["noise_model"] = _FQ_dict["noise_model"].to_dict()
                self._backend_dict = _FQ_dict

        elif self.name == "ideal_aer": # in default_backends.keys():
            #print("Creating default backend {} ".format(self.name))
            #self.from_json(name = self.name, config_json = default_backends[self.name])
            self._backend_dict = {}

        elif config_json != None:
            print("Creating backend from config_json")
            self.from_json(name=self.name, config_json = config_json)

        else:
            if not kwargs:
                print("Creating Ideal Aer Backend")
                self.from_json(name="ideal_aer", config_json = default_backends["ideal_aer"])

            else:
                print("Creating backend from arguments")
                self.from_json(name = self.name, config_json =  kwargs)


        
    def get_dict(self, save_json = False):
        """
            Method to get a dictionary with the class variables por the given Backend. If any value is as well an object, a dictionary
            of its class variables will be returned.

            Parameters:
            ----------
            save_json (bool or str): Defaul is False. If True, json file with deafult name "backend.json" is saved, if a str is given
                the json file is save with the given name.

            Return:
            ----------
            config_dict (dict): dictionary for the Backend configuration.
        """
        tipos_basicos = (int, float, complex, str, list, tuple, range, dict, set, frozenset, bool, bytes, bytearray, memoryview, type(None))
        config_dict = self.__dict__.copy() # copio el diccionario de las variables de clase
        for k, v in config_dict.items():
            if not isinstance(v, tipos_basicos): # busco si alguna es un objeto de una clase.
                config_dict[k] = v.__dict__ # las que lo sean, las sustituyo por su diccionario equivalente. En caso de que alguna tenga a su vez clases
                                            # dentro, pues tendríamos que hacer el mismo barrido.
        if save_json is True:
            default = "bakend.json"
            print("Saving configuration to json file: ", default)
        elif type(save_json) == str:
            print("Saving configuration to json file: ", save_json)
            ### en ambos tengo que ver como lo guardamos, adaptado al json de Jorge ###
        
        return config_dict

    def set(self, **kwargs):
        for k in kwargs.keys():
            setattr(self,k,kwargs[k])
            print(f"Configuration for {list(kwargs.values())}")

    def info(self):
        print(f"""--- Backend configuration ---""")
        for k, v in self.get_dict().items():
            if type(v) == dict:
                print(f"{k}: ")
                for j,m in v.items():
                    print(f"\t{j}: {m}")
            else:
                print(f"{k}: {v}")

    @classmethod
    def from_json(cls, name, config_json):
        cls._backend_dict = config_json 

        for key, value in cls._backend_dict.items():
            if key not in all_allowed_backend_options:
                print("Argument {} not allowed".format(key))
                break
            else:
                if key == "noise_model" and value != None and type(value) != dict:
                    cls._backend_dict[key] = value.to_dict()
                else:
                    pass



    ###### Esta funcion de momento no se usa #######
    def run(self, circ, run_parameters=None):
        if self.name == "aer":
            print("Instanciamos AerSimulator con los self.kwargs y le hacemos el run")
            #return AerSimulator(self.kwargs).run(circ, **run_parameters)
        
        elif self.name == "fakeqmio":
            print("Instanciamos FakeQmio con los self.kwargs y le hacemos el run")
            #return FakeQmio(self.kwargs).run(circ, **run_parameters)

        else:
            print("No valid simulator name")





global_qpus_dict={}

def link_QPUs(server_file="data.json", backends = None):
    """
        Global function that creates a dictionary of QPUs as a global variable.

        Parameters:
        -----------
       server_file (str): Name of the json file that has the information abaut the servers lauched so the workers can
            communicate with them to run the simulations.
            
        backends (<class 'qiskit_aer.backends.aer_simulator.AerSimulator'> or list of
            <class 'qiskit_aer.backends.aer_simulator.AerSimulator'>): Backend objet or objets that will be asigned to the given QPUs.
            By default is None, in that case the server will estabish default configuration for each QPU. If one backend is given,
            all QPUs of the cluster will have the same configuration, if a list of backends is given, each backend will be asigned
            to the corresponding QPU, in order. It is important for the backed list to have the same lenght as the number of workers
            and servers lauched, if not, IndexError will raise.


        Return:
        -----------
        qpus_dict (dict): dictionary of QPU objects with the corresponding class attributes described by the given
            configuration. Dictionary keys are ints from 0 to n_qpus-1, and for each QPU it is equal to QPU.id_.
        
    """

    global global_qpus_dict
    qpus_dict={}

    with open(server_file) as server_data:
        data = json.load(server_data)
        for key in list(data):
            server_endpoint="tcp://" + data[key]["ip"] + ":" + str(data[key]["port"])
            if backends is not None:
                qpus_dict[key]=QPU(id_=key, server_endpoint=server_endpoint, backend=backends[int(key)])
            else:# si no proporciono backend, la propia clase se inicializa con un default.
                qpus_dict[key]=QPU(id_=int(key), server_endpoint=server_endpoint)
    global_qpus_dict=qpus_dict
    return qpus_dict



def run(qpu, circ, run_parameters):
    result = qpu.run(circ, run_parameters)
    return result


def get_dict(circuit, backend_dict, run_parameters):

    run_parameters_dict = json.dumps(run_parameters)

    config_dict = dict(circuit=circuit, backend=backend_dict, run_arguments=run_parameters_dict)

    return config_dict



def dask_run(circuits, qpu_ids, client, run_parameters=None):
    """
        Function to run circuits through a dask Client. A task is sent to the QPU's corresponding dask worker which calls
        the function run().

        Parameters:
        -----------
        circuits (list or QuantumCircuit or OpenQASM str): circuit or circuits that will be run.

        qpu_ids (list of ints or int or None): list of the QPUS that will be used to run the circuits, or the single id if we were
            to use only one QPU. If we want to have several circuits, each one of them run in a different QPU, the lenght of
            the lists must be the same. Default is None, in this case if several circuits are submited, they will be run in arbitrary
            QPUs; if one circuit was given, it will be run in QPU with id '0'.

        run_parameters (dict or list): list of extra parameters for the simulation.

        Return:
        -----------
        client.gather(futures) (list): list that contains the outputs of run() for each worker that was submited a task.

        >>> dask_run([qc0,qc1,qc2],[0,0,1], client=client, run_paramters = {"shots":10})
            This will run qc0 and qc1 sequentially in QPU 0 and qc2 in QPU 1, all experiments with 10 shots.
                
        >>> dask_run( qc0, [0,1,2], client = client, run_parameters = [{"shots":10}, {"shots":20}, {"shots":30}])
            qc0 will be run in QPU 0 with 10 shots, in QPU 1 with 20 shots and in QPU 2 with 30 shots.

        >>> dask_run( [qc0,qc1,qc2], 0, client = client, run_paramters = [{"shots":10}, {"shots":20}, {"shots":30}])
            All circuits will be run in QPU 0 sequentially, but each of them with the given shots.
    """

    # hacemos la transpilación.


    if type(circuits) != list:# en caso de que yo tenga 1 circuito
        
        if qpu_ids is None: # no paso qpu_ids? lo ejecuto en la 0.solo 1 diccionario de run_parameters.
            qpu=global_qpus_dict[str(0)]
            futures=client.submit(run, qpu = qpu, circ = circuits, run_parameters = run_parameters, pure=False)
            
        elif type(qpu_ids) == int: # que le meto un qpu_ids? pues lo ejhecuta en esa, solo 1 diccionario de run parameters
            qpu=global_qpus_dict[str(qpu_ids)]
            futures=client.submit(run, qpu = qpu, circ = circuits, run_parameters = run_parameters, pure=False)
            
        elif type(qpus_ids) == list: # que le meto varios? pues me ejecuta ese mismo circuito en varias qpus (util para dsit de shots)
            futures = []
            if type(run_parameters) != list: # si le di varias configuraciones para el experimento que las use
                for i, id_ in enumerate(qpus_ids):
                    qpu=global_qpus_dict[str(id_)]
                    futures.append( client.submit(run, qpu = qpu, circ = circuits, run_parameters = run_parameters[i], pure=False))
            elif type(run_parameters) == dict: # si solo le di una, que la use para todos
                for i, id_ in enumerate(qpus_ids):
                    qpu=global_qpus_dict[str(id_)]
                    futures.append( client.submit(run, qpu = qpu, circ = circuits, run_parameters = run_parameters, pure=False))

    elif type(circuits) == list: # mando una lista de circuitos
                
        if qpu_ids is None: # que no digo nada de las qpus? los circutos se reparten entre todas arbitrariamente.
            if type(run_parameters) == list: # si me dieron una lista de diccionarios, cada circuito usa unos parámetros
                futures = []
                for i, c in enumerate(circuit):
                    qpu = global_qpus_dict[str( i % len(global_qpus_dict) )] # <= qpu arbitraria
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters[i], pure=False) )            
            elif type(run_parameters) == dict: # si me dieron solo un diccionario, todos los experimentos con esas caracteristicas
                futures = []
                for i, c in enumerate(circuit):
                    qpu = global_qpus_dict[str( i % len(global_qpus_dict) )] # <= qpu arbitraria
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters, pure=False) )                
        elif type(qpu_ids) == int: # que le doy un qpu id, uno solamente, entiendo que todos van a esa qpu.
            qpu=global_qpus_dict[str(qpu_ids)]
            if type(run_parameters) == list: # cada circuito con una configuración
                futures = []
                for i, c in enumerate(circuit):
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters[i], pure=False) )
            elif type(run_parameters) == dict: # todos los circuitos con la misma configuracion
                futures = []
                for i, c in enumerate(circuit):
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters, pure=False) )
                
        elif type(qpu_ids) == list: # que mando una lista de qpu_ids, cada circuito a una. TIENE QUE COINCIDIR LA LONGITUD DE LAS LISTAS!!
            if type(run_parameters) == list: # cada experimento con sus caracteristicas: circuito => qpu => parámetros
                futures = []
                for i, c in enumerate(circuit):
                    qpu = global_qpus_dict[str(qpu_ids[i])]
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters[i], pure=False) )
            elif type(run_parameters) == dict: # todo con las mismas características
                futures = []
                for i, c in enumerate(circuit):
                    qpu = global_qpus_dict[str(qpu_ids[i])]
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters, pure=False) )
    # recupero los resultados
    results = client.gather(futures)
    return results



def qpus_dask_mapper(function, iterable, client, qpu_ids):
    """
    PENDIENTE DE MODIFICAR !!!!
    Mapper function to implement in scipy.optimize optimizer, specifically differential_evolution(). This function
    must be passed as the `workers` parameter. Its purpose is to send tasks to the given QPUs.
    Parameters:
    -----------
    function (func): function that will be sent to the dask Client as a task.
    iterable (iterable): a interable object.
    client (<class 'distributed.Cluster'>): client that will submit tasks to the given workers.
    qpu_ids (list): list of ints that refer to the qpus that the user wants to use.
    Return:
    -----------
    List of results of the tasks.
    """
    futures = client.map(func, iterable)
    return [ f.result() for f in futures ]



################### IMPLEMENTACION CON POLYPUS #########################

def get_qpus(num_qpus, list_config_dict):
    # Esta funcion tiene que hacer el link_qpus y devolver un dicctionario de objetos qpu
    # {"qpu0":objeto, "qpu1", objeto, ...}"
    pass



################### FIN IMPLEMENTACION POLYPUS ##########################








################# RUN ANTIGUO #####################
def __run(circ, qpu=None, parameters=None):
    """
        Global function to run a circuit in a given QPU. It connects to the corresponding server and gives information
        about the backend if needed.

        Parameters:
        -----------
        circ (QuantumCircuit or QASM str): Circuit that will be run. It can be introduced as a QuantumCircuit object
            or user can submit directly the OpenQASM code.

        qpu (<class 'cluster.QPU'>): QPU where the circuit will be run.

        Return:
        -----------
        result (<class 'qiskit.result.result.Result'>): result of the simulation of the circuit.
    """
    
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    
    try:
        socket.connect(qpu.server_endpoint)
    except:
        dask.distributed.print("Server not found.")
        return None
    
    if type(circ) is str:
        circ_str=circ
    elif type(circ) is QuantumCircuit:
        circ_str=qasm2.dumps(circ)
    else:
        print("Circuit format not valid: introduce a QuantumCircuit ot OpenQASM string.")

    # envío el circuito
    socket.send_string(circ_str, zmq.SNDMORE)

    # en caso de que le pase parámetros adicionales,
    
    
    if parameters is not None:
        socket.send(b'\x01', zmq.SNDMORE)
        # por ahora se van a leer así en el server, ya adaptaremos despúes a hacer un diccionario como dios manda
        print("VOY A ENVIAR LOS PARÁMETROS")
        initial_layout, shots, repetition_period = parameters
        socket.send(np.array(initial_layout).tobytes(), zmq.SNDMORE)
        socket.send(np.array(shots).tobytes(), zmq.SNDMORE)
        socket.send(np.array(repetition_period).tobytes())
    else:
        socket.send(b'\x00')
    
    results = socket.recv_pyobj()
    
    # TODO: hacer las cosas con zip (ahora da problemas el pickle)
    # #results = zlib.decompress(zip_results)
    #result = pickle.load(pick_results)
    return results.get_counts()


#######################FINAL RUN ANTIGUO########################











########################## EMPIEZA PARTE ALVARO ##################################

class _QPU:
    def __init__(self, id=None, backend=None, server_id=None):

        if backend==None:
            self.backend = Backend(name="aer") # Backend(name="fakeqmio")
        elif isinstance(backend, Backend):
            self.backend = backend
        #else:
            #back_aux = Backend()
            #back_aux.__backend = backend
            #self.backend = back_aux
        if server_id != None:
            repo_path = os.getenv("REPO_PATH")
            with open("{}/tmp/data.json".format(repo_path)) as server_data:
                data = json.load(server_data)
                self.server_endpoint = "tcp://" + data["{}".format(server_id)]["ip"] + ":" + str(data["{}".format(server_id)]["port"])
                self.linked = True
                self.id = server_id
                
        else:
            self.id = id
            self.linked = False

    def _link_qpu(self, server_id):
        repo_path = os.getenv("REPO_PATH")
        with open("{}/tmp/data.json".format(repo_path)) as server_data:
            data = json.load(server_data)
            self.server_endpoint = "tcp://" + data["{}".format(server_id)]["ip"] + ":" + str(data["{}".format(server_id)]["port"])
            self.linked = True
            self.id = server_id
    
    
    def _run(self, circ, **run_parameters):
        if self.server_endpoint == None:
            print("QPU has no server endpoint")
            return 0
        else:
            context = zmq.Context()
            socket = context.socket(zmq.REQ)
            try:
                socket.connect(self.server_endpoint)
            except:
                dask.distributed.print("Server not found.")

            if type(circ) is str:
                circ_str=circ
            elif type(circ) is QuantumCircuit:
                circ_str=qasm2.dumps(circ)
            else:
                print("Circuit format not valid: introduce a QuantumCircuit ot OpenQASM string.")
            """
        	Hay que trabajar los datos de entrada de la funcion para enviar un
		solo diccionario:
            """
            print("vamos a instanciar config_dict")
            config_dict = _get_dict(circ_str, self.backend.get_dict(), **run_parameters)
            print("conf_dict", config_dict)
            config_dict_ser = json.dumps(config_dict)
            socket.send_string(config_dict_ser)
            
            result_str = socket.recv_string()
            #result_dict = json.loads(result_str)
            print(result_str)
            print("Diccionario con resultados, deserializado")
            return result


    def _submit(self, circ, **run_parameters):
        if self.server_endpoint == None:
            print("QPU has no server endpoint")
            return 0
        else:
            context = zmq.Context()
            socket = context.socket(zmq.REQ)
            try:
                socket.connect(self.server_endpoint)
            except:
                dask.distributed.print("Server not found.")

            if type(circ) is str:
                circ_str=circ
            elif type(circ) is QuantumCircuit:
                circ_str=qasm2.dumps(circ)
            else:
                print("Circuit format not valid: introduce a QuantumCircuit ot OpenQASM string.")
            """
        	Hay que trabajar los datos de entrada de la funcion para enviar un
		solo diccionario:
            """
            print("vamos a instanciar config_dict")
            config_dict = _get_dict(circ_str, self.backend.get_dict(), **run_parameters)
            print("conf_dict", config_dict)
            config_dict_ser = json.dumps(config_dict)
        
        experiment = Experiment(qpu = self, config_dict = config_dict)
        return experiment


    def _execute(experiment):
        print("Uso la funcion de c++ para ejecutar el circuito")

    """
    1) Install pybind11: pip install pybind11

    2) Create a C++ File with Bindings: Save the following in example.cpp:
    #include <pybind11/pybind11.h>

    int add(int a, int b) {
        return a + b;
    }

    PYBIND11_MODULE(example, m) {
        m.def("add", &add, "A function that adds two numbers");
    }

    3) Compile It: Use pybind11's build command:
    c++ -O3 -Wall -shared -std=c++11 -fPIC $(python3 -m pybind11 --includes) example.cpp -o example$(python3-config --extension-suffix)

    4) Use It in Python::
    import example
    print(example.add(5, 7))  # Output: 12

    """





    # No se usa este trasnpile
    def _transpile(self, circuit, **transpile_args):
        transpiled_circuit = transpile(circuit, self.backend.__backend, **transpile_args)
        return transpiled_circuit



# Ahora mismo esta clase no se usa
class Experiment:
    def __init__(self, qpu, config_dict):
        self.qpu = qpu
        self.config_dict = config_dict
        self.config_dict_ser = json.dumps(config_dict)



def _get_dict(circuit, backend_dict, run_parameters):

    run_parameters_dict = json.dumps(run_parameters)

    config_dict = dict(circuit=circuit, backend=backend_dict, run_arguments=run_parameters_dict)

    return config_dict


def _run(qpu, circ, run_parameters):
    result = qpu.run(circ, run_parameters)
    return result




def link_several_qpus(list_qpus, list_server_ids):
    if len(list_qpus) != len(list_sever_ids):
        print("Different number of qpus and servers")
        pass
    else:
        with open("data.json") as server_data:
                data = json.load(server_data)
                for i in len(list_qpus):
                    list_qpus[i].server_endpoint = "tcp://" + data[list_server_ids[i]]["ip"] + ":" + str(data[list_server_ids[i]]["port"])
                    list_qpus[i].linked = True
                    list_qpus[i].id = list_server_ids[i]


############################ FINAL PARTE ALVARO #############################



