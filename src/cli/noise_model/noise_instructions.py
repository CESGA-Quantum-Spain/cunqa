import os
import sys
import glob

installation_path = os.getenv("HOME")
STORE_PATH = os.getenv("STORE")
SLURM_JOB_ID = os.getenv("SLURM_JOB_ID")
sys.path.append(installation_path)

from cunqabackend import CunqaBackend
from cunqa.logger import logger

import argparse
import json

from qiskit_aer.noise import NoiseModel
import fcntl

schema_noise_properties = os.getenv("STORE") + "/.cunqa/json_schema/calibrations_schema.json"
schema_backend = os.getenv("STORE") + "/.cunqa/json_schema/backend_schema.json"

parser = argparse.ArgumentParser(description="FakeQmio from calibrations")

# we ask here for arguments when running the script: path to the calibration 
parser.add_argument("noise_properties_path", type = str, help = "Path to calibrations noise_properties file")
parser.add_argument("backend_path",  type = str, help = "Path to backend noise_properties file")
parser.add_argument("thermal_relaxation", type = int, help = "Weather thermal relaxation is added to FakeQmio")
parser.add_argument("readout_error", type = int, help = "Weather thermal relaxation is added to FakeQmio")
parser.add_argument("gate_error", type = int, help = "Weather thermal relaxation is added to FakeQmio")
parser.add_argument("family_name", type = str, help = "family_name for QPUs")
parser.add_argument("fakeqmio", type = int, help = "FakeQmio noise properties provided")



args = parser.parse_args()


# validating json schema for noise_properties (mandatory)
with open(schema_noise_properties, "r") as file:
    schema_noise_properties = json.load(file)


if args.noise_properties_path == "last_calibrations":

    # fakeqmio with default calibrations

    jsonpath = "/opt/cesga/qmio/hpc/calibrations"
    files = jsonpath + "/????_??_??__??_??_??.json"
    files = glob.glob(files)
    calibration_file=max(files, key=os.path.getctime)

    with open(calibration_file, "r") as file:
        noise_properties_json = json.load(file)
else:

    # personalized noise model or fakeqmio with non default calibrations

    with open(args.noise_properties_path, "r") as file:
        noise_properties_json = json.load(file)

#TODO: validate noise_properties_json with respect to schema_noise_properties

# validating json schema for backend (optional)

if args.backend_path != "default":

    with open(schema_backend, "r") as file:
        schema_backend = json.load(file)

    with open(args.backend_path, "r") as file:
        backend_json = json.load(file)

    #TODO: validate backend_json with respect to schema_backend


thermal_relaxation, readout_error, gate_error = True, False, False
# read arguments
if args.thermal_relaxation == 0:
    logger.debug("thermal_relaxation = False")
    thermal_relaxation = False
else:
    logger.debug("thermal_relaxation = True")
    
if args.readout_error == 1:
    readout_error = True
    logger.debug("readout_error = True")
else:
    logger.debug("readout_error = False")

if args.gate_error == 1:
    gate_error = True
    logger.debug("gate_error = True")
else:
    logger.debug("gate_error = False")


if args.fakeqmio:
    logger.debug(f"FakeQmio noise properties provided.")


backend = CunqaBackend(noise_properties_json = noise_properties_json)

try:
    noise_model = NoiseModel.from_backend(
            backend, thermal_relaxation=thermal_relaxation,
            temperature=True,
            gate_error=gate_error,
            readout_error=readout_error)
    
    noise_model_json = noise_model_json = noise_model.to_dict(serializable = True)

    
except Exception as error:
    logger.error(f"Error while generating noise model: {error}")
    raise SystemExit


description = "CunqaBackend with: " if not args.fakeqmio else "FakeQmio with: "
errors = ""
if thermal_relaxation:
    errors += " thermal_relaxation"
if readout_error:
    errors += " readout_error"
if gate_error:
    errors += " gate_error"

description = description + ", ".join(errors.split())+"."

tmp_file = "{}/.cunqa/tmp_noisy_backend_{}.json".format(STORE_PATH, SLURM_JOB_ID)

if args.backend_path == "default": # we have not read the backend_json and checked it, we generate it using CunqaBackend

    backend_json = {
            "name": f"CunqaBackend_{args.family_name}" if not args.fakeqmio else f"FakeQmio_{args.family_name}", 
            "version": "",
            "n_qubits": backend.num_qubits, 
            "description": description,
            "coupling_map" : backend.coupling_map_list,
            "basis_gates": backend.basis_gates,
            "custom_instructions": "",
            "gates": [],
            "noise_model":noise_model_json,
            "noise_properties_path":args.noise_properties_path,
            "noise_path": tmp_file
    }

else:
    backend_json = {
        "name": args.backend_json["name"], 
        "version": args.backend_json["version"],
        "n_qubits": args.backend_json["n_qubits"], 
        "description": args.backend_json["description"],
        "coupling_map" : args.backend_json["coupling_map"],
        "basis_gates": args.backend_json["basis_gates"],
        "custom_instructions": args.backend_json["custom_instructions"],
        "gates": args.backend_json["gates"],
        "noise_model":noise_model_json,
        "noise_properties_path":args.noise_properties_path,
        "noise_path": tmp_file
    }


logger.debug(f"Created noisy backend: {description}")

os.makedirs(os.path.dirname(tmp_file), exist_ok=True)

with open(tmp_file, 'w') as file:
    fcntl.flock(file.fileno(), fcntl.LOCK_EX)
    json.dump(backend_json, file)
    file.flush()
    os.fsync(file.fileno())
    fcntl.flock(file.fileno(), fcntl.LOCK_UN)