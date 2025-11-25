
<p align="center">
  <a>
    <img src="https://img.shields.io/badge/os-linux-blue" alt="Python version" height="18">
  </a>
  <a>
    <img src="https://img.shields.io/badge/python-3.9-blue.svg" alt="Python version" height="18">
  </a>
  <a href="cesga-quantum-spain.github.io/cunqa/">
    <img src="https://img.shields.io/badge/docs-blue.svg" alt="Python version" height="18">
  </a>
</p>

<p align="center">
  <a href="https://cesga-quantum-spain.github.io/cunqa/">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/source/_static/logo_cunqa_white.png" width="600" height="150">
    <source media="(prefers-color-scheme: light)" srcset="docs/source/_static/logo_cunqa_black.png" width="600" height="150">
    <img src="docs/source/_static/logo_cunqa_black.png" width="30%" style="display: inline-block;" alt="CUNQA logo">
  </picture>
  </a>
</p>

<p align="center">
  A HPC platform to simulate Distributed Quantum Computing. 
</p>

<br>

<p align="center">
  <a href="https://cesga-quantum-spain.github.io/cunqa/">
  <img width=30% src="https://img.shields.io/badge/documentation-black?style=for-the-badge&logo=read%20the%20docs" alt="Documentation">
  </a>
</p>


<p>
    <div align="center">
      <a href="https://www.cesga.es/">
        <picture>
          <source media="(prefers-color-scheme: dark)" srcset="docs/source/_static/logo_cesga_blanco.png" width="200" height="50">
          <source media="(prefers-color-scheme: light)" srcset="docs/source/_static/logo_cesga_negro.png" width="200" height="50">
          <img src="docs/source/_static/logo_cesga_negro.png" width="30%" style="display: inline-block;" alt="CESGA logo">
        </picture>
      </a>
      <a href="https://quantumspain-project.es/">
        <picture>
          <source media="(prefers-color-scheme: dark)" srcset="docs/source/_static/QuantumSpain_logo_white.png" width="240" height="50">
          <source media="(prefers-color-scheme: light)" srcset="docs/source/_static/QuantumSpain_logo_color.png" width="240" height="50">
          <img src="docs/source/_static/QuantumSpain_logo_white.png" width="30%" style="display: inline-block;" alt="QuantumSpain logo">
        </picture>
      </a>
    </div>
</p>

## Authors 
<ul>
  <li> <a href="https://github.com/martalosada">Marta Losada Estévez </a> </li>
  <li> <a href="https://github.com/jorgevazquezperez">Jorge Vázquez Pérez </a></li>
  <li> <a href="https://github.com/alvarocarballido">Álvaro Carballido Costas </a></li>
  <li> <a href="https://github.com/D-Exposito">Daniel Expósito Patiño </a></li>
  <br> 
</ul>

# Table of contents
  - [INSTALLATION](#installation)
    - [Clone repository](#clone-repository)
    - [Define STORE environment variable](#clone-repository)
    - [Dependencies](#dependencies)
    - [Configure, build and install](#configure-build-and-install)
    - [Install as Lmod module](#install-as-lmod-module)
  - [UNINSTALL](#uninstall)
  - [RUN YOUR FIRST DISTRIBUTED PROGRAM](#run-your-first-distributed-program)
    1. [`qraise`command](#1-qraisecommand)
    2. [Python Program Example](#2-python-program-example)
    3. [`qdrop` command](#3-qdrop-command)
  - [ACKNOWLEDGEMENTS](#acknowledgements)

## Installation 
### Clone repository
It is important to say that, for ensuring a correct cloning of the repository, the SSH is the one preferred. In order to get this to work one has to do:

```console
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/SSH_KEY
```
 
Now, everything is set to get the source code. 

```console
git clone git@github.com:CESGA-Quantum-Spain/cunqa.git
```

### Define STORE environment variable
Before doing any kind of compilation, the user has to define the `STORE` environment variable in bash. This repository will be the root for the `.cunqa` folder, where CUNQA is going to store several runtime files (configuration files and logging files, mainly).

```console
export STORE=/path/to/your/store
```

### Dependencies
CUNQA has a set of dependencies. They are divided in three main groups.

- Must be installed before configuration.
- Can be installed, but if they are not they will be by the configuration process.
- They will be installed by the configuration process.

From the first group, **the ones that must be installed**, the dependencies are the following. The versions here displayed are the ones that have been employed in the development and, therefore, that are recommended.

```
gcc             12.3.0
qiskit          1.2.4
CMake           3.21.0
python          3.9 (recommended 3.11)
pybind11        2.7 (recommended 2.12)
MPI             3.1
OpenMP          4.5
Boost           1.85.0
Blas            -
Lapack          -
```

From the second group, **the ones that will be installed if they are not yet**, they are the next ones.

```
nlohmann JSON   3.11.3
spdlog          1.16.0
MQT-DDSIM       1.24.0
libzmq          4.3.5
cppzmq          4.11.0
CunqaSimulator  0.1.1
```

And, finally, **the ones that will be installed**.
```
argparse        -
qiskit-aer      0.17.2 (modified version)
``` 

### Configure, build and install
Now, as with any other CMake project, is can be installed using the usual directives. The CMAKE_INSTALL_PREFIX variable should be defined and, if not, its will be the HOME environment variable value. 

```console
cmake -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
cmake --build build/ --parallel $(nproc)
cmake --install build/
```

It is important to mention that the user can also employ [Ninja](https://ninja-build.org/) to perform this task.

```console
cmake -G Ninja -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
ninja -C build -j $(nproc)
cmake --install build/
```

Alternatively, you can use the `configure.sh` file, but only after all the dependencies have been solved.
```console
source configure.sh /your/installation/path
```

### Install as Lmod module
Cunqa is available as Lmod module in CESGA. To use it all you have to do is:

- In QMIO: `module load qmio/hpc gcc/12.3.0 cunqa/0.3.1-python-3.9.9-mpi`
- In FT3: `module load cesga/2022 gcc/system cunqa/0.3.1`

If your HPC center is interested in using it this way, EasyBuild files employed to install it in CESGA are available inside `easybuild/` folder.

## Uninstall
There has also been developed a Make directive to uninstall CUNQA if needed: 

1. If you installed using the standard way: `make uninstall`. 

2. If you installed using Ninja: `ninja uninstall`.

Be sure to execute this command inside the `build/` directory in both cases. An alternative is using:

```console
cmake --build build/ --target uninstall
``` 

to abstract from the installation method.

## Run your first distributed program

Once **CUNQA** is installed, the basic workflow to use it is:
1. Raise the desired QPUs with the command `qraise`.
2. Run circuits on the QPUs:
    - Connect to the QPUs through the python API.
    - Define the circuits to execute.
    - Execute the circuits on the QPUs.
    - Obtain the results.
3. Drop the raised QPUs with the command `qdrop`.

### 1. `qraise` command
The `qraise` command raises as many QPUs as desired. Each QPU can be configured by the user to have a personalized backend. There is a help FLAG with a quick guide of how this command works:
```console
qraise --help
```
1. The only two mandatory FLAGS of `qraise` are the **number of QPUs**, set up with `-n` or `--num_qpus` and the **maximum time** the QPUs will be raised, set up with `-t` or `--time`. 
So, for instance, the command 
```console 
qraise -n 4 -t 01:20:30
``` 
will raise four QPUs during at most 1 hour, 20 minutes and 30 seconds. The time format is `hh:mm:ss`.
> [!NOTE]  
> By default, all the QPUs will be raised with [AerSimulator](https://github.com/Qiskit/qiskit-aer) as the background simulator and IdealAer as the background backend. That is, a backend of 32 qubits, all connected and without noise.
2. The simulator and the backend configuration can be set by the user through `qraise` FLAGs:

**Set simulator:** 
```console
qraise -n 4 -t 01:20:30 --sim=Munich
```
The command above changes the default simulator by the [mqt-ddsim simulator](https://github.com/cda-tum/mqt-ddsim). Currently, **CUNQA** only allows two simulators: ``--sim=Aer`` and ``--sim=Munich``.

**Set FakeQmio:**
```console
qraise -n 4 -t 01:20:30 --fakeqmio=<path/to/calibrations/file>
```
The `--fakeqmio` FLAG raises the QPUs as simulated [QMIO](https://www.cesga.es/infraestructuras/cuantica/)s. If no `<path/to/calibrations/file>` is provided, last calibrations of de QMIO are used. With this FLAG, the background simulator is AerSimulator.

**Set personalized backend:**
```console
qraise -n 4 -t 01:20:30 --backend=<path/to/backend/json>
```
The personalized backend has to be a *json* file with the following structure:
```json
{"backend":{"name": "BackendExample", "version": "0.0", "n_qubits": 32,"url": "", "is_simulator": true, "conditional": true, "memory": true, "max_shots": 1000000, "description": "", "basis_gates": [], "custom_instructions": "", "gates": [], "coupling_map": []}, "noise": {}}
```
> [!NOTE]
> The "noise" key must be filled with a json with noise instructions supported by the chosen simulator.

> [!IMPORTANT]  
> Several `qraise` commands can be executed one after another to raise as many QPUs as desired, each one having its own configuration, independently of the previous ones. The `get_QPUs()` method presented in the section below will collect all the raised QPUs.

### 2. Python Program Example
Once the QPUs are raised, they are ready to execute any quantum circuit. The following script shows a basic workflow to run a circuit on a single QPU.

> [!WARNING]
> To execute the following python example it is needed  to load the [Qiskit](https://github.com/Qiskit/qiskit) module:

In QMIO:
```console 
module load qmio/hpc gcc/12.3.0 qiskit/1.2.4-python-3.9.9
```

In FT3:
```console 
module load cesga/2022 gcc/system qiskit/1.2.4
```


```python 
# Python Script Example

import os
import sys

# Adding pyhton folder path to detect modules
sys.path.append(os.getenv("HOME"))

# Let's get the raised QPUs
from cunqa.qpu import get_QPUs

qpus  = get_QPUs(local=False) # List of all raised QPUs
for q in qpus:
    print(f"QPU {q.id}, name: {q.backend.name}, backend: {q.backend.simulator}, version: {q.backend.version}.")

# Let's create a circuit to run in our QPUs
from qiskit import QuantumCircuit

N_QUBITS = 2 # Number of qubits
qc = QuantumCircuit(N_QUBITS)
qc.h(0)
qc.cx(0,1)
qc.measure_all()

# Time to run
qpu0 = qpus[0] # Select one of the raise QPUs

job = qpu0.run(qc, transpile = True, shots = 1000) # Run the transpiled circuit
result = job.result # Get the result of the execution
counts = result.counts # Get the counts

print(f"Counts: {counts}" ) # {'00':546, '11':454}
```

> [!NOTE] 
> It is not mandatory to run a *QuantumCircuit* from Qiskit. The `.run` method also supports *OpenQASM 2.0* with the following structure: 
```json
{"instructions":"OPENQASM 2.0;\ninclude \"qelib1.inc\";\nqreg q[2];\ncreg c[2];\nh q[0];\ncx q[0], q[1];\nmeasure q[0] -> c[0];\nmeasure q[1] -> c[1];" , "num_qubits": 2, "num_clbits": 4, "quantum_registers": {"q": [0, 1]}, "classical_registers": {"c": [0, 1], "other_measure_name": [2], "meas": [3]}}

```

and *json* format with the following structure: 
```json
{"instructions": [{"name": "h", "qubits": [0], "params": []},{"name": "cx", "qubits": [0, 1], "params": []}, {"name": "rx", "qubits": [0], "params": [0.39528385768119634]}, {"name": "measure", "qubits": [0], "memory": [0]}], "num_qubits": 2, "num_clbits": 4, "quantum_registers": {"q": [0, 1]}, "classical_registers": {"c": [0, 1], "other_measure_name": [2], "meas": [3]}}

```
### 3. `qdrop` command
Once the work is finished, the raised QPUs should be dropped in order to not monopolize computational resources. 

The `qdrop` command can be used to drop all the QPUs raised with a single `qraise` by passing the corresponding qraise `SLURM_JOB_ID`:
```console 
qdrop SLURM_JOB_ID
```
Note that the ```SLURM_JOB_ID``` can be obtained, for instance, executing the `squeue` command.

To drop all the raised QPUs, just execute:
```console 
qdrop --all
```

## Acknowledgements
This work has been mainly funded by the project QuantumSpain, financed by the Ministerio de Transformación Digital y Función Pública of Spain’s Government through the project call QUANTUM ENIA – Quantum Spain project, and by the European Union through the Plan de Recuperación, Transformación y Resiliencia – NextGenerationEU within the framework of the Agenda España Digital 2026. J. Vázquez-Pérez was supported by the Axencia Galega de Innovación (Xunta de Galicia) through the Programa de axudas á etapa predoutoral (ED481A & IN606A).

Additionally, this research project was made possible through the access granted by the Galician Supercomputing Center (CESGA) to two key parts of its infrastructure. Firstly, its Qmio quantum computing infrastructure with funding from the European Union, through the Operational Programme Galicia 2014-2020 of ERDF_REACT EU, as part of theEuropean Union’s response to the COVID-19 pandemic. 

Secondly, The supercomputer FinisTerrae III and its permanent data storage system, which have been funded by the NextGeneration EU 2021 Recovery, Transformation and Resilience Plan, ICT2021-006904, and also from the Pluriregional Operational Programme of Spain 2014-2020 of the European Regional Development Fund (ERDF), ICTS-2019-02-CESGA3, and from the State Programme for the Promotion of Scientific and Technical Research of Excellence of the State
Plan for Scientific and Technical Research and Innovation 2013-2016 State subprogramme for scientific and technical infrastructures and equipment of ERDF, CESG15-DE-3114.
