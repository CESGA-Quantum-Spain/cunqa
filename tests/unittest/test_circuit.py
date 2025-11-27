import unittest
from cunqa.circuit import qc_to_json, from_json_to_qc, _registers_dict
from qiskit import QuantumCircuit, QuantumRegister , ClassicalRegister
import numpy as np

HOME = os.getenv("HOME")
sys.path.insert(0, HOME)

# Define the basis_gates list with dictionaries containing Qiskit gate info and gate name
basis_gates = [
    {"name": "u", "gate": QuantumCircuit.u, "qubits": 1, "param": [np.pi, 0.0, 0.0]}, 
    {"name": "u", "gate": QuantumCircuit.u, "qubits": 1, "param": [np.pi/2, np.pi, 0.0]}, 
    {"name": "u", "gate": QuantumCircuit.u, "qubits": 1, "param": [np.pi, np.pi/2, np.pi]}, 
    #{"name": "unitary", "gate": QuantumCircuit.unitary, "qubits": 1, "param": [[[1, 0], [0, 1]]]}, 
    {"name": "p", "gate": QuantumCircuit.p, "qubits": 1, "param": [np.pi]}, 
    {"name": "r", "gate": QuantumCircuit.r, "qubits": 1, "param": [np.pi, 1.0]}, 
    {"name": "rx", "gate": QuantumCircuit.rx, "qubits": 1, "param": [np.pi]}, 
    {"name": "ry", "gate": QuantumCircuit.ry, "qubits": 1, "param": [np.pi]}, 
    {"name": "rz", "gate": QuantumCircuit.rz, "qubits": 1, "param": [np.pi]},
    {"name": "id", "gate": QuantumCircuit.id, "qubits": 1, "param": None}, 
    {"name": "x", "gate": QuantumCircuit.x, "qubits": 1, "param": None}, 
    {"name": "y", "gate": QuantumCircuit.y, "qubits": 1, "param": None},
    {"name": "z", "gate": QuantumCircuit.z, "qubits": 1, "param": None}, 
    {"name": "h", "gate": QuantumCircuit.h, "qubits": 1, "param": None}, 
    {"name": "s", "gate": QuantumCircuit.s, "qubits": 1, "param": None},
    {"name": "sdg", "gate": QuantumCircuit.sdg, "qubits": 1, "param": None}, 
    {"name": "sx", "gate": QuantumCircuit.sx, "qubits": 1, "param": None}, 
    {"name": "sxdg", "gate": QuantumCircuit.sxdg, "qubits": 1, "param": None},
    {"name": "t", "gate": QuantumCircuit.t, "qubits": 1, "param": None}, 
    {"name": "tdg", "gate": QuantumCircuit.tdg, "qubits": 1, "param": None}, 
    {"name": "swap", "gate": QuantumCircuit.swap, "qubits": 2, "param": None},
    {"name": "cx", "gate": QuantumCircuit.cx, "qubits": 2, "param": None}, 
    {"name": "cy", "gate": QuantumCircuit.cy, "qubits": 2, "param": None}, 
    {"name": "cz", "gate": QuantumCircuit.cz, "qubits": 2, "param": None},
    {"name": "csx", "gate": QuantumCircuit.csx, "qubits": 2, "param": None}, 
    {"name": "cp", "gate": QuantumCircuit.cp, "qubits": 2, "param": [np.pi]}, 
    {"name": "cu", "gate": QuantumCircuit.cu, "qubits": 2, "param": [np.pi, np.pi/2, np.pi/4, np.pi]},
    {"name": "cp", "gate": QuantumCircuit.cp, "qubits": 2, "param": [np.pi/4]},   #cu1
    {"name": "cu", "gate": QuantumCircuit.cu, "qubits": 2, "param": [np.pi, np.pi/2, np.pi,  0.0]},  #cu3
    {"name": "rxx", "gate": QuantumCircuit.rxx, "qubits": 2, "param": [np.pi]},
    {"name": "ryy", "gate": QuantumCircuit.ryy, "qubits": 2, "param": [np.pi]}, 
    {"name": "rzz", "gate": QuantumCircuit.rzz, "qubits": 2, "param": [np.pi]}, 
    {"name": "rzx", "gate": QuantumCircuit.rzx, "qubits": 2, "param": [np.pi]},
    {"name": "ccx", "gate": QuantumCircuit.ccx, "qubits": 3, "param": None}, 
    {"name": "ccz", "gate": QuantumCircuit.ccz, "qubits": 3, "param": None}, 
    {"name": "crx", "gate": QuantumCircuit.crx, "qubits": 2, "param": [np.pi]},
    {"name": "cry", "gate": QuantumCircuit.cry, "qubits": 2, "param": [np.pi]}, 
    {"name": "crz", "gate": QuantumCircuit.crz, "qubits": 2, "param": [np.pi]}, 
    {"name": "cswap", "gate": QuantumCircuit.cswap, "qubits": 3, "param": None}
]
#n = len(basis_gates)
SUPPORTED_GATES_PARAMETRIC = ["u1", "p", "rx", "ry", "rz", "rxx", "ryy", "rzz", "rzx","cp", "crx", "cry", "crz", "cu1","c_if_rx","c_if_ry","c_if_rz", "d_c_if_rx","d_c_if_ry","d_c_if_rz", "u2", "r", "u", "u3", "cu3", "cu"]


class TestCircuitConversion(unittest.TestCase):
    """
    Class to test cunqa.circuit's methods that change the format of circuits or retrieve a dictionary with quantum and classical regiter information from a circuit.
    """

    def __init__(self, methodName = "runTest"):
        self.qcs = []
        self.json_circuits= []
        super().__init__(methodName)

    def test_qc_to_json(self):
        #self.json_circuits= []  in this list we will store json dictionaries describing a minimal circuit containing only one gate for all basis gates. We will generate these dictionaries ourselves
        conversions = []        #in this list we store the result of transforming QuantumCircuits describing the mentioned minimal circuits to json dictionaries with our function
        
        #we will compare the two lists to test that the qc_to_json function works correctly

        for gate_info in basis_gates: 
            gate = gate_info["gate"]
            num_qubits = gate_info["qubits"]
            params = gate_info["param"]  

            if num_qubits == 1:
                qc = QuantumCircuit(1)
                if params is not None:
                    gate(qc, *params, 0)  # Apply to qubit 0 with parameter
                else:
                    gate(qc, 0)  # Apply to qubit 0 without parameter   
                qc.measure_all()

            elif num_qubits == 2:
                qc= QuantumCircuit(2)
                if params is not None:
                    gate(qc, *params, 0, 1)  # Apply to qubit 0 with parameters
                else:
                    gate(qc, 0, 1)  # Apply to qubit 0 without parameters   
                qc.measure_all()

            elif num_qubits == 3:
                qc = QuantumCircuit(3)
                gate(qc, 0, 1, 2)     #No 3-qubit gate has parameters :-)
                qc.measure_all()

            self.qcs.append(qc) #We store the minimal circuits to reuse them in the next test
            json_transf = qc_to_json(qc)
            conversions.append(json_transf)  

            #let's create the corresponding dictionary circuits
            _params = params if params is not None else []
            
            json1 = {
                "id":"",
                "is_parametric": True if gate_info["name"] in SUPPORTED_GATES_PARAMETRIC else False,
                "instructions":[{"name": gate_info["name"], "qubits": list(range(num_qubits)),"params":_params }, {'memory': [0], 'name': 'measure', 'qubits': [0]}],
                "num_qubits": num_qubits,
                "num_clbits": num_qubits,
                "quantum_registers": {"q" : list(range(num_qubits)) },
                "classical_registers": {"meas": list(range(num_qubits))}
            }
            if num_qubits > 1:
                json1["instructions"].append({'memory': [1], 'name': 'measure', 'qubits': [1]})
            if num_qubits == 3:
                json1["instructions"].append({'memory': [2], 'name': 'measure', 'qubits': [2]})
            self.json_circuits.append(json1)
            self.MaxDiff=None
        return self.assertListEqual(conversions, self.json_circuits) #Check that all elements on the list of converted circuits coincide with the elements of the list with the same circuits 'handwritten' as jsons
    





    def test_json_to_qc(self):
        #comparing two instances of a class it's hard - two instances are not equal despite having the same attributes. If their dict has classes in it, the same problem will occur if one tries to compare classes' dicts.
        # This is the case for QuantumCircuit, thus I chose to test from_json_to_qc and then test json_to_qc by passing to json again. If the first function is wrong both tests should fail, if the second is wrong only this one should fail 

        double_converted = map(lambda x: qc_to_json(from_json_to_qc(x)), self.json_circuits) #convert all handcrafted jsons to qcs and back again to json

        once_converted= map(qc_to_json, self.qcs) #convert all handcafted quantum circuits to json
        
        return self.assertListEqual(list(double_converted), list(once_converted)) 
    




    
    def test_registers_dict(self):
        qc_0 = QuantumCircuit(QuantumRegister(4), ClassicalRegister(3))
        return self.assertListEqual(_registers_dict(qc_0) ,[{"q0":[0,1,2,3]},{"c0":[0,1,2]}])



if __name__ == "__main__":
    unittest.main(verbosity=2)