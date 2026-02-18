import os, sys
import glob
import argparse
import json
import fcntl

# Append to path to access CUNQA installation 
sys.path.append(os.getenv("HOME"))

from cunqa.constants import CUNQA_PATH
from cunqa.logger import logger
from cunqa.qiskit_deps.cunqabackend import CunqaBackend
from qiskit_aer.noise import NoiseModel

def create_parser():
    """
    Create and return the configured argument parser
    """
    parser = argparse.ArgumentParser(description="Your script description")
    
    # Add your arguments
    parser.add_argument("noise_properties_path", type=str, help="Path to calibrations noise_properties file")
    parser.add_argument("backend_path", type=str, help="Path to backend noise_properties file")
    parser.add_argument("thermal_relaxation", type=int, help="Whether thermal relaxation is added")
    parser.add_argument("readout_error", type=int, help="Whether readout error is added")
    parser.add_argument("gate_error", type=int, help="Whether gate error is added")
    parser.add_argument("family_name", type=str, help="Family name for QPUs")
    parser.add_argument("fakeqmio", type=int, help="FakeQmio noise properties provided")
    
    return parser

def validate_json_schema(json_data, schema_path):
    """
    Validate JSON data against a given schema.
    
    Args:
        json_data (dict): JSON data to validate
        schema_path (str): Path to the JSON schema file
    
    Raises:
        ValueError: If JSON validation fails
    """
    # TODO: Implement proper JSON schema validation
    # You might want to use jsonschema library for this
    try:
        with open(schema_path, "r") as schema_file:
            schema = json.load(schema_file)
        
        # Placeholder for actual validation logic
        # jsonschema.validate(instance=json_data, schema=schema)
    except Exception as e:
        logger.error(f"JSON schema validation failed: {e}")
        raise ValueError(f"Invalid JSON: {e}")

def load_noise_properties(noise_properties_path):
    """
    Load noise properties from a given path or use last calibration.
    
    Args:
        noise_properties_path (str): Path to noise properties file
    
    Returns:
        dict: Noise properties JSON
    """
    if noise_properties_path == "last_calibrations":
        # Find the most recent calibration file
        jsonpath = "/opt/cesga/qmio/hpc/calibrations"
        files = glob.glob(os.path.join(jsonpath, "????_??_??__??_??_??.json"))
        
        if not files:
            raise FileNotFoundError("No calibration files found")
        
        calibration_file = max(files, key=os.path.getctime)
        logger.debug(f"Using latest calibration file: {calibration_file}")
        
        with open(calibration_file, "r") as file:
            return json.load(file)
    else:
        # Load from specified path
        with open(noise_properties_path, "r") as file:
            return json.load(file)

def create_noise_model(backend, thermal_relaxation, readout_error, gate_error):
    """
    Create a noise model for the given backend and error configurations.
    
    Args:
        backend (CunqaBackend): Quantum backend
        thermal_relaxation (bool): Enable thermal relaxation
        readout_error (bool): Enable readout error
        gate_error (bool): Enable gate error
    
    Returns:
        NoiseModel: Configured noise model
    """
    try:
        noise_model = NoiseModel.from_backend(
            backend, 
            thermal_relaxation=thermal_relaxation,
            temperature=True,
            gate_error=gate_error,
            readout_error=readout_error
        )
        return noise_model
    except Exception as error:
        logger.error(f"Error while generating noise model: {error}")
        raise

def prepare_backend_json(backend, args, noise_model, noise_properties_path):
    """
    Prepare backend JSON with noise model and configuration details.
    
    Args:
        backend (CunqaBackend): Quantum backend
        args (argparse.Namespace): Parsed command-line arguments
        noise_model (NoiseModel): Generated noise model
        noise_properties_path (str): Path to noise properties
    
    Returns:
        dict: Backend configuration JSON
    """
    # Construct description based on enabled error types
    errors = []
    if args.thermal_relaxation:
        errors.append("thermal_relaxation")
    if args.readout_error:
        errors.append("readout_error")
    if args.gate_error:
        errors.append("gate_error")
    
    description = f"{'CunqaBackend' if not args.fakeqmio else 'FakeQmio'} with: {', '.join(errors)}."
    
    # If no backend path provided, generate default backend JSON
    if args.backend_path == "default":
        return {
            "name": f"{'CunqaBackend' if not args.fakeqmio else 'FakeQmio'}_{args.family_name}",
            "version": "",
            "n_qubits": backend.num_qubits, 
            "description": description,
            "coupling_map": backend.coupling_map_list,
            "basis_gates": backend.basis_gates,
            "custom_instructions": "",
            "gates": [],
            "noise_model": noise_model.to_dict(serializable=True),
            "noise_properties_path": noise_properties_path,
            "noise_path": ""  # Will be filled later
        }
    else:
        # Load existing backend JSON and update it
        with open(args.backend_path, "r") as file:
            backend_json = json.load(file)

        # TODO: validate backend_json
        #validate_json_schema(backend_json, schema_backend)
        
        backend_json.update({
            "noise_model": noise_model.to_dict(serializable=True),
            "noise_properties_path": noise_properties_path,
            "noise_path": ""  # Will be filled later
        })
        
        return backend_json

def write_backend_json(backend_json, tmp_file):
    """
    Write backend JSON to a temporary file with file locking.
    
    Args:
        backend_json (dict): Backend configuration JSON
        tmp_file (str): Path to temporary file
    """
    os.makedirs(os.path.dirname(tmp_file), exist_ok=True)
    
    with open(tmp_file, 'w') as file:
        fcntl.flock(file.fileno(), fcntl.LOCK_EX)
        try:
            json.dump(backend_json, file, indent=2)
            file.flush()
            os.fsync(file.fileno())
        finally:
            fcntl.flock(file.fileno(), fcntl.LOCK_UN)

def main(args=None):
    """
    Main function to process noise properties and generate backend configuration.
    
    Args:
        args (argparse.Namespace, optional): Parsed command-line arguments. 
                                             If None, parse from sys.argv.
    """
    # Parse arguments if not provided
    if args is None:
        parser = create_parser()
        args = parser.parse_args()
    
    # TODO: Paths to JSON schemas
    # schema_noise_properties = os.path.join(CUNQA_PATH, "json_schema", "calibrations_schema.json")
    # schema_backend = os.path.join(CUNQA_PATH, "json_schema", "backend_schema.json")
    
    try:
        # Load and validate noise properties
        noise_properties_json = load_noise_properties(args.noise_properties_path)
        # TODO: validate noise_properties_json
        #validate_json_schema(noise_properties_json, schema_noise_properties)
        
        # Create backend
        backend = CunqaBackend(noise_properties_json=noise_properties_json)
        
        # Determine error configurations
        thermal_relaxation = bool(args.thermal_relaxation)
        readout_error = bool(args.readout_error)
        gate_error = bool(args.gate_error)
        
        # Log error configurations
        logger.debug(f"Thermal Relaxation: {thermal_relaxation}")
        logger.debug(f"Readout Error: {readout_error}")
        logger.debug(f"Gate Error: {gate_error}")
        
        # Generate noise model
        noise_model = create_noise_model(
            backend, 
            thermal_relaxation, 
            readout_error, 
            gate_error
        )
        
        # Prepare backend JSON
        backend_json = prepare_backend_json(
            backend, 
            args, 
            noise_model, 
            args.noise_properties_path
        )
        
        # Generate temporary file path
        slurm_job_id = os.getenv("SLURM_JOB_ID", "unknown")
        tmp_file = os.path.join(CUNQA_PATH, f"tmp_noisy_backend_{slurm_job_id}.json")
        backend_json["noise_path"] = tmp_file
        
        # Write backend JSON
        write_backend_json(backend_json, tmp_file)
        
        logger.debug(f"Created noisy backend: {backend_json['description']}")
        
        return backend_json
    
    except Exception as e:
        logger.error(f"Error in main function: {e}")
        raise SystemExit(1)

# Allow script to be run directly
if __name__ == "__main__":
    main()
