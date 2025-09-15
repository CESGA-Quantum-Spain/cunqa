import os
import sys
import numpy as np
from typing import Union
from cunqa.logger import logger
import sympy
from sympy import Symbol, Number
import symengine.lib.symengine_wrapper as se

# SymPy can use SymEngine as a backend
sympy.use_symengine = True


class ParameterError(Exception):
    """ Class for signaling errors with the Parameter class."""
    pass

class Parameter(Symbol):
    """ """
    def __init__(self, name, **assumptions):
        super().__init__(name, **assumptions)

    def substitute(self):
        pass

    ####### MODIFY THE EXPRESSION #######
    def sin(self):
        """Sine of a Parameter"""
        return sympy.sin(self)

    def cos(self):
        """Cosine of a Parameter"""
        return sympy.cos(self)

    def tan(self):
        """Tangent of a Parameter"""
        return sympy.tan(self)

    def arcsin(self):
        """Arcsin of a Parameter"""
        return sympy.asin(self)

    def arccos(self):
        """Arccos of a Parameter"""
        return sympy.acos(self)

    def arctan(self):
        """Arctan of a Parameter"""
        return sympy.atan(self)

    def exp(self):
        """Exponential of a Parameter"""
        return sympy.exp(self)

    def log(self, base: float = None):
        """Logarithm of a Parameter"""
        if base is not None:
            return sympy.log(self, base)
        else:
            return sympy.log(self)

    def sign(self):
        """Sign of a Parameter"""
        return sympy.sign(self)

    # ####### DUNDER METHODS
    # def __add__(self, other):
    #     pass

    # def __radd__(self, other):
    #     pass

    # def __sub__(self, other):
    #     pass

    # def __rsub__(self, other):
    #     pass

    # def __mul__(self, other):
    #     pass

    # def __pos__(self):
    #     pass

    # def __neg__(self):
    #     pass

    # def __rmul__(self, other):
    #     pass

    # def __truediv__(self, other):
    #     pass

    # def __rtruediv__(self, other):
    #     pass

    # def __pow__(self, other):
    #     pass

    # def __rpow__(self, other):
    #     pass
    
    
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
    