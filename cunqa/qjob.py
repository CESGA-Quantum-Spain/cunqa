import json
from qiskit import QuantumCircuit
from qiskit.qasm2 import dumps
from qiskit.qasm2.exceptions import QASM2Error
from qiskit.exceptions import QiskitError
from cunqa.circuit import qc_to_json, from_json_to_qc, _registers_dict, _is_parametric, CunqaCircuit
from cunqa.transpile import transpiler, TranspilerError
from cunqa.logger import logger


class QJobError(Exception):
    """Exception for error during job submission to QPUs."""
    pass

def _divide(string, lengths):

    """
    Divides a string of bits in groups of given lenghts separated by spaces.

    Args:
    --------
    string (str): string that we want to divide.

    lengths (list[int]): lenghts of the resulting strings in which the original one is divided.

    Return:
    --------
    A new string in which the resulting groups are separated by spaces.

    """

    parts = []
    init = 0
    try:
        if len(lengths) == 0:
            return string
        else:
            for length in lengths:
                parts.append(string[init:init + length])
                init += length
            return ' '.join(parts)
    
    except Exception as error:
        logger.error(f"Something failed with division of string [{error.__name__}].")
        raise SystemExit # User's level


def _convert_counts(counts, registers):

    """
    Funtion to convert counts wirtten in hexadecimal format to binary strings and that applies the division of the bit strings.

    Args:
    --------
    counts (dict): dictionary of counts to apply the conversion.

    registers (dict): dictionary of classical registers.

    Return:
    --------
    Counts dictionary with keys as binary string correctly separated with spaces accordingly to the classical registers.
    """

    if isinstance(registers, dict):
        
        # counting number of classical bits
        num_clbits = sum([len(i) for i in registers.values()])
        # getting lenghts of bits for the different registers
        lengths = []
        for v in registers.values():
            lengths.append(len(v))
    else:
        logger.error(f"Error when converting `counts` strings.")
        raise QJobError # I capture this error in QJob.result()

    new_counts = {}
    if isinstance(counts, dict):
        for k,v in counts.items():
            if k.startswith('0x'): # converting to binary string and dividing in bit strings
                new_counts[_divide(format( int(k, 16), '0'+str(num_clbits)+'b' ), lengths)]= v
            else: # just dividing the bit stings
                new_counts[_divide(k, lengths)] = v
    elif isinstance(counts, list):
        for count in counts:
            new_counts[_divide(format(count[0],'0'+str(num_clbits)+'b')[::-1], lengths)] = count[1] 
    return new_counts


class Result:
    """
    Class to describe the result of an experiment.
    """
    def __init__(self, result, registers = None):
        """
        Initializes the Result class.

        Args:
        -----------
        result (dict): dictionary given as the result of the simulation.

        registers (dict): in case the circuit has more than one classical register, dictionary for the lengths of the classical registers must be provided.
        """
        logger.debug(f"Result received: {result}\n")

        if type(result) == dict:
            self.result = result
        elif result is None:
            logger.error(f"Something failed at server, result is {None} [{ValueError.__name__}].")
            raise ValueError
        else:
            logger.error(f"result must be dict, but {type(result)} was provided [{TypeError.__name__}].")
            raise TypeError # I capture this error in QJob.result() when creating the object.
        
        # processing result
        if len(self.result) == 0:
            logger.error(f" [{ValueError.__name__}].")
            raise ValueError # I capture this error in QJob.result() when creating the object.
        
        elif "ERROR" in self.result:
            message = self.result["ERROR"]
            logger.error(f"Error during simulation, please check availability of QPUs, run arguments sintax and circuit sintax: {message}")
            raise QJobError

        else:
            try:

                if "results" in list(self.result.keys()): # aer
                    self.counts = result["results"][0]["data"]["counts"]
                    self.time = self.result["results"][0]["time_taken"]

                elif "counts" in list(self.result.keys()): # munich and cunqa
                    self.counts = self.result["counts"]
                    self.time = self.result["time_taken"]

                self.counts = _convert_counts(self.counts, registers)


            except KeyError:
                logger.error(f"Some error occured with results file, no `counts` found. Check avaliability of the QPUs [{KeyError.__name__}].")
                raise KeyError # I capture this error in QJob.result() when creating the object.
            
            except Exception as error:
                logger.error(f"Some error occured with counts [{type(error).__name__}]: {error}.")
                raise error

        logger.debug("Results correctly loaded.")

    def get_dict(self):
        """
        Class method to obtain a dictionary with the class variables.

        Return:
        -----------
        Dictionary with the class variables, actually the result input but with some format changes.

        """
        return self.result

    def get_counts(self):
        """
        Class method to obtain the information for the counts result for the given experiment.

        Return:
        -----------
        Dictionary in which the keys are the bit strings and the values number of times that they were measured.

        """
        return self.counts
    
    def time_taken(self):
        """
        Class method to obtain the information for the time_taken for the given experiment.

        Return:
        -----------
        Real number representing the execution time

        """
        return self.time



class QJob:
    """
    Class to handle jobs sent to the simulator.
    """

    def __init__(self, qpu, circ, transpile, initial_layout = None, opt_level = 1, **run_parameters):
        """
        Initializes the QJob class.

        It is important to note that  if `transpilation` is set False, we asume user has already done the transpilation, otherwise some errors during the simulation
        can occur, for example if the QPU has a noise model with error associated to specific gates, if the circuit is not transpiled errors might not appear.

        If `transpile` is False and `initial_layout` is provided, it will be ignored, as well as `opt_level`.

        Possible instructions to add as `**run_parameters` can be: shots, method, parameter_binds, meas_level, ...


        Args:
        -----------
        QPU (<class 'qpu.QPU'>): QPU object that represents the virtual QPU to which the job is going to be sent.

        circ (json dict or <class 'cunqa.circuit.CunqaCircuit'>): circuit to be run.

        transpile (bool): if True, transpilation will be done with respect to the backend of the given QPU. Default is set to False.

        initial_layout (list[int]):  initial position of virtual qubits on physical qubits for transpilation, lenght must be equal to the number of qubits in the circuit.

        opt_level (int): optimization level for transpilation, default set to 1.

        **run_parameters : any other simulation instructions.

        """

        self._QPU = qpu
        self._future = None
        self._result = None
        self._updated = False

        if isinstance(transpile, bool):
            pass

        else:
            logger.error(f"transpile must be boolean, but a {type(transpile)} was provided [{TypeError.__name__}].")
            raise TypeError # I capture the error in QPU.run() when creating the job

        # transpilation
        if transpile:
            try:
                circt = transpiler( circ, self._QPU.backend, initial_layout = initial_layout, opt_level = opt_level )
                logger.debug("Transpilation done.")
            except Exception as error:
                logger.error(f"Transpilation failed [{type(error).__name__}].")
                raise TranspilerError # I capture the error in QPU.run() when creating the job
            
        else:
            if initial_layout is not None:
                logger.warning("Transpilation was not done, initial_layout provided was ignored. If you want to map the circuit to the given qubits of the backend you must set transpile=True and provide the list of qubits which lenght has to be equal to the number of qubits of the circuit.")
            circt = circ
            logger.warning("No transpilation was done, errors might occur if any gate or instruction is not supported by the simulator.")


        # conversion to the needed format
        try:
            if isinstance(circt, dict):

                logger.debug("A circuit dict was provided.")

                if (circt["is_distributed"] and self._QPU.backend.simulator != "CunqaSimulator"):
                    logger.error(f"Currently only Cunqasimulator supports communications [NotImplementedError]")
                    raise QJobError #captured and passed to QPUs

                self.num_qubits = circt["num_qubits"]
                cl_bits = circt["num_clbits"]
                self._cregisters = circt["classical_registers"]

                if self._QPU.backend.simulator == "AerSimulator":

                    logger.debug("Translating to dict for AerSimulator...")

                    circuit = circt['instructions']

                    exec_type = circt['exec_type']

                elif self._QPU.backend.simulator == "MunichSimulator":

                    logger.debug("Translating to QASM2 for MunichSimulator...")

                    circuit = dumps(from_json_to_qc(circt)).translate(str.maketrans({"\"":  r"\"", "\n":r"\n"}))

                    exec_type = circt['exec_type']

                elif self._QPU.backend.simulator == "CunqaSimulator":

                    logger.debug("Translating to dict for CunqaSimulator...")

                    circuit = circt['instructions']

                    exec_type = circt['exec_type']
            

            elif isinstance(circt, CunqaCircuit):

                logger.debug("A CunqaCircuit was provided.")

                if (circt.is_distributed and self._QPU.backend.simulator != "CunqaSimulator"):
                    logger.error(f"Currently only Cunqasimulator supports communications [NotImplementedError]")
                    raise QJobError #captured and passed to QPUs

                cl_bits = circt.num_clbits
                self._cregisters = circt.classical_regs
                
                if self._QPU.backend.simulator == "AerSimulator":

                    logger.debug("Translating to dict for AerSimulator...")

                    circuit = circt.instructions

                elif self._QPU.backend.simulator == "MunichSimulator":

                    logger.debug("Translating to QASM2 for MunichSimulator...")

                    circuit = dumps(circt.instructions)
                
                elif self._QPU.backend.simulator == "CunqaSimulator":

                    logger.debug("Extracting CunqaCircuit instructions for CunqaSimulator...")

                    circuit = circt.instructions

                    if circt.is_distributed:
                        exec_type = "dynamic"
                    else:
                        exec_type = "offloading"


            elif isinstance(circt, QuantumCircuit):

                logger.debug("A QuantumCircuit was provided.")

                cl_bits = sum([c.size for c in circt.cregs])
                self._cregisters = _registers_dict(circt)[1]

                if self._QPU.backend.simulator == "AerSimulator":

                    logger.debug("Translating to dict for AerSimulator...")

                    circuit = qc_to_json(circt)['instructions']

                elif self._QPU.backend.simulator == "MunichSimulator":

                    logger.debug("Translating to QASM2 for MunichSimulator...")

                    circuit = dumps(circt)

                elif self._QPU.backend.simulator == "CunqaSimulator":

                    logger.debug("Translating to dict for CunqaSimulator...")

                    circuit = qc_to_json(circt)['instructions']


            elif isinstance(circt, str):

                logger.debug("A QASM2 circuit was provided.")

                self._cregisters = _registers_dict(QuantumCircuit.from_qasm_str(circt))[1]
                cl_bits = sum(len(k) for k in self._cregisters.values())

                if self._QPU.backend.simulator == "AerSimulator":

                    logger.debug("Translating to dict for AerSimulator...")

                    circuit = qc_to_json(QuantumCircuit.from_qasm_str(circt))['instructions']

                elif self._QPU.backend.simulator == "MunichSimulator":

                    logger.debug("Translating to QASM2 for MunichSimulator...")

                    circuit = circt

                elif self._QPU.backend.simulator == "CunqaSimulator":

                    logger.debug("Translating to dict for CunqaSimulator...")

                    circuit = qc_to_json(QuantumCircuit.from_qasm_str(circt))['instructions']

            else:
                logger.error(f"Circuit must be dict, <class 'cunqa.circuit.CunqaCircuit'> or QASM2 str, but {type(circ)} was provided [{TypeError.__name__}].")
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
    

        # configuration
        try:
            # config dict
            run_config = {"shots":1024, "method":"statevector", "memory_slots":cl_bits, "seed": 188}

            if (run_parameters == None) or (len(run_parameters) == 0):
                logger.debug("No run parameters provided, default were set.")
                pass
            elif (type(run_parameters) == dict): 
                for k,v in run_parameters.items():
                    run_config[k] = v
            else:
                logger.warning("Error when reading `run_parameters`, default were set.")
            
            # instructions dict/string
            instructions = circuit


            if self._QPU.backend.simulator == "AerSimulator":
                self._execution_config = """ {{"config":{}, "instructions":{}, "exec_type":"{}" }}""".format(run_config, instructions, exec_type).replace("'", '"')

            elif self._QPU.backend.simulator == "MunichSimulator":
                self._execution_config = """ {{"config":{}, "instructions":"{}", "exec_type":"{}" }}""".format(run_config, instructions, exec_type).replace("'", '"')
            
            elif self._QPU.backend.simulator == "CunqaSimulator":
                self._execution_config = """ {{"config":{}, "instructions":{}, "exec_type":"{}", "num_qubits":{} }}""".format(run_config, instructions, exec_type, self.num_qubits).replace("'", '"')

            logger.debug("QJob created.")
            logger.debug(self._execution_config)

        
        except KeyError as error:
            logger.error(f"Format of the cirucit not correct, couldn't find 'instructions' [{type(error).__name__}].")
            raise QJobError # I capture the error in QPU.run() when creating the job
        
        except Exception as error:
            logger.error(f"Some error occured when generating configuration for the simulation [{type(error).__name__}].")
            raise QJobError # I capture the error in QPU.run() when creating the job
        


    def submit(self):
        """
        Asynchronous method to submit a job to the corresponding QClient.
        """
        if self._future is not None:
            logger.warning("QJob has already been submitted.")
        else:
            try:
                self._future = self._QPU._qclient.send_circuit(self._execution_config)
                logger.debug("Circuit was sent.")
            except Exception as error:
                logger.error(f"Some error occured when submitting the job [{type(error).__name__}].")
                raise QJobError # I capture the error in QPU.run() when creating the job
            
    def upgrade_parameters(self, parameters):
        """
        Asynchronous method to upgrade the parameters in a previously submitted parametric circuit.

         Args:
        -----------
        parameters (list[float or int]): list of parameters to assign to the parametrized circuit.
        """

        if self._result is None:
            self._future.get()

        if not _is_parametric(self._circuit):
            logger.error(f"Cannot upgrade parameters. Please check that the circuit provided has gates that accept parameters [{QJobError.__name__}].")
            raise SystemExit # User's level

        if isinstance(parameters, list):

            if all(isinstance(param, (int, float)) for param in parameters):  # Check if all elements are real numbers
                message = """{{"params":{} }}""".format(parameters).replace("'", '"')

            else:
                logger.error(f"Parameters must be real numbers [{ValueError.__name__}].")
                raise SystemExit # User's level
        
        try:
            logger.debug(f"Sending parameters to QPU {self._QPU.id}.")
            self._future = self._QPU._qclient.send_parameters(message)

        except Exception as error:
            logger.error(f"Some error occured when sending the new parameters to QPU {self._QPU.id} [{type(error).__name__}].")
            raise SystemExit # User's level
        
        self._updated = False # We indicate that new results will come, in order to call server

        return self

    def result(self):
        """
        Synchronous method to obtain the result of the job. Note that this call depends on the job being finished, therefore is blocking.
        """
        if (self._future is not None) and (self._future.valid()):
            try:
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
            except Exception as error:
                logger.error(f"Error while creating Results object [{type(error).__name__}].")
                raise SystemExit # User's level
        else:
            logger.debug(f"self._future is None or non-valid, None is returned.")

        return self._result


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
    if isinstance(qjobs, list):
        if all([isinstance(q, QJob) for q in qjobs]):
            return [q.result() for q in qjobs]
        else:
            logger.error(f"Objects of the list must be <class 'qjob.QJob'> [{TypeError.__name__}].")
            raise SystemExit # User's level
            
    elif isinstance(qjobs, QJob):
        return qjobs.result()

    else:
        logger.error(f"qjobs must be <class 'qjob.QJob'> or list, but {type(qjobs)} was provided [{TypeError.__name__}].")
        raise SystemError # User's level