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
parser.add_argument("SLURM_JOB_ID", type = int, help = "SLURM_JOB_ID enviroment variable")

args = parser.parse_args()

if (args.backend_path == "last_calibrations"):
    jsonpath=os.getenv("QMIO_CALIBRATIONS",".")
    files=jsonpath+"/????_??_??__??_??_??.json"
    files = glob.glob(files)
    calibration_file=max(files, key=os.path.getctime) 
    args.backend_path = calibration_file
    fakeqmio = FakeQmio()
else:
    fakeqmio = FakeQmio(calibration_file = args.backend_path)

noise_model = NoiseModel.from_backend(fakeqmio)
noise_model_json = noise_model.to_dict(serializable = True)

backend_json = {
    "backend":{
        "name": "FakeQmio", 
        "version": args.backend_path,
        "n_qubits": 32, 
        "url": "",
        "is_simulator": True,
        "conditional": True, 
        "memory": True,
        "max_shots": 1000000,
        "description": "FakeQmio backend",
        "coupling_map" : COUPLING_MAP,
        "basis_gates": BASIS_GATES, 
        "custom_instructions": "",
        "gates": []
    },
    "noise":noise_model_json
}


with open("{}/.cunqa/tmp_fakeqmio_backend_{}.json".format(STORE_PATH, args.SLURM_JOB_ID), 'w') as file:
    json.dump(backend_json, file)
