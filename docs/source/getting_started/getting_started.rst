Getting started
===============

Installation
------------

Clone repository
^^^^^^^^^^^^^^^^

It is important to say that, for ensuring a correct cloning of the repository, the SSH is the one 
preferred. In order to get this to work one has to do:

.. code-block:: console
   
   eval "$(ssh-agent -s)"
   ssh-add ~/.ssh/SSH_KEY

Now, everything is set to get the source code.

.. code-block:: console

   git clone git@github.com:CESGA-Quantum-Spain/cunqa.git


Define STORE environment variable
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before doing any kind of compilation, the user has to define the ``STORE`` environment variable in 
bash. This repository will be the root for the ``.cunqa`` folder, where CUNQA is going to store 
several runtime files (configuration files and logging files, mainly).

.. code-block:: console

   export STORE=/path/to/your/store


Dependencies
^^^^^^^^^^^^

CUNQA has a set of dependencies. They are divided in three main groups:

- Must be installed before configuration.
- Can be installed, but if they are not they will be by the configuration process.
- They will be installed by the configuration process.

From the first group, **the ones that must be installed**, the dependencies are the following. The 
versions here displayed are the ones that have been employed in the development and, therefore, that 
are recommended.

.. code-block:: text

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

From the second group, **the ones that will be installed if they are not yet**, they are the next 
ones.

.. code-block:: text

   nlohmann JSON   3.11.3
   spdlog          1.16.0
   MQT-DDSIM       1.24.0
   libzmq          4.3.5
   cppzmq          4.11.0
   CunqaSimulator  0.1.1

And, finally, **the ones that will be installed**.

.. code-block:: text

   argparse        -
   qiskit-aer      0.17.2 (modified version)


Configure, build and install
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Now, as with any other CMake project, it can be installed using the usual directives. The 
``CMAKE_INSTALL_PREFIX`` variable should be defined or will be the ``HOME`` environment variable 
value.

.. code-block:: console

   cmake -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
   cmake --build build/ --parallel $(nproc)
   cmake --install build/

It is important to mention that the user can also employ `Ninja <https://ninja-build.org/>`_ to 
perform this task.

.. code-block:: console

   cmake -G Ninja -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
   ninja -C build/ -j $(nproc)
   cmake --install build/

Alternatively, you can use the ``configure.sh`` file, but only after all the dependencies have been 
solved.

.. code-block:: console

   source configure.sh /your/installation/path


Install as Lmod module
^^^^^^^^^^^^^^^^^^^^^^

Cunqa is available as Lmod module in CESGA. To use it all you have to do is:

- In QMIO: ``module load qmio/hpc gcc/12.3.0 cunqa/0.3.1-python-3.9.9-mpi``
- In FT3: ``module load cesga/2022 gcc/system cunqa/0.3.1``

If your HPC center is interested in using it this way, EasyBuild files employed to install it in 
CESGA are available inside ``easybuild/`` folder.


Uninstall
---------

There has also been developed a Make directive to uninstall CUNQA if needed:

1. If you installed using the standard way: ``make uninstall``.
2. If you installed using Ninja: ``ninja uninstall``.

Be sure to execute this command inside the ``build/`` directory in both cases. An alternative is 
using:

.. code-block:: console

   cmake --build build/ --target uninstall

to abstract from the installation method.
