import numpy as np
from qiskit import QuantumCircuit, transpile
from qiskit.circuit import Parameter
from qiskit.circuit.library import QFT
from qiskit_aer import AerSimulator



def print_results(counts):
    # Extract the most frequent measurement (the best estimate of theta)
    most_frequent_output = max(counts, key=counts.get)
    
    binary_fraction = int(most_frequent_output, 2) / (2 ** len(most_frequent_output))

    # Theta is the binary fraction
    estimated_theta = binary_fraction

    print(f"Measured output: {most_frequent_output}")
    print(f"Estimated angle: {estimated_theta}")


def binary_to_float(binary_str):
    """Convert a binary string with decimal part to float"""
    int_part, dec_part = binary_str.split('.')
    integer = int(int_part, base=2) if int_part else 0
    decimal = sum(int(bit) * 2**(-i-1) for i, bit in enumerate(dec_part))
    return integer + decimal


def define_single_circuit(n_subcircuits, angle):

    circuit = QuantumCircuit(2*n_subcircuits, n_subcircuits)

    for i in range(n_subcircuits):
        start = 2*i
        
        circuit.h(start)
        circuit.x(start + 1)
        # Controlled unitary
        
        circuit.crz(angle*2*np.pi*2**(n_subcircuits-i), start, start + 1)
        
        for j in range(i - 1, -1, -1):
            circuit.rz(-np.pi*2**(-j-1), start).c_if(i - j - 1, 1)
        
        circuit.h(start)
        circuit.measure(start, i)
    
    return circuit

def run_iqpe_single_circuit(circuit, shots = 2000):
    simulator = AerSimulator()
    counts = simulator.run(circuit, shots = shots).result().get_counts()

    return counts

def define_parametric_circuit():

    Theta = Parameter("$\\theta$")
    Phase = Parameter("$\psi_s$")
    X_rot1 = Parameter("$X_1$")
    X_rot2 = Parameter("$X_2$")

    circuit = QuantumCircuit(3, 1)
    circuit.h(0)
    circuit.rx(X_rot1, 1)
    circuit.rx(X_rot2, 2)
    circuit.crz(Theta, 0, 1)
    circuit.crz(Theta, 0, 2)
    circuit.p(Phase, [0])
    circuit.h(0)
    circuit.measure([0], [0])

    return circuit

def run_iqpe_multi_circuit(parametric_circuit, backend, theta, x1, x2, bit_precision, shots = 2000, debug=False):
    measure = []

    for k in range(bit_precision):
        power = 2**(bit_precision - 1 - k)

        phase = 0
        for i in range(k):
            if measure[i]:
                phase = phase + (1 / (2**(k-i)))
        phase = -np.pi * phase

        circuit = parametric_circuit.assign_parameters({"$\\theta$": power * theta,"$\psi_s$": phase, "$X_1$": x1, "$X_2$": x2 })
        
        result = backend.run(circuit, shots=shots, repetition_period=None).result()

        counts = result.get_counts()

        zeros = counts.get("0", 0)
        ones = counts.get("1", 0)
        measure.append(0 if zeros > ones else 1) 
               
        d = 0
        for j,l in enumerate(measure):
            d = d + (l/(2**(bit_precision-j)))

        
    return d, measure


if __name__ == "__main__":
    """ n_subcircuits = 10 # How many subcircuits can I simulate?
    angle = 1/2**10
    circuit = define_single_circuit(n_subcircuits, angle)
    result = run_iqpe_single_circuit(circuit)
    print_results(result)
    print(f"Real angle: {angle}") """
    
    parametric_circuit = define_parametric_circuit()
    binary_str = "0.000100110101011100100001101111"
    phi = binary_to_float(binary_str)
    theta = 2 * np.pi * phi
    x1 = np.pi
    x2 = np.pi
    backend = AerSimulator()
    bit_precision = 30
    result, measurements = run_iqpe_multi_circuit(parametric_circuit, backend, theta, x1=x1, x2=x2, bit_precision=bit_precision, debug=True)
    print(f"Estimated angle: {result}")
    print(f"Real angle: {phi}")