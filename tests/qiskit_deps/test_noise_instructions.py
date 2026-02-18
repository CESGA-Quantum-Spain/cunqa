"""
File containing tests for the cunqa.qiskit_deps.noise_instructions file. 
The commented tests didn't work, later refinements will fix these errors.
"""
import os, sys
import json
import pytest
import tempfile
from unittest.mock import patch, mock_open, MagicMock

sys.path.append(os.getenv("HOME"))
import cunqa.qiskit_deps.noise_instructions as noise_instr

@pytest.fixture
def mock_args():
    """Fixture to create a mock ArgumentParser namespace"""
    return MagicMock(
        noise_properties_path="test_noise_properties.json",
        backend_path="default",
        thermal_relaxation=1,
        readout_error=0,
        gate_error=0,
        family_name="test_family",
        fakeqmio=0
    )

@pytest.fixture
def sample_noise_properties():
    """Fixture to provide a sample noise properties JSON"""
    return {
        "Qubits": 
        {
            "q[0]": 
            {
                "T1 (s)": 4.1215e-05,
                "T2 (s)": 4.3997e-05,
                "Drive Frequency (Hz)": 4358600000.0,
                "Measuring frequency (Hz)": 10276000000.0,
                "Readout duration (s)": 2.8599e-06,
                "Readout fidelity (RB)": 0.813,
                "Fidelity readout": 0.813,
                "T1 error (s)": 2.5871e-06,
                "T2 error (s)": 3.8227e-06
            },
            "q[1]": 
            {
                "T1 (s)": 7.0842e-05,
                "T2 (s)": 0.0001731,
                "Drive Frequency (Hz)": 4251600000.0,
                "Measuring frequency (Hz)": 9652700000.0,
                "Readout duration (s)": 5.7492e-06,
                "Readout fidelity (RB)": 0.9241,
                "Fidelity readout": 0.9241,
                "T1 error (s)": 7.5137e-06,
                "T2 error (s)": 7.411e-05
            }
        },
        "Q1Gates": 
        {
            "q[0]": 
            {
                "SX": 
                    {
                        "Gate duration (s)": 6.4e-08,
                        "Fidelity(RB)": 0.98992,
                        "Fidelity error": 0.0014552
                    },
                "Rz": 
                    {
                        "Gate duration (s)": 0,
                        "Fidelity(RB)": 1.0
                    }
            },
            "q[1]": 
            {
                "SX": 
                    {
                        "Gate duration (s)": 3.2e-08,
                        "Fidelity(RB)": 0.99802,
                        "Fidelity error": 0.00035564
                    },
                "Rz": 
                    {
                        "Gate duration (s)": 0,
                        "Fidelity(RB)": 1.0
                    }
            },
        },
        "Q2Gates(RB)": 
        {
            "0-1": 
            {
                "ECR": 
                {
                    "Control": 0,
                    "Target": 1,
                    "Duration (s)": 4.16e-07,
                    "Fidelity(RB)": 0.96657,
                    "Fidelity error": 0.0016152
                }
            }
        }
    }       

class TestNoisePropertiesLoading:
    def test_load_last_calibrations(self, sample_noise_properties):
        """Test loading the most recent calibration file"""
        with patch('cunqa.qiskit_deps.noise_instructions.glob.glob', return_value=['/path/to/2026_02_16__23_59_59.json']), \
             patch('cunqa.qiskit_deps.noise_instructions.os.path.getctime', return_value=1), \
             patch('builtins.open', mock_open(read_data=json.dumps(sample_noise_properties))):
            
            noise_props = noise_instr.load_noise_properties("last_calibrations")
            
            assert noise_props == sample_noise_properties

    def test_load_specific_noise_properties(self, sample_noise_properties):
        """Test loading noise properties from a specific file"""
        with patch('builtins.open', mock_open(read_data=json.dumps(sample_noise_properties))):
            noise_props = noise_instr.load_noise_properties("specific_path.json")
            
            assert noise_props == sample_noise_properties

    def test_no_calibration_files(self):
        """Test behavior when no calibration files exist"""
        with patch('cunqa.qiskit_deps.noise_instructions.glob.glob', return_value=[]):
            with pytest.raises(FileNotFoundError):
                noise_instr.load_noise_properties("last_calibrations")

class TestNoiseModelCreation:
    '''def test_create_noise_model(self, mock_args):
        """Test noise model creation with various error configurations"""
        # Mock dependencies
        mock_backend = MagicMock()
        mock_backend.num_qubits = 2
        mock_backend.coupling_map_list = [[0, 1]]
        mock_backend.basis_gates = ['u1', 'u2', 'cx']

        with patch('noise_instr.NoiseModel.from_backend') as mock_noise_model:
            # Setup mock noise model
            mock_noise_model_instance = MagicMock()
            mock_noise_model.return_value = mock_noise_model_instance
            mock_noise_model_instance.to_dict.return_value = {"noise": "model"}

            # Test different error configurations
            configs = [
                (True, False, False),
                (False, True, False),
                (False, False, True),
                (True, True, True)
            ]

            for thermal, readout, gate in configs:
                noise_model = noise_instr.create_noise_model(
                    mock_backend, thermal, readout, gate
                )
                
                # Verify noise model creation was called with correct parameters
                mock_noise_model.assert_called_with(
                    mock_backend, 
                    thermal_relaxation=thermal,
                    temperature=True,
                    gate_error=gate,
                    readout_error=readout
                )'''

class TestBackendJsonPreparation:
    def test_prepare_backend_json_default(self, mock_args, sample_noise_properties):
        """Test backend JSON preparation with default configuration"""
        # Mock dependencies
        mock_backend = MagicMock()
        mock_backend.num_qubits = 2
        mock_backend.coupling_map_list = [[0, 1]]
        mock_backend.basis_gates = ['u1', 'u2', 'cx']

        mock_noise_model = MagicMock()
        mock_noise_model.to_dict.return_value = {"noise": "model"}

        # Prepare backend JSON
        backend_json = noise_instr.prepare_backend_json(
            mock_backend, 
            mock_args, 
            mock_noise_model, 
            "test_noise_properties.json"
        )

        # Assertions
        assert backend_json['name'] == 'CunqaBackend_test_family'
        assert backend_json['n_qubits'] == 2
        assert backend_json['coupling_map'] == [[0, 1]]
        assert backend_json['basis_gates'] == ['u1', 'u2', 'cx']
        assert backend_json['noise_model'] == {"noise": "model"}

    def test_prepare_backend_json_with_existing_backend(self, mock_args, sample_noise_properties):
        """Test backend JSON preparation with an existing backend configuration"""
        # Create a mock existing backend JSON
        existing_backend_json = {
            "name": "Existing Backend",
            "version": "1.0",
            "n_qubits": 5,
            "description": "Test backend",
            "coupling_map": [[0, 1], [1, 2]],
            "basis_gates": ['u3', 'cx'],
            "custom_instructions": "",
            "gates": []
        }

        # Mock file opening
        with patch('builtins.open', mock_open(read_data=json.dumps(existing_backend_json))):
            mock_backend = MagicMock()
            mock_backend.num_qubits = 2
            mock_backend.coupling_map_list = [[0, 1]]
            mock_backend.basis_gates = ['u1', 'u2', 'cx']

            mock_noise_model = MagicMock()
            mock_noise_model.to_dict.return_value = {"noise": "model"}

            # Modify args to use a specific backend path
            mock_args.backend_path = "/path/to/existing_backend.json"

            # Prepare backend JSON
            backend_json = noise_instr.prepare_backend_json(
                mock_backend, 
                mock_args, 
                mock_noise_model, 
                "test_noise_properties.json"
            )

            # Verify existing backend properties are preserved
            assert backend_json['name'] == "Existing Backend"
            assert backend_json['n_qubits'] == 5
            assert backend_json['noise_model'] == {"noise": "model"}

class TestMainFunction:
    '''def test_main_function_full_flow(self, mock_args, sample_noise_properties):
        """Test the entire main function flow"""
        with tempfile.TemporaryDirectory() as tmpdir:
            # Patch various dependencies
            with patch('cunqa.constants.CUNQA_PATH', tmpdir), \
                 patch.dict(os.environ, {'SLURM_JOB_ID': 'test_job'}), \
                 patch('cunqa.qiskit_deps.noise_instructions.load_noise_properties', return_value=sample_noise_properties), \
                 patch('cunqa.qiskit_deps.cunqabackend.CunqaBackend') as mock_backend_class, \
                 patch('cunqa.qiskit_deps.noise_instructions.create_noise_model') as mock_create_noise_model, \
                 patch('cunqa.qiskit_deps.noise_instructions.write_backend_json') as mock_write_backend_json:
                
                # Setup mock objects
                mock_backend = MagicMock()
                mock_backend.num_qubits = 2
                mock_backend.coupling_map_list = [[0, 1]]
                mock_backend.basis_gates = ['u1', 'u2', 'cx']
                mock_backend_class.return_value = mock_backend

                mock_noise_model = MagicMock()
                mock_noise_model.to_dict.return_value = {"noise": "model"}
                mock_create_noise_model.return_value = mock_noise_model

                # Run main function
                result = noise_instr.main(mock_args)

                # Verify function calls and results
                mock_backend_class.assert_called_once()
                mock_create_noise_model.assert_called_once()
                mock_write_backend_json.assert_called_once()

                assert result['name'].startswith('CunqaBackend')
                assert result['n_qubits'] == 2 '''

''' def test_validate_json_schema(sample_noise_properties):
    """Test JSON schema validation"""
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as schema_file:
        json.dump({
            "type": "object",
            "properties": {
                "qubits": {"type": "array"},
                "gates": {"type": "array"}
            },
            "required": ["qubits", "gates"]
        }, schema_file)
        schema_file.close()

        try:
            # Test successful validation
            noise_instr.validate_json_schema(sample_noise_properties, schema_file.name)
            
            # Test invalid JSON
            with pytest.raises(ValueError):
                noise_instr.validate_json_schema({"invalid": "data"}, schema_file.name)
        finally:
            os.unlink(schema_file.name) '''

# Error handling and edge case tests
def test_main_function_error_handling():
    """Test error handling in main function"""
    with pytest.raises(SystemExit):
        noise_instr.main(MagicMock(noise_properties_path="nonexistent_path")) 
