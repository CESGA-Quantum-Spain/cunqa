CUNQA by example
================

Here some examples will be displayed and explained in order to understand not only how CUNQA works, 
but also its purpose and goals. This section provides a complete overview of the CUNQA 
functionalities. Each subsection follows the structure outlined below:

1. Code examples that explore the underlying concepts and functionalities.
2. A working example that allows users to directly try the functionalities and validate the concepts.

In this way, users can understand the fundamental concepts presented in each subsection and observe
how they are applied in practice. The main topics covered in this section are the following:

- :doc:`basic_usage`: basic guide on how to use the CUNQA API.
- :doc:`no_communications`: simplest scheme, i.e., distributing tasks without inter-QPU comms.
- :doc:`classical_communications`: send/recv classical bits between QPUs.
- :doc:`quantum_communications`: quantum comms with **teledata** and **telegate** examples.

Together, these topics provide a solid foundation for understanding the design principles and
capabilities of CUNQA in distributed quantum computing scenarios.

.. toctree::
   :maxdepth: 1
   :hidden:
   
      CUNQA basic usage <basic_usage.rst>
      No-communications <no_communications.rst>
      Classical-communications <classical_communications.rst>
      Quantum-communications <quantum_communications.rst>