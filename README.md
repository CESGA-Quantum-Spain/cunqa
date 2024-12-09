# API-simulator
API definition for communicating the user with the simulator.

# Clone repository
In order to get all the submodules correctly loaded do:

```console
git clone --recursive git@github.com:CESGA-Quantum-Spain/api-simulator.git
cd api-simulator/scripts/
bash setup_submodules.sh
```

# C++ Dependecies
TO BE EXPLICITED

# Installation 
First of all, some modules have to be loaded and, also, if miniconda is activate, deactivate it to not interfere.

- In the FT3, load the modules:
```console
ml load cesga/2022 gcc/system gcccore/system openmpi/4.1.4 flexiblas/3.3.0 boost python/3.10.8 pybind11/2.12.0 cmake/3.27.6
conda deactivate
```
- In the QMIO, **TO BE DETERMINED**.

Now, using the environment path `INSTALL_PATH`, define the directory that you want to install to. **Use absolute paths for a consistent usage**. 

```console
export INSTALL_PATH=<your/installation/path>
```

Once your `INSTALL_PATH` variable has been set, export the bin folder to PATH in order to execute correctly the `qraise` and, inside of it, the `setup-qpus`.

```console
export PATH=$PATH:$INSTALL_PATH/bin
```

Now everything is set for the installation. 

```console
cmake -B build/
cmake --build build/
cmake --install build/
```

Alternatively, one can use the `configure.sh` script in the `scripts/` folder.

```console
source configure.sh <your/installation/path>
``` 
