import os
import sys

installation_path = os.getenv("HOME")+"/cunqa/"
sys.path.append(installation_path)

from logger import logger
import argparse
import json
from jsonschema import validate, ValidationError

from qiskit_aer.noise import NoiseModel

schema_noise_properties = installation_path + "noise_model/json_schema/calibrations_schema.json"
schema_backend = installation_path + "noise_model/json_schema/backend_schema.json"

# constructing NoiseModel from calibrations using qmiotools
from qmiotools.integrations.qiskitqmio import FakeQmio 
from qiskit_aer.noise import NoiseModel

parser = argparse.ArgumentParser(description="FakeQmio from calibrations")

# we ask here for arguments when running the script: path to the calibration 
parser.add_argument("noise_properties_path", type = str, help = "Path to calibrations noise_properties file")
parser.add_argument("backend_path",  type = str, help = "Path to backend noise_properties file")
parser.add_argument("thermal_relaxation", type = int, help = "Weather thermal relaxation is added to FakeQmio")
parser.add_argument("readout_error", type = int, help = "Weather thermal relaxation is added to FakeQmio")
parser.add_argument("gate_error", type = int, help = "Weather thermal relaxation is added to FakeQmio")
parser.add_argument("family_name", type = str, help = "family_name for QPUs")

args = parser.parse_args()


# validating json schema for noise_properties (mandatory)
with open(schema_noise_properties, "r") as file:
    schema_noise_properties = json.load(file)

with open(args.noise_properties_path, "r") as file:
    noise_properties_json = json.load(file)

try:
    validate(instance=noise_properties_json, schema=schema_noise_properties)
    logger.debug("noise_properties json is valid!")

except ValidationError as error:
    logger.error(f"Error validating noise_properties json [{type(error).__name__}:{error.message}]")
    raise SystemExit


# validating json schema for backend (optional)

if args.backend_path != "default":

    with open(schema_backend, "r") as file:
        schema_backend = json.load(file)

    with open(args.backend_path, "r") as file:
        backend_json = json.load(file)

    try:
        validate(instance=backend_json, schema=schema_backend)
        logger.debug("backend json is valid!")

    except ValidationError as error:
        logger.error(f"Error validating backend json [{type(error).__name__}:{error.message}]")
        raise SystemExit



thermal_relaxation, readout_error, gate_error = True, False, False
# read arguments
if args.thermal_relaxation == 0:
    thermal_relaxation = False
if args.readout_error == 1:
    readout_error = True
if args.gate_error == 1:
    gate_error = True


from cunqabackend import CunqaBackend

backend = CunqaBackend(noise_properties_json = noise_properties_json)

from qiskit_aer.noise import NoiseModel

noise_model = NoiseModel.from_backend(
        backend, thermal_relaxation=thermal_relaxation,
        temperature=True,
        gate_error=gate_error,
        readout_error=readout_error)

noise_model_json = noise_model_json = noise_model.to_dict(serializable = True)



description = "CunqaBackend with: "
errors = ""
if thermal_relaxation:
    errors += " thermal_relaxation"
if readout_error:
    errors += " readout_error"
if gate_error:
    errors += " gate_error"

description = description + ", ".join(errors.split())+"."

if args.backend_path == "default": # we have not read the backend_json and checked it, we generate it using CunqaBackend

    backend_json = {
            "name": f"CunqaBackend_{args.family_name}", 
            "version": args.backend_path,
            "n_qubits": backend.num_qubits, 
            "description": description,
            "coupling_map" : backend.coupling_map_list,
            "basis_gates": backend.basis_gates,
            "custom_instructions": "",
            "gates": [],
            "noise_model":noise_model_json,
            "noise_properties":noise_properties_json
    }

else:
    backend_json = {
        "backend":args.backend_json["backend"],
        "noise_model":noise_model_json,
        "noise_properties":noise_properties_json
    }

STORE_PATH = os.getenv("STORE")

logger.debug(f"Created noisy backend: {description}")


with open("{}/.cunqa/tmp_noisy_backend_{}.json".format(STORE_PATH, args.family_name), 'w') as file:
    json.dump(backend_json, file)
