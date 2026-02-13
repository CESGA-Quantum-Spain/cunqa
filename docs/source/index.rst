.. rst-class:: big-cunqa

CUNQA
======

.. figure:: /_static/QuantumCommScheme.png
   :alt: Quantum Communications scheme
   :width: 50%
   :align: center


|

.. centered:: **Emulator of Distributed Quantum Computing for HPC environments**


Welcome to CUNQA's documentation!
=================================

**CUNQA** is a Python/C++ platform that emulates distributed quantum computing (DQC) architectures 
on HPC environments. To achieve this, CUNQA defines the concept of a **virtual QPU (vQPU)**, which 
is a  combination of classical resources and simulation software that, working together, simulate 
the appearance and operation of a real QPU. These vQPUs are responsible for executing the quantum 
tasks defined by the user.

For these vQPUs to work, they require a management system. This system is written in C++, aiming 
for higher performance and better integration with the simulation libraries, most of which are also 
written in this language. In addition to this management system, it was necessary to define an API 
that allows the definition of quantum tasks and their subsequent submission to the vQPUs, providing
an intuitive and convenient way of using the platform.

This documentation will exclusively detail the parts of the library that are required by the user 
to employ the platform which can be found in the :doc:`CUNQA reference <reference/reference>` 
section.

All the code is available on the `GitHub repository <https://github.com/CESGA-Quantum-Spain/cunqa>`_ 
and a detailed explanation of the work done can be seen at the CUNQA preprint [vazquez2025]_.

.. raw:: html

   <p style="font-size: 9px;line-height: 1.5;">
   This work has been mainly funded by the project QuantumSpain, financed by the Ministerio de 
   Transformación Digital y Función Pública of Spain's Government through the project call QUANTUM 
   ENIA - Quantum Spain project, and by the European Union through the Plan de Recuperación, 
   Transformación y Resiliencia - NextGenerationEU within the framework of the Agenda España Digital. 
   1.    J. Vázquez-Pérez was supported by the Axencia Galega de Innovación (Xunta de Galicia) through 
   the Programa de axudas á etapa predoutoral (ED481A & IN606A). Additionally, this research 
   project was made possible through the access granted by the Galician Supercomputing Center 
   (CESGA) to two key parts of its infrastructure. Firstly, its Qmio quantum computing 
   infrastructure with funding from the European Union, through the Operational Programme 
   Galicia 2014-2020 of ERDF_REACT EU, as part of the European Union's response to the COVID-19 
   pandemic. Secondly, The supercomputer FinisTerrae III and its permanent data storage system, 
   which have been funded by the NextGeneration EU 2021 Recovery, Transformation and Resilience 
   Plan, ICT2021-006904, and also from the Pluriregional Operational Programme of Spain 2014-2020 
   of the European Regional Development Fund (ERDF), ICTS-2019-02-CESGA3, and from the State 
   Programme for the Promotion of Scientific and Technical Research of Excellence of the State Plan 
   for Scientific and Technical Research and Innovation 2013-2016 State subprogramme for scientific 
   and technical infrastructures and equipment of ERDF, CESG15-DE-3114. 
   </p>

.. [vazquez2025] J. Vázquez-Pérez, D. Expósito-Patiño, M. Losada, 
   Á. Carballido, A. Gómez y T. F. Pena,
   *CUNQA: a Distributed Quantum Computing emulator for HPC*,
   arXiv:2511.05209 [quant-ph], 2025.
   Available at: https://arxiv.org/abs/2511.05209


.. toctree::
   :maxdepth: 1
   :hidden:
   :caption: Contents:

   Installation <installation/getting_started>
   Quick start <quick_start/quick_start>
   CUNQA overview <cunqa_overview/cunqa_overview>
   Further Examples <further_examples/further_examples>
   CUNQA reference <reference/reference>