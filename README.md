
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
  A Distributed Quantum Computing emulator for HPC 
</p>

<br>

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

## Documentation
For a complete and exhaustive explanation and functionality showcase of CUNQA visit the [CUNQA documentation](https://cesga-quantum-spain.github.io/cunqa/).

<p align="center">
  <a href="https://cesga-quantum-spain.github.io/cunqa/">
  <img width=30% src="https://img.shields.io/badge/documentation-black?style=for-the-badge&logo=read%20the%20docs" alt="Documentation">
  </a>
</p>

# Table of contents
  - [INSTALL](#install)
    - [Clone repository](#clone-repository)
    - [Define STORE environment variable](#clone-repository)
    - [Dependencies](#dependencies)
    - [Configure, build and install](#configure-build-and-install)
    - [Install as Lmod module](#install-as-lmod-module)
  - [UNINSTALL](#uninstall)
  - [RUN YOUR FIRST DISTRIBUTED PROGRAM](#run-your-first-distributed-program)
    - [PYTHON-BASH](#python-bash)
    - [PYTHON-ONLY](#python-only)
  - [ACKNOWLEDGEMENTS](#acknowledgements)

## Install

### Clone repository

To get the source code, simply clone the CUNQA repository:

```bash
git clone git@github.com:CESGA-Quantum-Spain/cunqa.git
```

> [!WARNING]
> If SSH cloning fails, you may not have properly linked your environment to GitHub. To do this, run the following commands:
>
> ```bash
> eval "$(ssh-agent -s)"
> ssh-add ~/.ssh/SSH_KEY
> ```
>
> where *SSH_KEY* is the secure key that connects your environment with GitHub, usually stored in the `~/.ssh` folder.

---

### Define STORE environment variable

At build time, CUNQA will look at the `STORE` environment variable to set the root of the `.cunqa` folder where configuration and logging files will be stored. If it is **not** defined in your environment, run:

```bash
export STORE=/path/to/your/store
```

If you plan to compile CUNQA multiple times, we recommend adding this directive to your `.bashrc` file to avoid potential issues.

---

### Dependencies

CUNQA has a set of dependencies, as any other platform. The versions listed below are those used during development and are therefore recommended. They are divided into three main groups:

#### Must be installed by the user before configuration

```text
gcc             12.3.0
qiskit          1.2.4
CMake           3.21.0
python          3.9 (recommended 3.11)
pybind11        2.7 (recommended 2.12)
MPI             3.1
OpenMP          4.5
Boost           1.85.0
Eigen           5.0.0
Blas            -
Lapack          -
```

#### Can be installed by the user (otherwise installed automatically during configuration)

```text
nlohmann JSON   3.11.3
spdlog          1.16.0
MQT-DDSIM       1.24.0
libzmq          4.3.5
cppzmq          4.11.0
CunqaSimulator  0.1.1
```

#### Installed automatically during configuration

```text
argparse        -
qiskit-aer      0.17.2 (modified version)
```

---

### Configure, build and install

To build, compile, and install CUNQA, follow the standard three-step CMake workflow:

```bash
cmake -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
cmake --build build/ --parallel $(nproc)
cmake --install build/
```

> [!WARNING]
> If `CMAKE_PREFIX_INSTALL` is not provided, CUNQA will be installed in the directory pointed to by 
the `HOME` environment variable.

> [!NOTE] 
> To enable GPU execution provided by AerSimulator, add the `-DAER_GPU=TRUE` flag at build time:
>
> ```bash
> cmake -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path -DAER_GPU=TRUE
> ```

You can also use [Ninja](https://ninja-build.org/) to perform this task:

```bash
cmake -G Ninja -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
ninja -C build/ -j $(nproc)
cmake --install build/
```

Alternatively, you can use the `configure.sh` script, but only after all dependencies have been 
resolved:

```bash
source configure.sh /your/installation/path
```

---

### Install as Lmod module

CUNQA is available as an Lmod module at CESGA. To use it:

- In QMIO:
  ```bash
  module load qmio/hpc gcc/12.3.0 cunqa/0.3.1-python-3.9.9-mpi
  ```

- In FT3:
  ```bash
  module load cesga/2022 gcc/system cunqa/0.3.1
  ```

If your HPC center is interested in deploying it this way, the EasyBuild files used at CESGA are 
available in the `easybuild/` folder.

---

### Uninstall

A Make directive is available to uninstall CUNQA if needed:

1. If installed using the standard method:
   ```bash
   make uninstall
   ```

2. If installed using Ninja:
   ```bash
   ninja uninstall
   ```

Be sure to execute these commands inside the `build/` directory in both cases.

Alternatively, you can run:

```bash
cmake --build build/ --target uninstall
```

to abstract from the installation method.

## Run your first distributed program

To achieve this, you have two options: a Python–Bash workflow or a Python-only workflow. With the 
first option, virtual QPUs (vQPUs) can be deployed from the terminal and the employed in the Python 
executable, while the second allows to deploy, use, and drop the vQPUs within the Python program.

### Python-Bash

To deploy the vQPUs we use the `qraise` Bash command.

```bash
qraise -n 4 -t 01:00:00 --co-located
```

Once the vQPUs are deployed, we can design and execute quantum tasks:

```python
import os, sys

# Adding path to access CUNQA module
sys.path.append(os.getenv("</your/cunqa/installation/path>"))

# Gettting the raised QPUs
from cunqa.qpu import get_QPUs

qpus  = get_QPUs(co_located=True)

# Creating a circuit to run in our QPUs
from cunqa.circuit import CunqaCircuit

qc = CunqaCircuit(num_qubits = 2)
qc.h(0)
qc.cx(0,1)
qc.measure_all()

# Submitting the same circuit to all vQPUs
from cunqa.qpu import run

qcs = [qc] * 4
qjobs = run(qcs , qpus, shots = 1000)

# Gathering results
from cunqa.qjob import gather

results = gather(qjobs)

# Getting and printing the counts
counts_list = [result.counts for result in results]

for counts in counts_list:
    print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}
```

It is a good practice to relinquish resources when the work is done. This is achieved by the `qdrop` 
command:

```bash
qdrop --all
```


### Python-only

Here, the `qraise` and `qdrop` steps are integrated into the Python executable.

```python
import os, sys

# Adding path to access CUNQA module
sys.path.append(os.getenv("</your/cunqa/installation/path>"))

# Raising the QPUs
from cunqa.qpu import qraise

family = qraise(2, "00:10:00", simulator="Aer", co_located=True)

# Gettting the raised QPUs
from cunqa.qpu import get_QPUs

qpus  = get_QPUs(co_located=True)

# Creating a circuit to run in our QPUs
from cunqa.circuit import CunqaCircuit

qc = CunqaCircuit(num_qubits = 2)
qc.h(0)
qc.cx(0,1)
qc.measure_all()

# Submitting the same circuit to all vQPUs
from cunqa.qpu import run

qcs = [qc] * 4
qjobs = run(qcs , qpus, shots = 1000)

# Gathering results
from cunqa.qjob import gather

results = gather(qjobs)

# Getting and printing the counts
counts_list = [result.counts for result in results]

for counts in counts_list:
    print(f"Counts: {counts}" ) # Format: {'00':546, '11':454}

# Relinquishing the resources
from cunqa.qpu import qdrop

qdrop(family)
```


## Acknowledgements
This work has been mainly funded by the project QuantumSpain, financed by the Ministerio de 
Transformación Digital y Función Pública of Spain’s Government through the project call 
QUANTUM ENIA – Quantum Spain project, and by the European Union through the Plan de Recuperación, 
Transformación y Resiliencia – NextGenerationEU within the framework of the Agenda España 
Digital 2026. J. Vázquez-Pérez was supported by the Axencia Galega de Innovación (Xunta de Galicia) 
through the Programa de axudas á etapa predoutoral (ED481A & IN606A).

Additionally, this research project was made possible through the access granted by the Galician 
Supercomputing Center (CESGA) to two key parts of its infrastructure. Firstly, its Qmio quantum 
computing infrastructure with funding from the European Union, through the Operational Programme 
Galicia 2014-2020 of ERDF_REACT EU, as part of theEuropean Union’s response to the COVID-19 
pandemic. 

Secondly, The supercomputer FinisTerrae III and its permanent data storage system, which have been 
funded by the NextGeneration EU 2021 Recovery, Transformation and Resilience Plan, ICT2021-006904, 
and also from the Pluriregional Operational Programme of Spain 2014-2020 of the European Regional 
Development Fund (ERDF), ICTS-2019-02-CESGA3, and from the State Programme for the Promotion of 
Scientific and Technical Research of Excellence of the State Plan for Scientific and Technical 
Research and Innovation 2013-2016 State subprogramme for scientific and technical infrastructures 
and equipment of ERDF, CESG15-DE-3114.

## How to cite:

When citing the software, please cite the original CUNQA paper:

```bibtex
@misc{vázquezpérez2025cunqadistributedquantumcomputing,
    title={CUNQA: a Distributed Quantum Computing emulator for HPC}, 
    author={
      Jorge Vázquez-Pérez and 
      Daniel Expósito-Patiño and 
      Marta Losada and 
      Álvaro Carballido and 
      Andrés Gómez and 
      Tomás F. Pena},
    year={2025},
    eprint={2511.05209},
    archivePrefix={arXiv},
    primaryClass={quant-ph},
    url={https://arxiv.org/abs/2511.05209}, 
}
```
