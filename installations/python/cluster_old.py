
import os
import zmq
import zlib
import pickle
from qiskit import qasm2, QuantumCircuit
import dask.distributed
import numpy as np
from qmiotools.integrations.qiskitqmio import FakeQmio

#from qmiotools.qmiotools.integrations.qiskitqmio import FakeQmio

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

class Backend():

    def __init__(self, name=None, **kwargs):
        # el nombre si que lo tengo que tener definido 100%
        self.name = name
        # transformo los argumentos que sean a variables de clase, para que me las devuelva el __dict__
        for k in kwargs.keys():
            setattr(self,k,kwargs[k])
    
        

class QPU():
    """
        Class to define a QPU associated to a dask worker.

        Class methods defined here:
        -----------
    """
    
    def __init__(self, id_=None, server_endpoint=None, backend=None):
        """
        Initializes th QPU class.

        Attributes:
        -----------
        id_ (int): Id assigned to the qpu, simply a int from 0 to n_qpus-1.

        worker_endpoint (str): Ip and port route given by the dask Client for the worker assigned to the given qpu.

        server_endpoint (str): Ip and port route that identifies the server that represents the qpu itself which the worker will
            communicate with.
            
        backend (<class 'Backend'>): Backend objest associated to the given QPU.

        """

        if id_ is not None:
            self.id=id_
        else:
            raise ValueError("id not correct")

        # asignamos server
        if server_endpoint is not None and type(server_endpoint) is str:
            self.server_endpoint=server_endpoint
        else:
            raise ValueError("Server not assigned")

        if backend is None:
            self.backend = Backend(name = "fakeqmio")
        else:
            self.backend = backend

def run(qpu, run_parameters=None):
        """
            Global function to run a circuit in a given QPU. It connects to the corresponding server and gives information
            about the backend if needed.
    
            Parameters:
            -----------
            run_parameters (dict): dictionary with simulation instructions to send to the QPU. If None given, a default setup
                is used.
    
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

        # mando el backend
        backend_dict = qpu.backend.__dict__
        
        
        if run_parameters is not None:
            socket.send(b'\x01', zmq.SNDMORE)
            # por ahora se van a leer así en el server, ya adaptaremos despúes a hacer un diccionario como dios manda
            print("VOY A ENVIAR LOS PARÁMETROS")
            initial_layout, shots, repetition_period = parameters
            socket.send( np.array(initial_layout).tobytes(), zmq.SNDMORE )
            socket.send( np.array(shots).tobytes(), zmq.SNDMORE )
            socket.send( np.array(repetition_period).tobytes() )        
        else:
            socket.send(b'\x00')
        
        results = socket.recv_pyobj()
        
        # TODO: hacer las cosas con zip (ahora da problemas el pickle)
        # #results = zlib.decompress(zip_results)
        #result = pickle.load(pick_results)
        return results.get_counts()


global_qpus_dict={}
import json

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

# ====================== RUN COMMANDS ===========================

# definimos un run() que haga conexión con el server correspondiente a la qpu donde estamos lanzando el circuito

def run(circ, qpu=None, parameters=None):
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
        socket.send( np.array(initial_layout).tobytes(), zmq.SNDMORE )
        socket.send( np.array(shots).tobytes(), zmq.SNDMORE )
        socket.send( np.array(repetition_period).tobytes() )
    else:
        socket.send(b'\x00')
    
    results = socket.recv_pyobj()
    
    # TODO: hacer las cosas con zip (ahora da problemas el pickle)
    # #results = zlib.decompress(zip_results)
    #result = pickle.load(pick_results)
    return results.get_counts()



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
        
        if qpu_ids is None:# no paso qpu_ids? lo ejecuto en la 0.solo 1 diccionario de run_parameters.
            qpu=global_qpus_dict[str(0)]
            futures=client.submit(run, qpu = qpu, circ = circuits, run_parameters = run_parameters, pure=False)
            
        elif type(qpu_ids) == int:# que le meto un qpu_ids? pues lo ejhecuta en esa, solo 1 diccionario de run parameters
            qpu=global_qpus_dict[str(qpu_ids)]
            futures=client.submit(run, qpu = qpu, circ = circuits, run_parameters = run_parameters, pure=False)
            
        elif type(qpus_ids) == list:# que le meto varios? pues me ejecuta ese mismo circuito en varias qpus (util para dsit de shots)
            futures = []
            if type(run_parameters) != list: # si le di varias configuraciones para el experimento que las use
                for i, id_ in enumerate(qpus_ids):
                    qpu=global_qpus_dict[str(id_)]
                    futures.append( client.submit(run, qpu = qpu, circ = circuits, run_parameters = run_parameters[i], pure=False)
            elif type(run_parameters) == dict: # si solo le di una, que la use para todos
                for i, id_ in enumerate(qpus_ids):
                    qpu=global_qpus_dict[str(id_)]
                    futures.append( client.submit(run, qpu = qpu, circ = circuits, run_parameters = run_parameters, pure=False)

    elif type(circuits) == list:# mando una lista de circuitos
                
        if qpu_ids is None:# que no digo nada de las qpus? los circutos se reparten entre todas arbitrariamente.
            if type(run_parameters) == list:# si me dieron una lista de diccionarios, cada circuito usa unos parámetros
                futures = []
                for i, c in enumerate(circuit):
                    qpu = global_qpus_dict[str( i % len(global_qpus_dict) )]# <= qpu arbitraria
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters[i], pure=False) )
            elif type(run_parameters) == dict:# si me dieron solo un diccionario, todos los experimentos con esas caracteristicas
                futures = []
                for i, c in enumerate(circuit):
                    qpu = global_qpus_dict[str( i % len(global_qpus_dict) )]# <= qpu arbitraria
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters, pure=False) )
                
        elif type(qpu_ids) == int:# que le doy un qpu id, uno solamente, entiendo que todos van a esa qpu.
            qpu=global_qpus_dict[str(qpu_ids)]
            if type(run_parameters) == list:# cada circuito con una configuración
                futures = []
                for i, c in enumerate(circuit):
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters[i], pure=False) )
            elif type(run_parameters) == dict:# todos los circuitos con la misma configuracion
                futures = []
                for i, c in enumerate(circuit):
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters, pure=False) )
                
        elif type(qpu_ids) == list:# que mando una lista de qpu_ids, cada circuito a una. TIENE QUE COINCIDIR LA LONGITUD DE LAS LISTAS!!
            if type(run_parameters) == list:# cada experimento con sus caracteristicas: circuito => qpu => parámetros
                futures = []
                for i, c in enumerate(circuit):
                    qpu = global_qpus_dict[str(qpu_ids[i])]
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters[i], pure=False) )
            elif type(run_parameters) == dict:# todo con las mismas características
                futures = []
                for i, c in enumerate(circuit):
                    qpu = global_qpus_dict[str(qpu_ids[i])]
                    futures.append( client.submit(run, qpu = qpu, circ = c, run_parameters = run_parameters, pure=False) )
    # recupero los resultados
    results = client.gather(futures)
    return results




