import sys, os
import pytest
import copy, logging
from unittest.mock import Mock, patch
from qiskit.providers.backend import BackendV2
from qiskit.circuit.library import XGate, CXGate
from qiskit.circuit import Parameter
from qiskit.transpiler import Target

sys.path.append(os.getenv("HOME"))
from cunqa.qiskit_deps.cunqabackend import CunqaBackend, _get_qubit_index, _get_qubits_indexes, _get_gate
import cunqa

class TestCunqaBackend:
    @pytest.fixture
    def sample_noise_properties_json(self):
        """Fixture providing a sample noise properties JSON for testing."""
        return {
            "Qubits": {
                "q[0]": {
                    "T1 (s)": 0.0001,
                    "T2 (s)": 0.00005,
                    "Drive Frequency (Hz)": 5e9,
                    "Readout duration (s)": 0.0001,
                    "Readout fidelity (RB)": 0.95
                },
                "q[1]": {
                    "T1 (s)": 0.0002,
                    "T2 (s)": 0.0001,
                    "Drive Frequency (Hz)": 5.1e9,
                    "Readout duration (s)": 0.00015,
                    "Readout fidelity (RB)": 0.93
                }
            },
            "Q1Gates": {
                "q[0]": {
                    "x": {
                        "Gate duration (s)": 0.00005,
                        "Fidelity(RB)": 0.99
                    }
                }
            },
            "Q2Gates(RB)": {
                "0-1": {
                    "cx": {
                        "Control": 0,
                        "Target": 1,
                        "Duration (s)": 0.0001,
                        "Fidelity(RB)": 0.95
                    }
                }
            }
        }

    def test_init_with_noise_properties(self, sample_noise_properties_json):
        """Test initialization with noise properties JSON."""
        backend = CunqaBackend(noise_properties_json=sample_noise_properties_json)
        
        assert backend._num_qubits == 2
        assert hasattr(backend, '_target')
        assert isinstance(backend.target, Target)
        assert backend.target.num_qubits == 2

    def test_init_with_backend(self):
        """Test initialization with an existing backend."""
        backend_json = {
            "name": "NoisyBackend",
            "version": "0.0.1",
            "description": "Example of a noisy backend",
            "n_qubits": 16,
            "basis_gates": [
                "id", "h", "x", "y", "z", "cx", "cy", "cz", "ecr"
            ],
            "custom_instructions": "",
            "gates": [],
            "coupling_map": [],
            "simulator":"Aer",
            "noise_properties_path":"/opt/cesga/qmio/hpc/calibrations/2025_05_15__12_41_26.json"
        }
        

        backend = CunqaBackend(backend=cunqa.backend.Backend(cunqa.backend.BackendData(backend_json)))
        
        assert hasattr(backend, '_target')
        assert backend.target.num_qubits == 32
        assert backend.name == "NoisyBackend"

    def test_get_qubit_index(self):
        """Test _get_qubit_index function."""
        assert _get_qubit_index("q[0]") == 0
        assert _get_qubit_index("q[42]") == 42
        
        with pytest.raises(ValueError):
            _get_qubit_index("invalid")

    def test_get_qubits_indexes(self):
        """Test _get_qubits_indexes function."""
        assert _get_qubits_indexes("0-1") == [0, 1]
        
        with pytest.raises(ValueError):
            _get_qubits_indexes("invalid")

    def test_get_gate(self):
        """Test _get_gate function."""
        x_gate = _get_gate("x")
        assert isinstance(x_gate, XGate)
        
        cx_gate = _get_gate("cx")
        assert isinstance(cx_gate, CXGate)
        
        rx_gate = _get_gate("rx")
        assert len(rx_gate.params) == 1
        assert isinstance(rx_gate.params[0], Parameter)
        
        with pytest.raises(ValueError):
            _get_gate("non_existent_gate")

    def test_basis_gates(self, sample_noise_properties_json):
        """Test basis_gates property."""
        backend = CunqaBackend(noise_properties_json=sample_noise_properties_json)
        
        assert set(backend.basis_gates) == {"x", "cx"}

    def test_max_circuits(self, sample_noise_properties_json):
        """Test max_circuits method."""
        backend = CunqaBackend(noise_properties_json=sample_noise_properties_json)
        assert backend.max_circuits() is None

    def test_coupling_map_list(self, sample_noise_properties_json):
        """Test coupling_map_list property."""
        backend = CunqaBackend(noise_properties_json=sample_noise_properties_json)
        
        # Expecting a list of tuples representing the coupling map
        assert backend.coupling_map_list == [(0, 1)]

    def test_noise_properties_parsing_with_warnings(self, sample_noise_properties_json, caplog):
        """Test parsing noise properties with potential warning scenarios."""
        # Create a deep copy to avoid modifying the original fixture
        test_noise_properties = copy.deepcopy(sample_noise_properties_json)
        
        # Add unsupported gates without modifying the original dictionary during iteration
        test_noise_properties['Q1Gates']['q[0]']['unsupported_gate'] = {}
        test_noise_properties['Q2Gates(RB)']['0-1']['unsupported_gate'] = {
            'Control': 0,
            'Target': 1,
            'Duration (s)': 0.0001,
            'Fidelity(RB)': 0.95
        }

        with pytest.raises(RuntimeError):
           CunqaBackend(noise_properties_json=test_noise_properties)
        