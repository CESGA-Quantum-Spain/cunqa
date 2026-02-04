"""
Contains the :py:class:`~cunqa.qpu.QPU` class and the functions to manage the virtual QPUs (vQPUs).
For the first purpose two classes exist: :py:class:`~cunqa.qpu.QPU` and 
:py:class:`~cunqa.qpu.Backend`, where the second is dependent on the first in the sense that the 
:py:class:`~cunqa.qpu.Backend` class is used to hold the features of the vQPU. Additionally, there 
are four functions: :py:func:`~cunqa.qpu.get_QPUs`, :py:func:`~cunqa.qpu.qdrop`, 
:py:func:`~cunqa.qpu.qraise`, and :py:func:`~cunqa.qpu.run`. We can divide this group of functions 
into two subgroups:

- :py:func:`~cunqa.qpu.qdrop` and :py:func:`~cunqa.qpu.qraise`, which are adaptations of the
  ``qdrop`` and ``qraise`` bash commands to Python, respectively. This allows the use of these
  commands within the Python workflow.
- :py:func:`~cunqa.qpu.get_QPUs` and :py:func:`~cunqa.qpu.run`, which are responsible for
  creating the :py:class:`~cunqa.qpu.QPU` objects corresponding to the vQPUs and for sending
  quantum tasks to the specified vQPUs, respectively.
"""

import os
import time
import json
import subprocess
import re
from typing import Union, Any, Optional

from qiskit import QuantumCircuit

from cunqa.qclient import QClient
from cunqa.circuit import CunqaCircuit, to_ir
from cunqa.real_qpus.qmioclient import QMIOClient
from cunqa.qjob import QJob
from cunqa.logger import logger
from cunqa.constants import QPUS_FILEPATH, REMOTE_GATES
from typing import TypedDict

class Backend(TypedDict):
    """
    .. autoattribute:: basis_gates
    .. autoattribute:: coupling_map
    .. autoattribute:: custom_instructions
    .. autoattribute:: description
    .. autoattribute:: gates
    .. autoattribute:: n_qubits
    .. autoattribute:: name
    .. autoattribute:: simulator
    .. autoattribute:: version
    """
    basis_gates: list[str] #: Native gates that the Backend accepts. If others are used, they must be translated into the native gates.
    coupling_map: list[list[int]] #: Defines the physical connectivity of the qubits, that is, which pairs are connected by two-qubit gates.
    custom_instructions: str #: Any custom instructions that the Backend has defined.
    description: str #: Custom description of the Backend itself.
    gates: list[str] #: Specific gates supported.
    n_qubits: int #: Number of qubits that form the Backend, which determines the maximal number of qubits supported for a quantum circuit.
    name: str #: Name assigned to the Backend.
    simulator: str #: Name of the simulator that simulates the circuits accordingly to the Backend.
    version: str #: Version of the Backend.

class QPU:
    """
    Class that serves as representation of a vQPU for user interaction. This class contains the 
    necessary data for connecting to the vQPU in order to send quantum tasks and obtain the results. 
    This communication is performed through the :py:attr:`QPU._qclient` attribute, which represents 
    the connection between the Python and the C++ parts of CUNQA.

    The initialization of the class is normally done by the :py:func:`~cunqa.qpu.get_QPUs` 
    function, which loads the `id`, `family` and `endpoint`, and instanciates the `backend` 
    objects. It could also be manually initialized by the user, but this is not recommended.

    .. autoattribute:: id
        :annotation: : int
    .. autoattribute:: backend
        :annotation: : Backend
    .. autoattribute:: family
        :annotation: : str

    """
    _id: int 
    _backend: Backend
    _family: str
    _qclient: Union[QClient, QMIOClient]
    _device: dict

    def __init__(self, id: int, backend: Backend, device: dict, family: str, endpoint: str):
        self._id = id
        self._backend = backend
        self._device = device
        self._family = family
        
        if (device['device_name'] == 'QPU'):
            self._qclient = QMIOClient() # TODO: Generalize QPU
        else:
            self._qclient = QClient()

        self._qclient.connect(endpoint)
        logger.debug(f"Object for QPU {id} created and connected to endpoint {endpoint}.")

    @property
    def id(self) -> int:
        """ID string assigned to the object."""
        return self._id
    
    @property
    def backend(self) -> Backend:
        """
        Object that provides the characteristics that the simulator at the vQPU uses to emulate a 
        real device.
        """
        return self._backend

    @property
    def family(self) -> str:
        """Name of the family to which the corresponding vQPU belongs."""
        return self._family

    def execute(self, circuit_ir: dict, **run_parameters: Any) -> QJob:
        """
        Class method responsible of executing a circuit into the corresponding vQPU that this class 
        connects to. Possible instructions to add as `**run_parameters` are simulator dependant, 
        with `shots` and `method` being the most common ones.

        Args:
            circuit_ir (dict): circuit IR to be simulated at the vQPU.
            **run_parameters: any other simulation instructions.
        """
        qjob = QJob(self._qclient, self._device, circuit_ir, **run_parameters)
        qjob.submit()
        logger.debug(f"Qjob submitted to QPU {self._id}.")

        return qjob


def run(
        circuits: Union[list[Union[dict, QuantumCircuit, CunqaCircuit]], Union[dict, QuantumCircuit, CunqaCircuit]], 
        qpus: Union[list[QPU], QPU], 
        **run_args: Any
    ) -> Union[list[QJob], QJob]:
    """
    Function responsible of sending circuits to several vQPUs. Each circuit will be sent to each 
    QPU in order, therefore, both lists should be the same size. If they are not, but the number of 
    circuits is less than the numbers of QPUs, the circuit will get executed. In case the number of 
    QPUs is less than the number of circuits an error will be raised. Usage example:

    .. code-block:: python

        >>> circuit1 = CunqaCircuit(1, id="circuit_1")
        >>> circuit2 = CunqaCircuit(1, id="circuit_2")
        >>> circuit1.h(0)
        >>> circuit1.measure(0,0)
        >>> circuit1.send(0, "circuit_2")      # Send bit 0 to circuit_2
        >>> circuit2.recv("x", 0, "circuit_1") # Receive bit from circuit_1 and apply a conditional x
        >>>
        >>> qpus = get_QPUs()
        >>>
        >>> qjobs = run([circuit1, circuit2], qpus)
        >>> results = gather(qjobs)

    .. note::
        This method will check if two circuits are related, i.e., if one of them was part of a 
        transformation and the other is the result of a transformation. If this is true and a third
        circuit tries to communicate with them, an error will be raised. A detailed example can be 
        seen in #TODO.

    Args:
        circuits (list[dict | ~cunqa.circuit.core.CunqaCircuit | ~qiskit.QuantumCircuit] | dict | ~cunqa.circuit.core.CunqaCircuit | ~qiskit.QuantumCircuit): circuits to be run.
        qpus (list[~cunqa.qpu.QPU] | ~cunqa.qpu.QPU): QPU objects associated to the vQPUs in which 
                                                      the circuits want to be run.
        run_args: any other run arguments and parameters.
    """

    if isinstance(circuits, list):
        circuits_ir = [to_ir(circuit) for circuit in circuits]
    else:
        circuits_ir = [to_ir(circuits)]

    def expand_mapping(items: list[str]) -> dict[str, str]:
        def split(item: str) -> list[str]:
            return [p for p in re.split(r"[|+]", item) if p]

        singles = {item for item in items if len(split(item)) == 1}

        conflicts = {
            part
            for item in items
            if len(split(item)) > 1
            for part in split(item)
            if part in singles
        }

        if conflicts:
            raise ValueError(f"Conflicting identifiers found: {sorted(conflicts)}")

        return {
            part: item
            for item in items
            for part in split(item)
        }

    if not isinstance(qpus, list):
        qpus = [qpus]

    # check wether there are enough qpus and create an allocation dict that for every 
    # circuit id has the info of the QPU to which it will be sent
    if len(circuits_ir) > len(qpus):
        raise ValueError(f"There are not enough QPUs: {len(circuits)} circuits were given," 
                         f"but only {len(qpus)} QPUs [{ValueError.__name__}].")
    elif len(circuits_ir) < len(qpus):
        logger.warning("More QPUs provided than the number of circuits. "
                       "Last QPUs will remain unused.")
    
    # translate circuit ids in comm instruction to qpu endpoints
    transformed_circs = expand_mapping([c["id"] for c in circuits_ir])
    correspondence = {c["id"]: qpus[i].id for i, c in enumerate(circuits_ir)}
    for circuit in circuits_ir:
        for instr in circuit["instructions"]:
            if instr["name"] in REMOTE_GATES:
                instr["qpus"] = [correspondence[transformed_circs[circ]] for circ in instr["circuits"]]
                instr.pop("circuits")
        circuit["sending_to"] = [correspondence[target_circuit] 
                                    for target_circuit in circuit["sending_to"]]
        circuit["id"] = correspondence[circuit["id"]]

    run_parameters = {k: v for k, v in run_args.items()}
    qjobs = [qpu.execute(circuit, **run_parameters) for circuit, qpu in zip(circuits_ir, qpus)]

    if len(circuits_ir) == 1:
        return qjobs[0]
    return qjobs

def get_QPUs(co_located: bool = False, family: Optional[str] = None) -> list[QPU]:
    """
    Returns :py:class:`~cunqa.qpu.QPU` objects corresponding to the vQPUs raised by the user. It 
    will use the two args `co_located` and `family` to filter from all the QPUs available.

    Args:
        co_located (bool): if ``False``, filters by the vQPUs available at the local node.
        family (str): filters vQPUs by their family name.    
    """
    # access raised QPUs information on qpu.json file
    with open(QPUS_FILEPATH, "r") as f:
        qpus_json = json.load(f)
        if len(qpus_json) == 0:
            logger.warning(f"No QPUs were found.")
            return None

    # extract selected QPUs from qpu.json information 
    local_node = os.getenv("SLURMD_NODENAME")
    if co_located:
        targets = {
            qpu_id: info
            for qpu_id, info in qpus_json.items()
            if ((info["net"].get("nodename") == local_node or info["net"].get("mode") == "co_located") and 
                (family is None or info.get("family") == family))
        }
    else:
        if local_node is None:
            logger.warning("You are searching for QPUs in a login node, none are found.")
            return None
        else:
            targets = {
                qpu_id: info
                for qpu_id, info in qpus_json.items()
                if ((info["net"].get("nodename") == local_node) and 
                    (family is None or info.get("family") == family))
            }
    
    qpus = [
        QPU(
            id = id,
            backend = info['backend'],
            device = info['net']['device'],
            family = info['family'],
            endpoint = info['net']['endpoint']
        ) for id, info in targets.items()
    ]
        
    if len(qpus) != 0:
        logger.debug(f"{len(qpus)} QPU objects were created.")
        return qpus
    else:
        logger.warning(f"No QPUs where found with the characteristics provided: "
                       f"co_located={co_located}, family_name={family}.")
        return None


def qraise(n, t, *, 
           classical_comm = False, 
           quantum_comm = False,  
           simulator = None, 
           backend = None, 
           fakeqmio = False, 
           family = None, 
           co_located = True, 
           cores = None, 
           mem_per_qpu = None, 
           n_nodes = None, 
           node_list = None, 
           qpus_per_node= None,
           partition=None
        ) -> str:
    """
    Raises vQPUs and returns the family name associated them. This function raises 
    QPUs from the python API, which can also be done from terminal by ``qraise`` command.

    Args:
        n (int): number of vQPUs to be raised.
        t (str): maximun time that the classical resources will be reserved for the job. Format: 
                 'D-HH:MM:SS'.
        classical_comm (bool): if ``True``, vQPUs will allow classical communications.
        quantum_comm (bool): if ``True``, vQPUs will allow quantum communications.
        simulator (str): name of the desired simulator to use. Default is `Aer 
                         <https://github.com/Qiskit/qiskit-aer>`_.
        backend (str): path to a file containing the backend information.
        fakeqmio (bool): ``True`` for raising `n` vQPUs with FakeQmio backend. Only available 
                         at CESGA.
        family (str): name to identify the group of vQPUs raised.
        co_located (bool): if ``True``, `co-located` mode is set, otherwise `hpc` mode is set. In 
                           `hpc` mode, vQPUs can only be accessed from the node in which they 
                           are deployed. In `co-located` mode, they can be accessed from other 
                           nodes.
        cores (str): number of cores per vQPU, the total for the SLURM job will be 
                     `n*cores`.
        mem_per_qpu (str): memory to allocate for each vQPU in GB, format to use is "XXG".
        n_nodes (str): number of nodes for the SLURM job.
        node_list (str): list of nodes in which the vQPUs will be deployed.
        qpus_per_node (str): sets the number of vQPUs deployed on each node.
        partition (str): partition of the nodes in which the QPUs are going to be executed.
    """
    logger.debug("Setting up the requested QPUs...")
    command = f"qraise -n {n} -t {t}"

    try:
        if fakeqmio:
            command = command + " --fakeqmio"
        if classical_comm:
            command = command + " --classical_comm"
        if quantum_comm:
            command = command + " --quantum_comm"
        if simulator is not None:
            command = command + f" --simulator={str(simulator)}"
        if family is not None:
            command = command + f" --family_name={str(family)}"
        if co_located:
            command = command + " --co-located"
        if cores is not None:
            command = command + f" --cores={str(cores)}"
        if mem_per_qpu is not None:
            command = command + f" --mem-per-qpu={str(mem_per_qpu)}G"
        if n_nodes is not None:
            command = command + f" --n_nodes={str(n_nodes)}"
        if node_list is not None:
            command = command + f" --node_list={str(node_list)}"
        if qpus_per_node is not None:
            command = command + f" --qpus_per_node={str(qpus_per_node)}"
        if backend is not None:
            command = command + f" --backend={str(backend)}"
        if partition is not None:
            command = command + f" --partition={str(partition)}"

        if not os.path.exists(QPUS_FILEPATH):
           with open(QPUS_FILEPATH, "w") as file:
                file.write("{}")

        print(f"Requested QPUs with command:\n\t{command}")

        #run the command on terminal and capture its output on the variable 'output'
        output = subprocess.run(command, capture_output=True, shell=True, text=True).stdout.rstrip("\n")

        #sees the output on the console and selects the job_id
        job_id = output.split(";", 1)[0]

        cmd_getstate = ["squeue", "-h", "-j", job_id, "-o", "%T"]
        
        i = 0
        while True:
            state = subprocess.run(
                cmd_getstate, 
                capture_output = True, 
                text = True, 
                check = True
            ).stdout.strip()
            if state == "RUNNING":
                try:     
                    with open(QPUS_FILEPATH, "r") as file:
                        data = json.load(file)
                except json.JSONDecodeError:
                    continue
                count = sum(1 for key in data if key.startswith(job_id))
                if count == n:
                    break
            # We do this to prevent an overload of the Slurm deamon 
            if i == 500:
                time.sleep(2)
            else:
                i += 1

        # Wait for QPUs to be raised, so that get_QPUs can be executed inmediately
        print("QPUs ready to work \U00002705")

        return family if family is not None else str(job_id)
    
    except subprocess.CalledProcessError as error:
        raise RuntimeError(f"An error was encoutered while qraising:\n {error.stderr}.")

def qdrop(*families: str):
    """
    Same functionality as the `qdrop` bash command, with the peculiarity that it only takes as 
    argument the vQPU family names (and it does not accept the job ID as the bash command). This is 
    done because the Python version of the `qraise` bash command returns only the family name. 

    If no families are provided, all vQPUs deployed by the user will be dropped.

    Args:
        families (str): family names of the groups of vQPUs to be dropped.
    """
    
    # Building the terminal command to drop the specified families
    cmd = ['qdrop'] 

    # If no QPU is provided we drop all QPU slurm jobs
    if len( families ) == 0:
        cmd.append('--all') 
    else:
        cmd.append('--fam')
        for family in families:
            cmd.append(family)
 
    subprocess.run(cmd) #run 'qdrop slurm_jobid_1 slurm_jobid_2 etc' on terminal
