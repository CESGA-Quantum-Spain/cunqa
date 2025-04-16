.. _installation-label:
Module Set Up
*****************



Cloning the repository 
========================
GitHub provides different options for cloning a repository, which can be checked in the dropdown menu of the green "<> Code" buttom. 
For ensuring a correct cloning of the repository, the SSH is the one preferred. This can be achieved by running the following on your terminal: ::

    eval "$(ssh-agent -s)"
    ssh-add ~/.ssh/SSH_KEY
We are set to retrieve the source code: ::

    git clone --recursive git@github.com:CESGA-Quantum-Spain/cunqa.git

The ``--recursive`` option ensures that all submodules are correctly loaded when cloning. As a additional step, we encourage to run the setup_submodules.sh file, which removes some of the files unused in the submodules and makes the repository lighter: ::

    cd cunqa/scripts
    bash setup_submodules.sh


Installation
======================

QMIO
--------

Automatic installation
^^^^^^^^^^^^^^^^^^^^^^^^
The ``scripts/configure.sh`` file is prepared to bring an automatic installation of the **CUNQA** platform. The user only has to execute this file followed by the path to the desire installation folder: ::

    source configure.sh <your/installation/path>
 
If the automatic installation fails, try the manual installation.

Manual installation
^^^^^^^^^^^^^^^^^^^^
1. As a cautionary measure, deactivate miniconda to not interfere with the rest of the installation: ::

    conda deactivate

2. Then, load the following modules: ::

    ml load qmio/hpc gcc/12.3.0 hpcx-ompi flexiblas/3.3.0 boost cmake/3.27.6 pybind11/2.12.0-python-3.9.9 nlohmann_json/3.11.3 ninja/1.9.0 qiskit/1.2.4-python-3.9.9

3. Define the enviroment variable ``INSTALL_PATH`` as the **ABSOLUTE PATH** to the directory where **CUNQA** is to be installed. ::

    export INSTALL_PATH=<your/installation/path>

4. Afterwards, add the bin folder to ``PATH`` in order to correctly execute all the commands to use the platform. ::

    export PATH=$PATH:$INSTALL_PATH/bin

5. Once the previous steps are done, everything is set for the build/installation. There are two options:

    - **Standard way (slower)** ::
        
        cmake -B build/ 
        cmake --build build/
        cmake --install build/

    - **Using** `Ninja <https://ninja-build.org/>`_ **(faster)** ::

        cmake -G Ninja -B build/
        ninja -C build -j $(nproc)
        cmake --install build/

FINISTERRAE
-------------
In the FT3, the installation is almost the same as in QMIO but with few exceptions.

Automatic installation
^^^^^^^^^^^^^^^^^^^^^^^^
For the **automatic installation**, the process is exactly the same as the one presented for QMIO: ::

    source configure.sh <your/installation/path>
 
Manual installation
^^^^^^^^^^^^^^^^^^^^
In the case of a **manual installation**, the steps 1-4 are analogous to the shown above for QMIO:

1. Conda deactivation: ::

    conda deactivate

2. Loading modules: ::

    ml load cesga/2022 gcc/system flexiblas/3.3.0 openmpi/5.0.5 boost pybind11 cmake qiskit/1.2.4

3. INSTALL_PATH: ::

    export INSTALL_PATH=<your/installation/path>

4. Bin PATH: ::

    export PATH=$PATH:$INSTALL_PATH/bin

5. Instead of a simple ``cmake -B build/`` as in QMIO, the user has to add the ``-DPYBIND_DIR`` option with the path to the pybind11 cmake modules:


    - **Standard way (slower)** ::
        
        cmake -B build/ -DPYBIND_PATH=/opt/cesga/2022/software/Compiler/gcccore/system/pybind11/2.12.0/lib64/python3.9/site-packages/pybind11
        cmake --build build/
        cmake --install build/

    - **Using** `Ninja <https://ninja-build.org/>`_ **(faster)** ::

        cmake -G Ninja -B build/ -DPYBIND_PATH=/opt/cesga/2022/software/Compiler/gcccore/system/pybind11/2.12.0/lib64/python3.9/site-packages/pybind11
        ninja -C build -j $(nproc)
        cmake --install build/

And that's it! Everything is set—either on QMIO or in the FT3—to perform an execution. Next we'll learn how to run a distributed program by means of a minimal example.

