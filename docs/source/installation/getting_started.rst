.. _sec_installation:

Installation
=============

.. role:: large
   :class: large-text

Clone repository
^^^^^^^^^^^^^^^^

To get the source code, simply clone CUNQA repository:

.. code-block:: console

   git clone git@github.com:CESGA-Quantum-Spain/cunqa.git

.. warning::
   If SSH cloning fails, you may not have properly linked your environment to GitHub. To do this, 
   run the following commands:

   .. code-block:: console
      
      eval "$(ssh-agent -s)"
      ssh-add ~/.ssh/SSH_KEY

   where *SSH_KEY* is the secure key that connects your environment with GitHub, usually stored in 
   the *~/.ssh* folder.

Now CUNQA must be built and installed. If you are installing CUNQA in an HPC center other than CESGA,
you might need to solve some dependencies or manually define the installation path. If you are installing 
it in CESGA, some steps can be skipped.

.. tab:: Generic HPC center

   .. rubric:: Define STORE environment variable

   At build time, CUNQA will look at the ``STORE`` environment variable to set the root of the 
   ``.cunqa`` folder where configuration files and logging files will be stored. So, if it is **not** 
   defined by default on the environment, just run:

   .. code-block:: console

      export STORE=/path/to/your/store

   If you plan to compile CUNQA multiple times, we recommend adding this directive to your ``.bashrc`` 
   file to avoid potential issues.

   .. rubric:: Dependencies

   CUNQA has a set of dependencies, as any other platform. The versions here displayed are the ones 
   that have been employed in the development and, therefore, that are recommended. They are divided 
   in three main groups:

   - **Must be installed by the user** before configuration.

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


   - **Can be installed by the user**, but if they are not they will be automatically installed by the 
   configuration process.

   .. code-block:: text

      nlohmann JSON   3.11.3
      spdlog          1.16.0
      MQT-DDSIM       1.24.0
      libzmq          4.3.5
      cppzmq          4.11.0
      CunqaSimulator  0.1.1

   - **Will be installed automatically** by the configuration process.

   .. code-block:: text

      argparse        -
      qiskit-aer      0.17.2 (modified version)


   .. rubric:: Configure, build and install

   To build, compile, and install CUNQA, the three usual steps in a CMake project are followed.

   .. code-block:: console

      cmake -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
      cmake --build build/ --parallel $(nproc)
      cmake --install build/

   .. warning::

      If ``CMAKE_PREFIX_INSTALL`` is not provided, CUNQA will be installed where the environment variable ``HOME`` points.

   You can also employ `Ninja <https://ninja-build.org/>`_ to perform this task.

   .. code-block:: console

      cmake -G Ninja -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
      ninja -C build/ -j $(nproc)
      cmake --install build/

   Alternatively, you can use the ``configure.sh`` file, but only after all the dependencies have been 
   solved.

   .. code-block:: console

      source configure.sh /your/installation/path

.. tab:: CESGA

   .. rubric:: Configure, build and install

   To build, compile, and install CUNQA, the three usual steps in a CMake project are followed.

   .. code-block:: console

      cmake -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
      cmake --build build/ --parallel $(nproc)
      cmake --install build/

   .. warning::

      If ``CMAKE_PREFIX_INSTALL`` is not provided, CUNQA will be installed where the environment variable ``HOME`` points.

   .. note::   

      To enable the GPU execution provided by AerSimulator, the flag ``-DAER_GPU=TRUE`` must be provided at build time: 

      .. code-block:: console

         cmake -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path -DAER_GPU=TRUE


   You can also employ `Ninja <https://ninja-build.org/>`_ to perform this task.

   .. code-block:: console

      cmake -G Ninja -B build/ -DCMAKE_PREFIX_INSTALL=/your/installation/path
      ninja -C build/ -j $(nproc)
      cmake --install build/

   Alternatively, you can use the ``configure.sh`` file.

   .. code-block:: console

      source configure.sh /your/installation/path

Install as Lmod module
^^^^^^^^^^^^^^^^^^^^^^

CUNQA is available as Lmod module in CESGA. To use it all you have to do is:

- In QMIO:

.. code-block:: console

   module load qmio/hpc gcc/12.3.0 cunqa/2.0.0-python-3.9.9-mpi

- In FT3:

.. code-block:: console

   module load cesga/2022 gcc/system cunqa/2.0.0 # without GPUs
   module load cesga/2022 gcc/system cunqa/2.0.0-cuda-12.8.0 # with GPUs

If your HPC center is interested in using it this way, EasyBuild files employed to install it in 
CESGA are available inside ``easybuild/`` folder.


Uninstall
^^^^^^^^^

There has also been developed a Make directive to uninstall CUNQA if needed:

1. If you installed using the standard way: ``make uninstall``.
2. If you installed using Ninja: ``ninja uninstall``.

Be sure to execute this command inside the ``build/`` directory in both cases. An alternative is 
using:

.. code-block:: console

   cmake --build build/ --target uninstall

to abstract from the installation method.
