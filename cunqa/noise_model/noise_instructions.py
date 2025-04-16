import os
import sys

installation_path = os.getenv("INSTALL_PATH")
sys.path.append(installation_path)

from cunqa.logger import logger
import argparse
import json
from jsonschema import validate, ValidationError

from qiskit_aer.noise import NoiseModel

schema_properties = "json_schema/calibrations_schema.json"


# constructing NoiseModel from calibrations using qmiotools
from qmiotools.integrations.qiskitqmio import FakeQmio 
from qiskit_aer.noise import NoiseModel

parser = argparse.ArgumentParser(description="FakeQmio from calibrations")

# we ask here for arguments when running the script: path to the calibration 
parser.add_argument("properties", type = str, help = "Path to backend properties file")
parser.add_argument("family_name", type = str, help = "family_name for QPUs")

args = parser.parse_args()


# validating json schema for properties
with open(schema_properties, "r") as file:
    schema_properties = json.load(file)

with open(args.properties, "r") as file:
    properties_json = json.load(file)

try:
    validate(instance=properties_json, schema=schema_properties)
    logger.debug("properties json is valid!")

except ValidationError as error:
    logger.error(f"Error validating properties json [{type(error).__name__}:{error.message}]")
    raise SystemExit


from cunqabackend import CunqaBackend

backend = CunqaBackend(properties_json = properties_json)

from qiskit_aer.noise import NoiseModel

noise_model = NoiseModel.from_backend(
        backend, thermal_relaxation=True,
        temperature=True,
        gate_error=True,
        readout_error=True)

noise_model_json = noise_model_json = noise_model.to_dict(serializable = True)

STORE_PATH = os.getenv("STORE")

with open("{}/.cunqa/tmp_noise_model_{}.json".format(STORE_PATH, args.family_name), 'w') as file:
    json.dump(noise_model_json, file)

