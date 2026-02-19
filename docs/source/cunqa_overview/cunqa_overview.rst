CUNQA overview
===============

This section presents an overview of CUNQA, an emulator of Distributed Quantum Computing (DQC) 
architectures on HPC environments. Each of the architectures (or schemes) is built upon virtual 
QPUs, their basic building blocks.

.. _sec_virtual_qpu:

Virtual QPU
------------

A **virtual QPU (vQPU)** is a classical process running on a HPC environment with an allocated set of 
classical resources responsible of simulating the behaviour of a real QPU. They are composed by two 
parts:

- **Server**: manages communication user-vQPU.
- **Simulator**: performs the actual execution.  Currently, the following simulators are 
  available:

   - `AerSimulator <https://github.com/Qiskit/qiskit-aer/>`_
   - `MQT-DDSIM <https://github.com/munich-quantum-toolkit/ddsim>`_
   - `Qulacs <https://github.com/qulacs/qulacs>`_
   - `Maestro <https://github.com/QoroQuantum/maestro>`_
   - `CunqaSimulator <https://github.com/CESGA-Quantum-Spain/cunqasimulator>`_

The modular structure of CUNQA allows the implementation of other simulators on demand.

.. dropdown:: How to deploy a vQPU?

  The deployment of vQPUs is made through the bash command :doc:`../reference/commands/qraise` or 
  through the Python function :py:func:`~cunqa.qpu.qraise`. Depending on the desired vQPU type, 
  different argumets must be provided to the command or to the function, depending on the one being 
  used. These arguments will be explored inside the description of each of the :ref:`sec_dqc_schemes` 
  below.

.. dropdown:: GPU support

  We support the **GPU** execution provided by AerSimulator. This must be enabled at 
  compile time as discussed in the :doc:`../installation/getting_started` section.

.. dropdown:: Noisy simulations

  CUNQA allows the simulation of quantum circuits using a noise model, but **only with the 
  no-communication model and with the AER simulator**. Adding it to the other communication models is 
  considered part of the future improvements. In order to do this, the vQPUs have to be deployed with 
  a valid noise model scheme. This is done with the aforementioned :doc:`../reference/commands/qraise` 
  Bash command or its Python function counterpart :py:func:`~cunqa.qpu.qraise`, with the first 
  accepting the flag ``noise-properties`` and the second the argument ``noise_path``; both being the path 
  to a noise properties JSON file. The format of this JSON file is shown in 
  :doc:`../further_examples/json_examples/noise_properties_example`.

  Additionally, a vQPU with the noise model of `CESGA's QMIO quantum computer <https://www.cesga.es/infraestructuras/cuantica/>`_ 
  can be deployed, but only if CUNQA is being executed inside the CESGA's infrastructure. This can be 
  done by employing the ``fakeqmio`` flag in the :doc:`../reference/commands/qraise` Bash command or 
  with ``fakeqmio`` argument in the :py:func:`~cunqa.qpu.qraise` Python function.

.. _sec_quantum_circs:

Quantum circuits
-----------------
As far as our knowledge extends, none of the most commonly used quantum circuit creation interfaces 
support the vQPU intercommunication instructions that we need to interact with CUNQA. Therefore, 
:py:class:`~cunqa.circuit.core.CunqaCircuit` was implemented as the basic tool to define 
distributed circuits. Its communication instructions will be explored in detail in their 
corresponding :ref:`sec_dqc_schemes` section below.

.. note:: 
   Apart from :py:class:`~cunqa.circuit.core.CunqaCircuit`, Qiskit QuantumCircuit and raw json 
   instructions (see :doc:`../further_examples/json_examples/circuit_json_example`) are supported 
   as circuit representations.

.. _sec_dqc_schemes:

DQC schemes
------------

As a DQC emulator, CUNQA supports the three basic DQC schemes:

- :doc:`embarrassingly_parallel`: classical distribution of quantum tasks with no communications 
  at all.
- :doc:`classical_comm`: interchange classical bits between vQPUs at execution time.
- :doc:`quantum_comm`: implementation of teledata and telegate protocols.

Each of the previous sections will show:

1. How to deploy an infrastructure with the corresponding schema.
2. How to create and design circuits that fit that schema.
3. How to execute the circuits in the infrastructure.
4. A simple example.

.. toctree::
   :maxdepth: 1
   :hidden:

      Embarrassingly parallel <embarrassingly_parallel.rst>
      Classical communications <classical_comm.rst>
      Quantum communications <quantum_comm.rst>

.. _sec_tools_for_dqc_algorithms:


Tools for the study of algorithms: DQC and VQAs 
------------------------------------------------
Several circuit manipulation techniques have been implemented in CUNQA to empower the study of DQC 
algorithms. In particular, the following functions are available for building circuits from smaller 
pieces and for dividing circuits into subcircuits:

.. dropdown:: DQC: ``union``, ``add`` and ``hsplit``

  - :py:func:`~cunqa.circuit.transformations.union`: combine circuits to produce another circuit with 
    a larger set of qubits. For instance, given two circuits with `n` and `m` qubits, a circuit with 
    `n+m` qubits with the corresponding instructions on each register would be obtained.
  - :py:func:`~cunqa.circuit.transformations.add`: sum two circuits to obtain a deeper circuit which 
    executes the instructions of the first summand and then those of the second summand.
  - :py:func:`~cunqa.circuit.transformations.hsplit`: divide the set of qubits of a circuit into 
    subcircuits. For instance, given a `n+m` qubit circuit, two circuits with `n` and `m` qubits 
    preserving the instructions would be obtained.

  .. note::
    The function :py:func:`~cunqa.circuit.transformations.union` replaces distributed instructions 
    between the circuits for local ones, while :py:func:`~cunqa.circuit.transformations.hsplit` 
    replaces local 2-qubit operations that involve different subcircuits into distributed 
    instructions. Indeed, :py:func:`~cunqa.circuit.transformations.union` is the **inverse** of 
    :py:func:`~cunqa.circuit.transformations.hsplit`.


  These functions facilitate the inquiry into DQC algorithms as they can transform a set of 
  communicated circuits into a single monolithic circuit (``union``), and conversely, split a circuit 
  implementing a monolithic algorithm into several circuits to run in communicated QPUs (``hsplit``).

  Check :doc:`tools_for_DQC` for detailed examples of these functions.

  .. toctree::
    :maxdepth: 1
    :hidden:

        add, union and hsplit <tools_for_DQC.rst>

Additionally, CUNQA provides convenient ways of implementing Variational Quantum Algorithms.

.. dropdown:: VQAs: parameters and ``upgrade_parameters``

  Variational Quantum Algorithms require vast amounts of circuit evaluations for their optimization 
  processes, where the circuit structure remains constant and solely the parameters of certain gates
  change between evaluations. CUNQA facilitates working with these type of template circuits by its 
  support for Parameters. 

  Parameters are placeholders for the values that will be inserted on parametric gates at each evaluation. 
  They can be single variables, say `x` or expressions with multpile variables like `cos(2*x) + exp(z/2)`.

  Parameters are inserted as a string in parametric gates to mark that its value will vary, and their value
  is given when executing. After running a parametric circuit, new parameters can be given for another 
  evaluation using :py:meth:`~cunqa.qjob.QJob.upgrade_parameters`. If a value is not given to a 
  certain variable, it will retain the value from last evaluation.

  .. code-block:: python
    
    circuit.rx(param="x", qubit=0)
    # Parameters are given values when running
    qjob = run(circuit, qpu, param_values={"x": np.pi}, shots= 1024)
    result = qjob.result

    # For another execution with new parameters, use QJob.upgrade_parameters()
    new_result = qjob.upgrade_parameters({"x": 0}).result

  Check :doc:`tools_for_VQAs` for a detailed example of the use of parameters.

  .. toctree::
    :maxdepth: 1
    :hidden:

        Parameters and upgrade_parameters <tools_for_VQAs.rst>

.. _sec_real_qpus:

Real QPUs
---------
CUNQA also allows working with real quantum hardware. In particular, the 
`CESGA's QMIO quantum computer <https://www.cesga.es/infraestructuras/cuantica/>`_ can be 
*deployed* alongside vQPUs to execute quantum tasks in a truly hybrid DQC infrastructure. 
Check :doc:`../reference/commands/qraise` to see how to deploy the real QPU.

CUNQA is constructed with the idea of, in the future, supporting real QPUs in a similar manner as
vQPUs are nowadays. 