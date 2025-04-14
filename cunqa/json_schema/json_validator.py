import json
from jsonschema import validate, ValidationError


#schema = "config_file_schema.json"
#file_to_validate = "../../examples/backends/example_backend.json"

schema = "calibrations_schema.json"
file_to_validate = "../../examples/noise_example/calibration_file.json"


with open(schema, "r") as config_schema:
    my_schema = json.load(config_schema)


with open(file_to_validate, "r") as backend:
    json_to_validate = json.load(backend)


try:
    validate(instance=json_to_validate, schema=my_schema)
    print("JSON is valid!")
except ValidationError as e:
    print("Validation error:")
    print(f"{e.message}")