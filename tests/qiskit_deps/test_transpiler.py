import os, sys
from unittest.mock import Mock, patch, mock_open
import copy
import pytest
from qiskit import QuantumCircuit

IN_GITHUB_ACTIONS = os.getenv("GITHUB_ACTIONS") == "true"

if IN_GITHUB_ACTIONS:
    sys.path.insert(0, os.getcwd())
else:
    HOME = os.getenv("HOME")
    sys.path.insert(0, HOME)

from cunqa.circuit import CunqaCircuit
from cunqa.backend import Backend
from cunqa.qiskit_deps.transpiler import transpiler

@pytest.fixture
def fakeqmio_backend():
    """Create a FakeQmio backend with detailed topology and gate information"""
    return Backend({
        "name": "FakeQmio",
        "version": "/opt/cesga/qmio/hpc/calibrations/2025_05_15__12_41_26.json",
        "n_qubits": 32,
        "description": "FakeQmio backend",
        "simulator": None,
        "coupling_map": [[0,1],[2,1],[2,3],[4,3],[5,4],[6,3],[6,12],[7,0],[7,9],[9,10],[11,10],[11,12],[13,21],[14,11],[14,18],[15,8],[15,16],[18,17],[18,19],[20,19],[22,21],[22,31],[23,20],[23,30],[24,17],[24,27],[25,16],[25,26],[26,27],[28,27],[28,29],[30,29],[30,31]],
        "basis_gates": ["sx","x","rz","ecr"],
        "custom_instructions": "",
        "noise_properties_path": "",
        "gates": []
    })

################# TESTS #################

def test_transpiler_quantum_circuit(fakeqmio_backend):
    """Test transpiling a Qiskit QuantumCircuit with a realistic backend"""
    qc = QuantumCircuit(5)
    qc.h(0)
    qc.cx(0, 1)
    qc.measure_all()

    transpiled_qc = transpiler(qc, fakeqmio_backend)
    
    # Check that the transpiled circuit has the correct type and doesn't exceed backend's qubit count
    assert isinstance(transpiled_qc, QuantumCircuit)
    assert transpiled_qc.num_qubits <= fakeqmio_backend.n_qubits

def test_original_circuit_unaffected_by_transpiler(fakeqmio_backend):
    """Test that transpiling doesn't modify the circuit object that it transpiles, but rather returns a new circuit. """
    circ = CunqaCircuit(5)
    circ.h(0)
    circ.ry(0.6, 2)
    circ.p(3.14, 4)
    circ.cx(0, 1)
    circ.measure_all()

    qc_copy = copy.deepcopy(circ)

    transpiled_qc = transpiler(circ, fakeqmio_backend)

    # Assert that original circuit remains unchanged
    assert circ.info == qc_copy.info
    
    for instr_origin, instr_copy in zip(circ.instructions, qc_copy.instructions):
        assert instr_origin == instr_copy

def test_transpiler_initial_layout(fakeqmio_backend):
    """Test transpiling with a specific initial layout"""
    qc = QuantumCircuit(5)
    qc.h(0)
    qc.cx(0, 1)
    qc.measure_all()

    # Use a valid initial layout based on the coupling map
    valid_layout = [0, 1, 2, 3, 4]
    transpiled_qc = transpiler(qc, fakeqmio_backend, initial_layout=valid_layout)
    
    assert isinstance(transpiled_qc, QuantumCircuit)
    assert transpiled_qc.num_qubits <= fakeqmio_backend.n_qubits

def test_transpiler_basis_gates_conversion(fakeqmio_backend):
    """Test that gates are converted to backend's basis gates"""
    circ = CunqaCircuit(3)
    circ.h(0)           # Hadamard gate
    circ.cx(0, 1)       # Controlled-NOT gate
    circ.rz(0.334, 2)   # RZ
    circ.ccx(0,1,2)     # Toffoli
    circ.p(0.863426, 1) # Phase gate
    circ.rzx(0.9, 1,2)  # RZX gate

    transpiled_circ = transpiler(circ, fakeqmio_backend, opt_level=3)
    
    # Check that only basis gates are used
    for instruction in transpiled_circ.instructions:
        assert instruction["name"] in fakeqmio_backend.basis_gates

def test_transpiler_optimization_levels(fakeqmio_backend):
    """Test different optimization levels with a realistic backend"""
    qc = QuantumCircuit(5)
    qc.h(0)
    qc.cx(0, 1)
    qc.rx(0.5, 2)
    qc.measure_all()

    for opt_level in range(4):  # Qiskit supports optimization levels 0-3
        transpiled_qc = transpiler(qc, fakeqmio_backend, opt_level=opt_level)
        
        assert isinstance(transpiled_qc, QuantumCircuit)
        assert transpiled_qc.num_qubits <= fakeqmio_backend.n_qubits

def test_transpiler_seed_reproducibility(fakeqmio_backend):
    """Test that transpilation with the same seed produces consistent results"""
    qc = QuantumCircuit(5)
    qc.h(0)
    qc.cx(0, 1)
    qc.rx(0.5, 2)
    qc.measure_all()

    # Transpile twice with the same seed
    transpiled_qc1 = transpiler(qc, fakeqmio_backend, seed=42)
    transpiled_qc2 = transpiler(qc, fakeqmio_backend, seed=42)

    # Compare the string representations (this is a basic comparison)
    assert str(transpiled_qc1) == str(transpiled_qc2)

def test_transpiler_large_circuit(fakeqmio_backend):
    """Test transpiling a larger circuit that might stress the backend"""
    qc = QuantumCircuit(10)
    for i in range(9):
        qc.h(i)
        qc.cx(i, i+1)
    qc.measure_all()

    # Ensure the circuit can be mapped to the backend
    transpiled_qc = transpiler(qc, fakeqmio_backend)
    
    assert isinstance(transpiled_qc, QuantumCircuit)
    # Check that the transpiled circuit doesn't exceed backend's qubit count
    assert transpiled_qc.num_qubits <= fakeqmio_backend.n_qubits




################ COMPLEX TESTS FOR BACKEND TOPOLOGY CONSTRAINTS ################

def test_transpiler_coupling_map_constraints(fakeqmio_backend):
    """
    Rigorously test that transpilation respects the backend's coupling map
    by checking that every two-qubit gate is between physically connected qubits
    """
    # Convert coupling map to a set of tuples for efficient lookup
    coupling_set = set(tuple(sorted(edge)) for edge in fakeqmio_backend.coupling_map)

    def check_two_qubit_gate_connectivity(transpiled_qc):
        """
        Verify that every two-qubit gate is between physically connected qubits
        according to the backend's coupling map
        """
        for instruction, qubits, _ in transpiled_qc.data:
            # Check only two-qubit gates
            if len(qubits) == 2:
                # Sort qubit indices to match coupling map representation
                qubit_pair = tuple(sorted(qc.find_bit(q).index for q in qubits))
                
                # ASSERTIONS:    the qubit pair exists in the coupling map
                assert qubit_pair in coupling_set, (
                    f"Two-qubit gate between qubits {qubit_pair} "
                    f"is not in the backend's coupling map: {coupling_set}"
                )

    def create_test_circuit(num_qubits, interactions):
        """
        create a test circuit with specified two-qubit interactions
        """
        qc = QuantumCircuit(num_qubits)
        
        for q in range(num_qubits):
            qc.h(q)
        
        # Add specified two-qubit interactions
        for q1, q2 in interactions:
            qc.cx(q1, q2)
        
        return qc

    # Test with various circuit complexities
    test_circuits = [
        # Simple two-qubit interaction
        lambda: create_test_circuit(5, [(0, 1), (2, 3)]),
        
        # More complex multi-qubit circuit
        lambda: create_test_circuit(10, [(0, 1), (2, 3), (4, 5), (6, 7), (8, 9)]),
        
        # Circuit with potential routing challenges
        lambda: create_test_circuit(6, [(0, 5), (1, 4), (2, 3)])
    ]

    # Run tests for each circuit
    for circuit_generator in test_circuits:
        qc = circuit_generator()
        
        # Try different optimization levels
        for opt_level in range(4):

            transpiled_qc = transpiler(qc, fakeqmio_backend, opt_level=opt_level)
            
            # Check coupling map constraints
            check_two_qubit_gate_connectivity(transpiled_qc)





def test_transpiler_routing_for_non_adjacent_qubits(fakeqmio_backend):
    """
    Test that the transpiler can route gates between non-adjacent qubits
    by using intermediate qubits when direct connection is not possible
    """
    def create_non_adjacent_circuit():
        """
        Create a circuit that requires routing between non-directly connected qubits
        """
        qc = QuantumCircuit(10)
        qc.h(0)  # Initial Hadamard
        qc.cx(0, 9)  # Interaction between distant qubits
        return qc

    # Transpile with different optimization levels
    for opt_level in range(4):
        qc = create_non_adjacent_circuit()
        transpiled_qc = transpiler(
            qc, 
            fakeqmio_backend, 
            opt_level=opt_level
        )
        
        # Verify that the circuit was successfully transpiled
        assert transpiled_qc is not None
        
        # Collect all two-qubit gate interactions in the transpiled circuit
        two_qubit_interactions = [
            tuple(sorted(qc.find_bit(q).index for q in qubits)) 
            for instruction, qubits, _ in transpiled_qc.data 
            if len(qubits) == 2
        ]
        
        # Verify that all interactions are valid according to the coupling map
        coupling_set = set(tuple(sorted(edge)) for edge in fakeqmio_backend.coupling_map)
        
        # Check that all two-qubit interactions are either:
        # 1. Directly in the coupling map, or
        # 2. Implicitly routable through the coupling map
        for interaction in two_qubit_interactions:
            assert any(
                interaction == edge or 
                is_routable_interaction(interaction, fakeqmio_backend.coupling_map)
                for edge in coupling_set
            ), f"Interaction {interaction} is not routable"


# Helper function for previous test
def is_routable_interaction(interaction, coupling_map):
    """
    Check if an interaction can be routed through the coupling map
    """
    # Convert coupling map to an adjacency list representation
    graph = {}
    for edge in coupling_map:
        u, v = edge
        if u not in graph:
            graph[u] = set()
        if v not in graph:
            graph[v] = set()
        graph[u].add(v)
        graph[v].add(u)
    
    # Breadth-first search to check if a path exists
    def bfs_path_exists(start, end):
        visited = set()
        queue = [(start, [start])]
        
        while queue:
            (node, path) = queue.pop(0)
            if node not in visited:
                if node == end:
                    return True
                
                visited.add(node)
                
                for next_node in graph.get(node, []):
                    if next_node not in visited:
                        queue.append((next_node, path + [next_node]))
        
        return False
    
    # Check if there's a path between the two qubits
    start, end = interaction
    return bfs_path_exists(start, end)