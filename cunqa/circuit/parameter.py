from sympy import Symbol
from typing import Any

class Param:
    """
    Class representing a symbolic parameter associated with a quantum gate.

    This class encapsulates a symbolic expression (a SymPy expression)
    that defines a parametrized quantity inside a quantum circuit, such as
    rotation angles in parametrized gates (e.g., RX(θ), RZ(φ), etc.).

    The parameter can remain symbolic during circuit construction and later be
    evaluated by assigning numerical values to its variables. Once evaluated,
    the resulting numerical value is stored internally and can be retrieved
    through the :py:attr:`Param.value` attribute.
    """
    
    _value: float # Value of the parameter after evaluation or assignment.
    expr: Any # Symbolic expression representing the parameter of a gate
    
    def __init__(self, expr):
        self._value = None
        self.expr = expr
    
    @property
    def value(self) -> float:
        """Numerical value assigned to the parameter after evaluation."""
        return self._value
     
    @property
    def variables(self) -> list[Symbol]:
        """
        Symbolic variables appearing in the parameter expression. Returns the free symbols of the 
        internal symbolic expression.
        """
        return self.expr.free_symbols
    
    def __float__(self):
        return float(self._value)
    
    def eval(self, values):
        """Evaluates the symbolic expression using the provided substitutions."""
        self._value = self.expr.subs(values)
        
    def assign_value(self, value):
        """
        Directly assigns a numerical value to the parameter. This method bypasses symbolic 
        evaluation and directly sets the internal value.
        """
        self._value = value
        
def encoder(obj):
    if isinstance(obj, Param):
        return float(obj)
    raise TypeError(f"Object of type {type(obj)} is not JSON serializable")