import os
import sys
import numpy as np
import sympy
from typing import Union
from cunqa.logger import logger
import symengine.lib.symengine_wrapper as se

# SymPy can use SymEngine as a backend
sympy.use_symengine = True

numpy_to_sympy_map = {
    "sin": sympy.sin,
    "cos": sympy.cos,
    "tan": sympy.tan,
    "exp": sympy.exp,
    "log": sympy.log,
    "log10": sympy.log,  # Log base 10, but uses sympy.log with base 10 argument
    "sqrt": sympy.sqrt,
    "abs": sympy.Abs,
    "arcsin": sympy.asin,
    "arccos": sympy.acos,
    "arctan": sympy.atan,
    "sinh": sympy.sinh,
    "cosh": sympy.cosh,
    "tanh": sympy.tanh,
    "arcsinh": sympy.asinh,
    "arccosh": sympy.acosh,
    "arctanh": sympy.atanh,
    "deg2rad": sympy.rad,
    "rad2deg": sympy.deg,
    "ceil": sympy.ceiling,
    "floor": sympy.floor,
    "round": sympy.N,  # sympy.N rounds symbolic numbers
    "gamma": sympy.gamma,
    "factorial": sympy.factorial
}

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

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Override numpy ufuncs to produce a symbolic expression.

        This method is called when numpy ufuncs, like np.sin, np.cos, etc.,
        are applied to Parameter objects.

        Args:
            ufunc: The numpy ufunc object being applied
            method: A string indicating how the ufunc is being used
            inputs: The arguments passed to the ufunc
            kwargs: Any keyword args (like 'out', 'where', etc.)

        Returns:
            A new sympy object corresponding to applying ufunc to the inputs
            symbolically.

        Raises:
            NotImplemented: if the ufunc/method combination is not handled
            (so numpy tries fallback behavior or errors).
        """
        if "out" in kwargs:
            # TODO: look into what these options do and decide how to support them
            return NotImplemented
        
        if method == "__call__":
            if ufunc.__name__ in numpy_to_sympy_map and ufunc.__name__!= "log10":
                # Applies the sympy function instead
                return numpy_to_sympy_map[ufunc.__name__](*inputs)
            
            elif ufunc.__name__!= "log10":
                return sympy.log(self, 10)
            
            else:
                logger.error(f"The numpy.{ufunc.__name__} function is not implemented for Parameters :(")
                raise NotImplemented
            
        return NotImplemented

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
    