from cunqa.logger import logger
from qiskit import QuantumCircuit, Instruction


class CunqaCircuit(QuantumCircuit):

    def __init__(self, circuit_id = None, *args):

        # assign circuit id
        self._id = circuit_id

        # instanciation of cunqa_info dict to store instructions
        self.cunqa_info = {"instructions":[]}

        # class attribute to track weather a circuit supports communications
        self.is_distributed = False

        # initialization of the QuantumCircuit class
        super.__init__(self, *args)


    # TODO: modify more functions from QuantumCircuit that add instructions **here** =======================================

    def _append_standard_gate(self, op, *args):
        
        self.cunqa_info["instructions"].append(
            {
                "name":op.name,
                "qubits":[],
                "clbits":[],
                "params":[],
                "qpus":[]
            }
        )
        return super._append_standard_gate(self,op,*args)

    # ========================================================================================================================

    # send_gate function **here** ============================================================================================
    def send_gate(self, gate, control_qubit, target_circuit, target_qubit):
        """
        self (QuantumCircuit): instruction will be added to the QuantumCircuit provided.

        gate(str): reference to the gate we want to send.

        control(int): control qubit from self.

        target(tuple(circuit_id, qubit)): circuit to which we will send the instruction and target qubit where the gate will be conditionally applied.
        """
        instruction = Instruction(name = "d_c_if_"+gate, num_qubits = 1, num_clbits = 0)

        # with this sintax only circuits with one sigle quantum register are supported
        # TODO: adapt to several qregs, checkout how qiskit does it
        control_qubit = self.qregs[0][control_qubit]

        self.append(CircuitInstruction(instruction, control_qubit), target = "hola")

        self.cunqa_info["instructions"].append(
            {
                "name":gate,
                "qubits":[],
                "clbuts":[],
                "params":[],
                "qpus":[]
            }
        )
        return self._append_standard_gate()
    
    # ========================================================================================================================
    

    # rcv_gate function **here** ============================================================================================
    def rcv_gate(self, jdhksaj)
        


    # ========================================================================================================================






def _qasm2_to_json(qasm_str, version = None):
    """
    Transforms a QASM circuit to json.

    Args:
    ---------
    qc (str): circuit to transform to json.

    Return:
    ---------
    json dict with the circuit information.
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
            if line:
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
                        print(parts_split)
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

def qc_to_json(qc):
    """
    Transforms a QuantumCircuit to json dict.

    Args:
    ---------
    qc (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>): circuit to transform to json.

    Return:
    ---------
    Json dict with the circuit information.
    """
    if isinstance(qc, dict):
        logger.warning(f"Circuit provided is already a dict.")
        return qc
    elif isinstance(qc,QuantumCircuit):
        pass
    else:
        logger.error(f"Circuit must be <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'> or dict, but {type(qc)} was provided [{TypeError.__name__}].")
        raise TypeError # this error should not be raised bacause in QPU we already check type of the circuit

    try:
        
        quantum_registers, classical_registers = registers_dict(qc)
        
        json_data = {
            "instructions":[],
            "num_qubits":sum([q.size for q in qc.qregs]),
            "num_clbits": sum([c.size for c in qc.cregs]),
            "quantum_registers":quantum_registers,
            "classical_registers":classical_registers
        }
        for i in range(len(qc.data)):
            if qc.data[i].name == "barrier":
                pass
            elif qc.data[i].name != "measure":

                qreg = [r._register.name for r in qc.data[i].qubits]
                qubit = [q._index for q in qc.data[i].qubits]

                json_data["instructions"].append({"name":qc.data[i].name, 
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg,qubit)],
                                                "params":qc.data[i].params
                                                })
            else:
                qreg = [r._register.name for r in qc.data[i].qubits]
                qubit = [q._index for q in qc.data[i].qubits]
                
                creg = [r._register.name for r in qc.data[i].clbits]
                bit = [b._index for b in qc.data[i].clbits]

                json_data["instructions"].append({"name":qc.data[i].name,
                                                "qubits":[quantum_registers[k][q] for k,q in zip(qreg,qubit)],
                                                "memory":[classical_registers[k][b] for k,b in zip(creg,bit)]
                                                })
                    

        return json_data
    
    except Exception as error:
        logger.error(f"Some error occured during transformation from QuantumCircuit to json dict [{type(error).__name__}].")
        raise error


from qiskit import QuantumCircuit
from qiskit.circuit import QuantumRegister, ClassicalRegister, CircuitInstruction, Instruction, Qubit, Clbit


def from_json_to_qc(circuit_dict):
    """
    Function to transform a circuit in json dict format to <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>.

    Args:
    ----------
    circuit_dict (dict): circuit to be transformed to QuantumCircuit.

    Return:
    -----------
    QuantumCircuit with the given instructions.

    """

    if isinstance(circuit_dict, QuantumCircuit):
        logger.warning("Circuit provided is already <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>.")
        return circuit_dict

    elif isinstance(circuit_dict, dict):
        circuit = circuit_dict
    else:
        logger.error(f"circuit_dict must be dict, but {type(circuit_dict)} was provided [{TypeError.__name__}]")
        raise TypeError

    try:
        instructions = circuit['instructions']
        num_qubits = circuit['num_qubits']
        classical_registers = circuit['classical_registers']

    except KeyError as error:
        logger.error(f"Circuit json not correct, requiered keys must be: 'instructions', 'num_qubits', 'num_clbits', 'quantum_resgisters' and 'classical_registers' [{type(error).__name__}].")
        raise error
        
        
    try:
    
        qc = QuantumCircuit(num_qubits)

        bits = []
        for cr, lista in classical_registers.items():
            for i in lista: 
                bits.append(i)
            qc.add_register(ClassicalRegister(len(lista), cr))


        for instruction in instructions:
            if instruction['name'] != 'measure':
                inst = CircuitInstruction( 
                    operation = Instruction(name = instruction['name'],
                                            num_qubits = len(instruction['qubits']),
                                            num_clbits = 0,
                                            params = instruction['params']
                                            ),
                    qubits = (Qubit(QuantumRegister(num_qubits, 'q'), q) for q in instruction['qubits']),
                    clbits = ()
                    )
                qc.append(inst)
            elif instruction['name'] == 'measure':
                bit = instruction['memory'][0]
                if bit in bits: # checking that the bit referenced in the instruction it actually belongs to a register
                    for k,v in classical_registers.items():
                        if bit in v:
                            reg = k
                            l = len(v)
                            clbit = v.index(bit)
                            inst = CircuitInstruction(
                                operation = Instruction(name = instruction['name'],
                                                        num_qubits = 1,
                                                        num_clbits = 1,
                                                        params = []
                                                        ),
                                qubits = (Qubit(QuantumRegister(num_qubits, 'q'), q) for q in instruction['qubits']),
                                clbits = (Clbit(ClassicalRegister(l, reg), clbit),)
                                )
                else:
                    logger.error(f"Bit {bit} not found in {bits}, please check the format of the circuit json.")
                    raise IndexError
                qc.append(inst)
                
        return qc
    
    except KeyError as error:
        logger.error(f"Some error with the keys of `instructions` occured, please check the format [{type(error).__name__}].")
        raise error
    
    except TypeError as error:
        logger.error(f"Error when reading instructions, check that the given elements have the correct type [{type(error).__name__}].")
        raise TypeError
    
    except IndexError as error:
        logger.error(f"Error with format for classical_registers [{type(error).__name__}].")
        raise error

    except Exception as error:
        logger.error(f"Error when converting json dict to QuantumCircuit [{type(error).__name__}].")
        raise error




def _registers_dict(qc):
    """
    Extracts the number of classical and quantum registers from a QuantumCircuit.

    Args
    -------
     qc (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>): quantum circuit whose number of registers we want to know

    Return:
    --------
    Two element list with quantum and classical registers, in that order.
    """

    quantum_registers = {}
    for qr in qc.qregs:
        quantum_registers[qr.name] = qr.size

    countsq = []

    valuesq = list(quantum_registers.values())

    for i, v in enumerate(valuesq):
        if i == 0:
            countsq.append(list(range(0, v)))
        else:
            countsq.append(list(range(sum(valuesq[:i]), sum(valuesq[:i])+v)))

    for i,k in enumerate(quantum_registers.keys()):
        quantum_registers[k] = countsq[i]

    classical_registers = {}
    for cr in qc.cregs:
        classical_registers[cr.name] = cr.size

    counts = []

    values = list(classical_registers.values())

    for i, v in enumerate(values):
        if i == 0:
            counts.append(list(range(0, v)))
        else:
            counts.append(list(range(sum(values[:i]), sum(values[:i])+v)))

    for i,k in enumerate(classical_registers.keys()):
        classical_registers[k] = counts[i]

    return [quantum_registers, classical_registers]

def _is_parametric(circuit):
    """
    Function to determine weather a cirucit has gates that accept parameters, not necesarily parametric <class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>.
    For example, a circuit that is composed by hadamard and cnot gates is not a parametric circuit; but if a circuit has any of the gates defined in `parametric_gates` we
    consider it a parametric circuit for our purposes.

    Args:
    -------
    circuit (<class 'qiskit.circuit.quantumcircuit.QuantumCircuit'>, dict or str): the circuit from which we want to find out if it's parametric.

    Return:
    -------
    True if the circuit is considered parametric, False if it's not.
    """
    parametric_gates = ["u", "u1", "u2", "u3", "rx", "ry", "rz", "crx", "cry", "crz", "cu1", "cu3", "rxx", "ryy", "rzz", "rzx", "cp", "cswap", "ccx", "crz", "cu"]
    if isinstance(circuit, QuantumCircuit):
        for instruction in circuit.data:
            if instruction.operation.name in parametric_gates:
                return True
        return False
    elif isinstance(circuit, dict):
        for instruction in circuit['instructions']:
            if instruction['name'] in parametric_gates:
                return True
        return False
    elif isinstance(circuit, str):
        lines = circuit.splitlines()
        for line in lines:
            line = line.strip()
            if any(line.startswith(gate) for gate in parametric_gates):
                return True
        return False
           