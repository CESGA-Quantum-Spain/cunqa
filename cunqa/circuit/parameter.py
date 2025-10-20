import os
import sys
import numpy as np
import sympy
from typing import Union
from cunqa.logger import logger
import symengine.lib.symengine_wrapper as se

# SymPy can use SymEngine as a backend
sympy.use_symengine = True

class ParameterError(Exception):
    """ Class for signaling errors with the Parameter class."""
    pass

class Parameter(sympy.Symbol):
    """ """
    def __new__(cls, name, **assumptions):
        # Primary instance creation
        # Ensures unique symbol instances
        return sympy.Symbol.__new__(cls, name, **assumptions)

    def subs(self, param, value):
        """ Use .subs(param, value) to substitute the variable parameter for the value in the symbolic expression"""
        # All well and good but once a function like sin is applied to a parameter, it no longer is a 
        # cunqa Parameter and substituting will return a sympy type number, 
        result = super().subs(self, param, value)
        return result.evalf()  

    # Dunder methods already implemented on the parent class
    
    
class Number:
    """ """
    def __init__(self, value: Union[int, float]):
        if isinstance(value, int):
            self.number = sympy.Integer(value)
        elif isinstance(value, float):
            self.number = sympy.Float(value)
        elif isinstance(value, complex):
            self.number = sympy.Float(value.real()) + sympy.Float(value.imag())*sympy.I
        else:
            logger.error(f"Value of an unsupported type was given: {type(value)}.")
            raise SystemExit
    