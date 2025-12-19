from __future__ import annotations
import importlib as _importlib

_submodules = [
    "circuit_extensions",
    "converters",
    "core",
    "helpers",
    "ir",
    "partitioning"
]

_lazy_symbols = {
    "to_ir": ("cunqa.circuit.ir", "to_ir"),
    "CunqaCircuit": ("cunqa.circuit.core", "CunqaCircuit"),
}

__all__ = _submodules + list(_lazy_symbols.keys()) + ["__version__"]


def __dir__():
    return __all__


def __getattr__(name: str):
    if name in _submodules:
        mod = _importlib.import_module(f"{__name__}.{name}")
        globals()[name] = mod  # cache
        return mod

    if name in _lazy_symbols:
        module_name, attr_name = _lazy_symbols[name]
        try:
            mod = _importlib.import_module(module_name)
        except Exception as e:
            raise ImportError(
                f"Cannot import '{name}' from '{module_name}'. "
                f"This usually means optional dependencies are missing "
                f"or the install is incomplete. Original error: {e}"
            ) from e
        obj = getattr(mod, attr_name)
        globals()[name] = obj  # cache
        return obj

    raise AttributeError(f"Module '{__name__}' has no attribute '{name}'")
