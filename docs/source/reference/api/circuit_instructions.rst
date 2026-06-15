CunqaCircuit instructions
=========================

This page is a curated, categorized overview of the instructions offered by
:py:class:`~cunqa.circuit.core.CunqaCircuit`. It is meant as an entry point; the **complete and
authoritative** list of methods (with full signatures) is generated from the source in the
:doc:`cunqa.circuit` API reference.

Unless stated otherwise, the ``param``/``theta`` argument of parametric gates accepts a number or a
parameter string (a placeholder whose value is supplied at execution time; see
:doc:`upgrade_parameters`). Methods taking ``*qubits`` receive the involved qubit
indices in order (controls first, target last).

Construction and registers
--------------------------

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Method / property
     - Description
   * - ``CunqaCircuit(num_qubits, num_clbits, id=...)``
     - Create a circuit. ``num_qubits`` may be an ``int`` (:term:`data qubits <data qubit>` only) or a tuple
       ``(num_data_qubits, num_comm_qubits)`` to reserve :term:`communication qubits <comm qubit>`.
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.get_qubits`
     - Returns the ``(data_qubits, comm_qubits)`` index lists.
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.add_comm_qubits`
     - Reserve additional :term:`communication qubits <communication qubit>`.
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.add_data_register` / ``add_cl_register``
     - Add a named quantum / classical register.
   * - ``id`` / ``num_qubits`` / ``num_clbits`` / ``info``
     - Properties exposing the circuit identifier, sizes and the serialized circuit.

Single-qubit gates (no parameters)
----------------------------------

``i``, ``x``, ``y``, ``z``, ``h``, ``s``, ``sdg``, ``sx``, ``sxdg``, ``t``, ``tdg``, ``v``, ``vdg``
and other fixed single-qubit gates. All take a single ``qubit`` index, e.g.:

.. code-block:: python

   circuit.h(0)
   circuit.x(1)

Single-qubit gates (parametric)
-------------------------------

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Method
     - Description
   * - ``rx`` / ``ry`` / ``rz``
     - Rotations about the X / Y / Z axis: ``rx(theta, qubit)``.
   * - ``p`` / ``u1``
     - Phase gate: ``p(theta, qubit)``.
   * - ``r`` / ``u2``
     - Two-parameter single-qubit gates: ``u2(theta, phi, qubit)``.
   * - ``u3`` / ``u``
     - General single-qubit gate: ``u(theta, phi, lam, qubit)``.
   * - ``raxis``
     - Rotation about an arbitrary axis: ``raxis(theta, axis, qubit)``.

Two-qubit gates
---------------

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Method
     - Description
   * - ``cx`` / ``cy`` / ``cz`` / ``ch``
     - Controlled Pauli / Hadamard: ``cx(control, target)``.
   * - ``swap`` / ``iswap`` / ``sqrtswap`` / ``dcx`` / ``ecr``
     - Two-qubit non-controlled gates: ``swap(q0, q1)``.
   * - ``crx`` / ``cry`` / ``crz`` / ``cp`` / ``cu1``
     - Controlled rotations / phase: ``crz(theta, control, target)``.
   * - ``rxx`` / ``ryy`` / ``rzz`` / ``rzx`` / ``rxy``
     - Two-qubit interaction rotations: ``rzz(theta, q0, q1)``.
   * - ``cu`` / ``cu2`` / ``cu3`` / ``fs`` / ``xxpyy`` / ``xxmyy``
     - Other parametric two-qubit gates.

Multi-qubit and controlled gates
--------------------------------

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Method
     - Description
   * - ``ccx`` / ``ccy`` / ``ccz`` / ``cswap``
     - Three-qubit gates (Toffoli, Fredkin, ...).
   * - ``mcx`` / ``mcy`` / ``mcz`` / ``mch`` / ``mcswap`` ...
     - Multi-controlled versions; controls first, target last.
   * - ``mcrx`` / ``mcry`` / ``mcrz`` / ``mcp`` / ``mcu`` ...
     - Multi-controlled parametric gates.
   * - ``mcmx`` / ``mcraxis`` / ``mcstatex``
     - Multi-controlled gates with explicit control count / axis / control states.

Pauli strings and gadgets
-------------------------

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Method
     - Description
   * - ``paulistr`` / ``cpaulistr`` / ``mcpaulistr``
     - Apply a Pauli string (e.g. ``"XYZ"``) to the given qubits.
   * - ``pauligadget`` / ``cpauligadget`` / ``mcpauligadget`` / ``nonunitarypauligadget``
     - Pauli exponential ("gadget") rotations.
   * - ``phasegadget`` / ``cphasegadget`` / ``mcphasegadget``
     - Phase gadget instructions.
   * - ``multipauli`` / ``multipaulirotation``
     - Multi-qubit Pauli operators and rotations.

Matrix and special instructions
-------------------------------

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Method
     - Description
   * - ``unitary`` / ``cunitary``
     - Apply an arbitrary (controlled) unitary matrix.
   * - ``sparsematrix`` / ``diagonal``
     - Apply a sparse or diagonal matrix.
   * - ``randomunitary``
     - Apply a random unitary (optional ``seed``).
   * - ``fusedswap``
     - Block-wise fused swap.

Noise instructions
------------------

Circuit-level noise channels (only meaningful on noise-capable :term:`simulators <simulator>`, see
:doc:`../simulators`): ``amplitudedampingnoise``, ``bitflipnoise``, ``dephasingnoise``,
``depolarizingnoise``, ``independentxznoise``, ``twoqubitdepolarizingnoise``. Each takes a
probability and the target qubit(s), e.g. ``circuit.depolarizingnoise(0.01, 0)``.

Measurement and state
---------------------

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Method
     - Description
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.measure`
     - ``measure(qubits, clbits, save=True)`` — measure qubit(s) into classical bit(s).
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.measure_all`
     - Measure all :term:`data qubits <data qubit>` into a new ``"measure"`` register.
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.reset`
     - Reset qubit(s) to :math:`|0\rangle` (use after a measurement).
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.save_state`
     - Save the simulation state (statevector / density matrix) into the result.

Classical control
-----------------

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Method
     - Description
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.cif`
     - Open a classically conditioned block: ``with circuit.cif(clbits, condition=1, operation="and")
       as sub:``. Supported operations: ``and``, ``or``, ``xor``, ``nand``, ``nor``, ``xnor``.
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.endcif`
     - Close a ``cif`` block (handled automatically by the context manager).

Distributed directives
----------------------

These instructions only work on :term:`vQPUs <vQPU>` deployed with the corresponding communications enabled.

.. list-table::
   :header-rows: 1
   :widths: 26 74

   * - Method
     - Description
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.send` / :py:meth:`~cunqa.circuit.core.CunqaCircuit.recv`
     - :term:`Classical-communications <classical communications>` primitives: send / receive a
       measured classical bit to / from a remote circuit.
   * - :py:meth:`~cunqa.circuit.core.CunqaCircuit.gen_ent`
     - Low-level entanglement-generation primitive underlying the :term:`quantum communications`
       protocols.

For the high-level :term:`teledata` and :term:`telegate` protocols built on top of ``gen_ent``, use
the :py:mod:`cunqa.qc_protocols` helpers (:py:func:`~cunqa.qc_protocols.qsend`,
:py:func:`~cunqa.qc_protocols.qrecv`, :py:func:`~cunqa.qc_protocols.cat_entangler`,
:py:func:`~cunqa.qc_protocols.cat_disentangler`).
