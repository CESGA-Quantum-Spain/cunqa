Overview
========

This section presents an overview of CUNQA, an emulator of :term:`Distributed Quantum Computing`
(:term:`DQC`) architectures on HPC environments. Each of the architectures (or schemes) is built upon virtual 
:term:`QPUs <QPU>`, their basic building blocks.

.. _sec_virtual_qpu:

Virtual QPU
------------

A **virtual QPU (vQPU)** is a classical process running on a HPC environment with an allocated set of 
classical resources responsible of simulating the behaviour of a real :term:`QPU`. They are composed by two 
parts:

- **Server**: manages communication user-:term:`vQPU`.
- **Simulator**: performs the actual execution. Several :term:`simulators <simulator>` are supported, each selected at
  deployment time through the ``--simulator`` flag. The full list of supported :term:`simulators <simulator>`, the value
  to pass to ``--simulator`` to select each one, and their feature support (communication schemes,
  noise and GPU) is given in the :doc:`../reference/simulators` reference page.

The modular structure of CUNQA allows the implementation of other :term:`simulators <simulator>` on demand; see
:doc:`../development/adding_a_simulator`.

.. dropdown:: How to deploy a vQPU?

    The deployment of :term:`vQPUs <vQPU>` is made through the bash command :doc:`../reference/commands/qraise` or 
    through the Python function :py:func:`~cunqa.qpu.qraise`. Depending on the desired :term:`vQPU` type, 
    different argumets must be provided to the command or to the function, depending on the one being 
    used. These arguments will be explored inside the description of each of the :ref:`sec_dqc_schemes` 
    below.

.. dropdown:: GPU support

    We support the **GPU** execution provided by AerSimulator. This must be enabled at 
    compile time as discussed in the :doc:`../installation/getting_started` section.

.. dropdown:: Noisy simulations

    CUNQA allows the simulation of quantum circuits using a noise model, but **only with the 
    no-communication model and with the AER simulator**. Adding it to the other communication models is
    considered part of the future improvements. In order to do this, the :term:`vQPUs <vQPU>` have to be deployed with 
    a valid noise model scheme. This is done with the aforementioned :doc:`../reference/commands/qraise`
    Bash command or its Python function counterpart :py:func:`~cunqa.qpu.qraise`, with the first
    accepting the flag ``--backend`` and the second the argument ``backend``; both being the path
    to a :term:`backend` configuration JSON file that, in turn, references a noise properties JSON file. The
    format of these files is shown in
    :doc:`../further_examples/json_examples/backend_json_example` and
    :doc:`../further_examples/json_examples/noise_properties_example`.

.. _sec_quantum_circs:

Quantum circuits
-----------------
As far as our knowledge extends, none of the most commonly used quantum circuit creation interfaces 
support the :term:`vQPU` intercommunication instructions that we need to interact with CUNQA. Therefore, 
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

As a :term:`DQC` emulator, CUNQA supports the three basic :term:`DQC` schemes:

- :doc:`embarrassingly_parallel`: classical distribution of quantum tasks with no communications 
  at all.
- :doc:`classical_comm`: interchange classical bits between :term:`vQPUs <vQPU>` at execution time.
- :doc:`quantum_comm`: implementation of :term:`teledata` and :term:`telegate` protocols.

Each of the previous sections will show:

1. How to **deploy an infrastructure** with the corresponding schema.
2. How to **create and design circuits** that fit that schema.
3. How to **execute the circuits** in the infrastructure.
4. How to **program a simple example**.

.. toctree::
    :maxdepth: 1
    :hidden:

        Embarrassingly parallel <embarrassingly_parallel.rst>
        Classical communications <classical_comm.rst>
        Quantum communications <quantum_comm.rst>

.. _sec_real_qpus:

Real QPUs
---------
CUNQA also allows working with real quantum hardware. In particular, the 
`CESGA's QMIO quantum computer <https://www.cesga.es/infraestructuras/cuantica/>`_ can be 
*deployed* alongside :term:`vQPUs <vQPU>` to execute quantum tasks in a truly hybrid :term:`DQC` infrastructure. 
Check :doc:`../reference/commands/qraise` to see how to deploy the real :term:`QPU`.

CUNQA is constructed with the idea of, in the future, supporting real :term:`QPUs <QPU>` in a similar manner as
:term:`vQPUs <vQPU>` are nowadays. 
