"""
    Contains objects that define and manage quantum emulation jobs.
"""

from typing import  Union, Any
import json

from qiskit import QuantumCircuit
from qiskit.qasm2.exceptions import QASM2Error
from qiskit.exceptions import QiskitError

from cunqa.circuit import qc_to_json,CunqaCircuit, _registers_dict
from cunqa.logger import logger
from cunqa.backend import Backend
from cunqa.result import Result
from cunqa.qclient import QClient

class QJobError(Exception):
    """Exception for error during job submission to QPUs."""
    pass

class QJob:
    """
    Class to handle jobs sent to the simulator.
    """

    def __init__(self, qclient: QClient, backend: Backend, circuit: Union[dict, CunqaCircuit], **run_parameters: Any):
        """
        Initializes the QJob class.

        It is important to note that  if `transpilation` is set False, we asume user has already done the transpilation, otherwise some errors during the simulation
        can occur, for example if the QPU has a noise model with error associated to specific gates, if the circuit is not transpiled errors might not appear.

        If `transpile` is False and `initial_layout` is provided, it will be ignored, as well as `opt_level`.

        Possible instructions to add as `**run_parameters` can be: shots, method, parameter_binds, meas_level, ...


        Args:
            QPU (<class 'qpu.QPU'>): QPU object that represents the virtual QPU to which the job is going to be sent.

            circ (json dict or <class 'cunqa.circuit.CunqaCircuit'>): circuit to be run.

            transpile (bool): if True, transpilation will be done with respect to the backend of the given QPU. Default is set to False.

            initial_layout (list[int]):  initial position of virtual qubits on physical qubits for transpilation, lenght must be equal to the number of qubits in the circuit.

            opt_level (int): optimization level for transpilation, default set to 1.

            **run_parameters : any other simulation instructions.

        """

        self._backend = backend
        self._qclient = qclient
        self._updated = False
        self._future = None
        self._result = None

        self._convert_circuit(circuit)
        self._configure(**run_parameters)

    @property
    def result(self):
        """
        Synchronous method to obtain the result of the job. Note that this call depends on the job being finished, therefore is blocking.
        """
        try:
            if self._future is not None and self._future.valid():
                if self._result is not None:
                    if not self._updated: # if the result was already obtained, we only call the server if an update was done
                        res = self._future.get()
                        self._result = Result(json.loads(res), registers=self._cregisters)
                        self._updated = True
                    else:
                        pass
                else:
                    res = self._future.get()
                    self._result = Result(json.loads(res), registers=self._cregisters)
                    self._updated = True
            else:
                logger.debug(f"self._future is None or non-valid, None is returned.")
        except Exception as error:
                logger.error(f"Error while obtaining the result: {error}.")
                raise SystemExit # User's level

        return self._result

    @property
    def time_taken(self):
        """
        Method to obtain the time that the job took. It can also be obtained by `QJob._result.time_taken`.
        """

        if self._future is not None and self._future.valid():
            if self._result is not None:
                try:
                    return self._result.time_taken
                except AttributeError:
                    logger.warning("Time taken not available.")
                    return None
            else:
                logger.error(f"QJob not finished [{QJobError.__name__}].")
                raise SystemExit # User's level
        else:
            logger.error(f"No QJob submited [{QJobError.__name__}].")
            raise SystemExit # User's level

    def submit(self):
        """
        Asynchronous method to submit a job to the corresponding QClient.
        """
        if self._future is not None:
            logger.warning("QJob has already been submitted.")
        else:
            try:
                self._future = self._qclient.send_circuit(self._execution_config)
                logger.debug("Circuit was sent.")
            except Exception as error:
                logger.error(f"Some error occured when submitting the job [{type(error).__name__}].")
                raise QJobError # I capture the error in QPU.run() when creating the job
            
    def upgrade_parameters(self, parameters: list[float]):
        """
        Asynchronous method to upgrade the parameters in a previously submitted parametric circuit.

        Args:
            parameters (list[float]): list of parameters to assign to the parametrized circuit.
        """

        if self._result is None:
            self._future.get()

        if isinstance(parameters, list):

            if all(isinstance(param, (int, float)) for param in parameters):  # Check if all elements are real numbers
                message = """{{"params":{} }}""".format(parameters).replace("'", '"')

            else:
                logger.error(f"Parameters must be real numbers [{ValueError.__name__}].")
                raise SystemExit # User's level
        
        try:
            logger.debug(f"Sending parameters to QPU.")
            self._future = self._qclient.send_parameters(message)

        except Exception as error:
            logger.error(f"Some error occured when sending the new parameters to QPU [{type(error).__name__}].")
            raise SystemExit # User's level
        
        self._updated = False # We indicate that new results will come, in order to call server

    def _convert_circuit(self, circuit):
        try:
            if isinstance(circuit, dict):

                logger.debug("A circuit dict was provided.")

                self.num_qubits = circuit["num_qubits"]
                self.num_clbits = circuit["num_clbits"]
                self._cregisters = circuit["classical_registers"]

                logger.debug("Translation to dict not necessary...")

                circuit = circuit['instructions']
            

            elif isinstance(circuit, CunqaCircuit):

                logger.debug("A CunqaCircuit was provided.")

                self.num_qubits = circuit.num_qubits
                self.num_clbits = circuit.num_clbits
                self._cregisters = circuit.classical_regs
                
                logger.debug("Translating to dict from CunqaCircuit...")

                circuit = circuit.instructions


            elif isinstance(circuit, QuantumCircuit):

                logger.debug("A QuantumCircuit was provided.")

                self.num_qubits = circuit.num_qubits
                self.num_clbits = sum([c.size for c in circuit.cregs])
                self._cregisters = _registers_dict(circuit)[1]

                logger.debug("Translating to dict from QuantumCircuit...")

                circuit = qc_to_json(circuit)['instructions']

            elif isinstance(circuit, str):

                logger.debug("A QASM2 circuit was provided.")

                qc_from_qasm = QuantumCircuit.from_qasm_str(circuit)

                self.num_qubits = qc_from_qasm.num_qubits
                self._cregisters = _registers_dict(qc_from_qasm)[1]
                self.num_clbits = sum(len(k) for k in self._cregisters.values())

                logger.debug("Translating to dict from QASM2 string...")

                circuit = qc_to_json(qc_from_qasm)['instructions']

            else:
                logger.error(f"Circuit must be dict, <class 'cunqa.circuit.CunqaCircuit'> or QASM2 str, but {type(circuit)} was provided [{TypeError.__name__}].")
                raise QJobError # I capture the error in QPU.run() when creating the job
            
            self._circuit = circuit
            
        except KeyError as error:
            logger.error(f"Format of the cirucit dict not correct, couldn't find 'num_clbits', 'classical_registers' or 'instructions' [{type(error).__name__}].")
            raise QJobError # I capture the error in QPU.run() when creating the job

        except QASM2Error as error:
            logger.error(f"Error while translating to QASM2 [{type(error).__name__}].")
            raise QJobError # I capture the error in QPU.run() when creating the job
    
        except QiskitError as error:
            logger.error(f"Format of the circuit not correct  [{type(error).__name__}].")
            raise QJobError # I capture the error in QPU.run() when creating the job
        
        except Exception as error:
            logger.error(f"Some error occured with the circuit dict provided [{type(error).__name__}].")
            raise QJobError # I capture the error in QPU.run() when creating the job

    def _configure(self, **run_parameters):
        # configuration
        try:
            # config dict
            run_config = {
                "shots":1024, 
                "method":"statevector", 
                "num_clbits": self.num_clbits, 
                "num_qubits": self.num_qubits, 
                "seed": 188}

            if (run_parameters == None) or (len(run_parameters) == 0):
                logger.debug("No run parameters provided, default were set.")
                pass
            elif (type(run_parameters) == dict): 
                for k,v in run_parameters.items():
                    run_config[k] = v
            else:
                logger.warning("Error when reading `run_parameters`, default were set.")
            
            # instructions dict/string
            instructions = self._circuit
            self._execution_config = """ {{"config":{}, "instructions":{}, "num_qubits":{} }}""".format(run_config, instructions, self.num_qubits).replace("'", '"')

            logger.debug("QJob created.")
            logger.debug(self._execution_config)

        except KeyError as error:
            logger.error(f"Format of the cirucit not correct, couldn't find 'instructions' [{type(error).__name__}].")
            raise QJobError # I capture the error in QPU.run() when creating the job
        
        except Exception as error:
            logger.error(f"Some error occured when generating configuration for the simulation [{type(error).__name__}].")
            raise QJobError # I capture the error in QPU.run() when creating the job


def gather(qjobs) -> list[Result]:
    """
        Function to get result of several QJob objects, it also takes one QJob object.

        Args:
            qjobs (list of QJob objects or QJob object)

        Return:
            Result or list of results.
    """
    if isinstance(qjobs, list):
        if all([isinstance(q, QJob) for q in qjobs]):
            return [q.result for q in qjobs]
        else:
            logger.error(f"Objects of the list must be <class 'qjob.QJob'> [{TypeError.__name__}].")
            raise SystemExit # User's level
            
    elif isinstance(qjobs, QJob):
        return qjobs.result

    else:
        logger.error(f"qjobs must be <class 'qjob.QJob'> or list, but {type(qjobs)} was provided [{TypeError.__name__}].")
        raise SystemError # User's level
