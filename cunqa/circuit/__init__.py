from cunqa.circuit.converters import convert
from cunqa.circuit.circuit_extensions import cunqa_dunder_methods
from cunqa.circuit.circuit import CunqaCircuit

CunqaCircuit = cunqa_dunder_methods(CunqaCircuit)