import os
import glob
import argparse
import json
from qmiotools.integrations.qiskitqmio import FakeQmio 
from qiskit_aer.noise import NoiseModel

STORE_PATH = os.getenv("STORE")
INSTALL_PATH = os.getenv("INSTALL_PATH")
BASIS_GATES = ["sx", "x", "rz", "ecr"]
COUPLING_MAP = [
    [0,1],
    [2,1],
    [2,3],
    [4,3],
    [5,4],
    [6,3],
    [6,12],
    [7,0],
    [7,9],
    [9,10],
    [11,10],
    [11,12],
    [13,21],
    [14,11],
    [14,18],
    [15,8],
    [15,16],
    [18,17],
    [18,19],
    [20,19],
    [22,21],
    [22,31],
    [23,20],
    [23,30],
    [24,17],
    [24,27],
    [25,16],
    [25,26],
    [26,27],
    [28,27],
    [28,29],
    [30,29],
    [30,31]
]

parser = argparse.ArgumentParser(description="FakeQmio from calibrations")

parser.add_argument("backend_path", type = str, help = "Path to Qmio calibration file")
parser.add_argument("thermal_relaxation", type = int, help = "Weather thermal relaxation is added to FakeQmio")
parser.add_argument("readout_error", type = int, help = "Weather thermal relaxation is added to FakeQmio")
parser.add_argument("gate_error", type = int, help = "Weather thermal relaxation is added to FakeQmio")
parser.add_argument("family_name", type = str, help = "family_name for QPUs")

args = parser.parse_args()

# set defaults
thermal_relaxation, readout_error, gate_error = True, False, False
# read arguments
if args.thermal_relaxation == 0:
    thermal_relaxation = False
if args.readout_error == 1:
    readout_error = True
if args.gate_error == 1:
    gate_error = True

if (args.backend_path == "last_calibrations"):
    jsonpath=os.getenv("QMIO_CALIBRATIONS",".")
    files=jsonpath+"/????_??_??__??_??_??.json"
    files = glob.glob(files)
    calibration_file=max(files, key=os.path.getctime) 
    args.backend_path = calibration_file
    
    fakeqmio = FakeQmio(None,thermal_relaxation, readout_error, gate_error)
else:
    fakeqmio = FakeQmio(args.backend_path,thermal_relaxation, readout_error, gate_error)

noise_model = NoiseModel.from_backend(fakeqmio)
noise_model_json = noise_model.to_dict(serializable = True)

description = "FakeQmio backend with: "
errors = ""
if thermal_relaxation:
    errors += " thermal_relaxation"
if readout_error:
    errors += " readout_error"
if gate_error:
    errors += " gate_error"

description = description + ", ".join(errors.split())+"."


with open(args.backend_path, "r") as file:
    noise_properties_json = json.load(file)

backend_json = {
        "name": "FakeQmio", 
        "version": args.backend_path,
        "n_qubits": 32, 
        "url": "",
        "description": description,
        "coupling_map" : COUPLING_MAP,
        "basis_gates": BASIS_GATES,
        "custom_instructions": "",
        "gates": [],
        "noise_model":noise_model_json,
        "noise_properties":noise_properties_json
}


with open("{}/.cunqa/tmp_fakeqmio_backend_{}.json".format(STORE_PATH, args.family_name), 'w') as file:
    json.dump(backend_json, file)