"""
Quantum-communication protocols for distributed circuits.

This subpackage provides the high-level helpers built on top of the
:py:meth:`~cunqa.circuit.core.CunqaCircuit.gen_ent` primitive to communicate quantum information
between circuits running on different vQPUs:

- **Teledata** (:py:func:`~cunqa.qc_protocols.teledata.qsend` / :py:func:`~cunqa.qc_protocols.teledata.qrecv`):
  teleports the state of a data qubit from one circuit to another.
- **Telegate** (:py:func:`~cunqa.qc_protocols.telegate.cat_entangler` /
  :py:func:`~cunqa.qc_protocols.telegate.cat_disentangler`): shares a control qubit across circuits
  so that remote controlled gates can be applied locally.

These functions reserve communication qubits on the participating circuits (created via the
``CunqaCircuit((num_data, num_comm), num_clbits)`` form), so each circuit must declare enough comm
qubits beforehand.
"""

from cunqa.qc_protocols.teledata import qsend, qrecv
from cunqa.qc_protocols.telegate import cat_entangler, cat_disentangler
