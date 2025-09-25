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

class VariableError(Exception):
    """ Class for signaling errors with the Variable class."""
    pass

class Variable(sympy.Symbol):
    """
    Class for handling variable parameters on gates. These will be input as gates arguments and need to be initiallized before executing a circuit with the method `:py:meth:assign_parameters`. 
    Unlike qiskit's Parameter, it can be reassigned several times or updated with `:py:class:QJob` method `:py:meth:upgrade_parameters`.
    """
    def __new__(cls, name, **assumptions):
        # Primary instance creation
        # Ensures unique symbol instances
        return sympy.Symbol.__new__(cls, name, **assumptions)

    def subs(self, param, value):
        """ Use .subs(param, value) to substitute the variable parameter for the value in the symbolic expression"""
        result = super().subs(self, param, value)
        return result.evalf()  

    def __array_ufunc__(self, ufunc, method, *inputs, **kwargs):
        """Override numpy ufuncs to produce a symbolic expression.

        This method is called when numpy ufuncs, like np.sin, np.cos, etc.,
        are applied to Variable objects.

        Args:
            ufunc: The numpy ufunc object being applied
            method (str): A string indicating how the ufunc is being used
            inputs (Any): The arguments passed to the ufunc
            kwargs (Any): Any keyword args (like 'out', 'where', etc.)

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
            
            elif ufunc.__name__== "log10":
                return sympy.log(self, 10)
            
            else:
                logger.error(f"The numpy.{ufunc.__name__} function is not implemented for Variables :(")
                raise NotImplemented
            
        return NotImplemented
    
    # Dunder methods already implemented on the parent class (including __add__, __pow__, etc and __eq__, __hash__ etc), but their behaviour
    # when a numpy function is applied to them and when substitution happens needs to be the same as that of Variable. For that, we will extend 
    # the classes Add, Mul, Pow, Mod
    
def variables(names, **args):
    r"""
    Wrapper of sympy.symbols. Transform strings into instances of :class:`Variable` class.

    :func:`variables` function returns a sequence of variables with names taken
    from ``names`` argument, which can be a comma or whitespace delimited
    string, or a sequence of strings::

        >>> from sympy import variables, Function

        >>> x, y, z = variables('x,y,z')
        >>> a, b, c = variables('a b c')

    The type of output is dependent on the properties of input arguments::

        >>> variables('x')
        x
        >>> variables('x,')
        (x,)
        >>> variables('x,y')
        (x, y)
        >>> variables(('a', 'b', 'c'))
        (a, b, c)
        >>> variables(['a', 'b', 'c'])
        [a, b, c]
        >>> variables({'a', 'b', 'c'})
        {a, b, c}

    If an iterable container is needed for a single parameter, set the ``seq``
    argument to ``True`` or terminate the parameter name with a comma::

        >>> variables('x', seq=True)
        (x,)

    To reduce typing, range syntax is supported to create indexed variables.
    Ranges are indicated by a colon and the type of range is determined by
    the character to the right of the colon. If the character is a digit
    then all contiguous digits to the left are taken as the nonnegative
    starting value (or 0 if there is no digit left of the colon) and all
    contiguous digits to the right are taken as 1 greater than the ending
    value::

        >>> variables('x:10')
        (x0, x1, x2, x3, x4, x5, x6, x7, x8, x9)

        >>> variables('x5:10')
        (x5, x6, x7, x8, x9)
        >>> variables('x5(:2)')
        (x50, x51)

        >>> variables('x5:10,y:5')
        (x5, x6, x7, x8, x9, y0, y1, y2, y3, y4)

        >>> variables(('x5:10', 'y:5'))
        ((x5, x6, x7, x8, x9), (y0, y1, y2, y3, y4))

    If the character to the right of the colon is a letter, then the single
    letter to the left (or 'a' if there is none) is taken as the start
    and all characters in the lexicographic range *through* the letter to
    the right are used as the range::

        >>> variables('x:z')
        (x, y, z)
        >>> variables('x:c')  # null range
        ()
        >>> variables('x(:c)')
        (xa, xb, xc)

        >>> variables(':c')
        (a, b, c)

        >>> variables('a:d, x:z')
        (a, b, c, d, x, y, z)

        >>> variables(('a:d', 'x:z'))
        ((a, b, c, d), (x, y, z))

    Multiple ranges are supported; contiguous numerical ranges should be
    separated by parentheses to disambiguate the ending number of one
    range from the starting number of the next::

        >>> variables('x:2(1:3)')
        (x01, x02, x11, x12)
        >>> variables(':3:2')  # parsing is from left to right
        (00, 01, 10, 11, 20, 21)

    Only one pair of parentheses surrounding ranges are removed, so to
    include parentheses around ranges, double them. And to include spaces,
    commas, or colons, escape them with a backslash::

        >>> variables('x((a:b))')
        (x(a), x(b))
        >>> variables(r'x(:1\,:2)')  # or r'x((:1)\,(:2))'
        (x(0,0), x(0,1))

    All newly created variables have assumptions set according to ``args``::

        >>> a = variables('a', integer=True)
        >>> a.is_integer
        True

        >>> x, y, z = variables('x,y,z', real=True)
        >>> x.is_real and y.is_real and z.is_real
        True
    """
    return sympy.symbols(names=names, cls=Variable, **args) 
    

# Probably for our purposes we won't ever need to use numbers directly
""" class Number:
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
            raise SystemExit """
    