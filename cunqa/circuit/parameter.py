import os
import sys
import numpy as np
import sympy
from typing import Union
from cunqa.logger import logger
import symengine.lib.symengine_wrapper as se

# SymPy can use SymEngine as a backend
sympy.use_symengine = True

class Variable(sympy.Symbol):
    """
    Object that signals to a parametric gate that its value can vary. Variables can be summed, multiplied by 
    other variables or numbers, exponentiated, divided, etc to create parametric expressions. To apply functions
    to variables please use those of the `sympy` module, e.g. `sin_a = sympy.sin(Variable('a'))`. 

    .. note::
        The some available `sympy` functions are 
        "sin", "cos", "tan", "exp", "log", "log10", "sqrt", "abs", "arcsin", "arccos", "arctan", "sinh", 
        "cosh", "tanh", "arcsinh", "arccosh", "arctanh", "deg2rad", "rad2deg", "floor", "round", "gamma",
        "factorial".
    """
    def __new__(cls, name, **assumptions):
        # Primary instance creation
        # Ensures unique symbol instances
        return sympy.Symbol.__new__(cls, name, **assumptions)

    def subs(self, param, value):
        """ Use .subs(param, value) to substitute the variable parameter for the value in the symbolic expression""" 
        result = super().subs(self, param, value)
        return float(result.evalf())

    # Dunder methods already implemented on the parent class
    # In particular, as sympy defines __hash__ and __eq__, so 
    # sympy objects and therefore cunqa.Variable objects can be used as keys of a dict
    
    