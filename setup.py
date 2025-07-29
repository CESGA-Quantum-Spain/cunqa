from setuptools import setup, find_packages

setup(
    name="cunqa",
    version="0.3.0",
    packages=find_packages(),      # encontrará "cunqa"
    include_package_data=True,     # para MANIFEST.in si haces sdist
    package_data={
        # incluye cualquier .so o .pyd que esté en cunqa/
        "cunqa": ["*.so", "*.pyd"],
    },
    zip_safe=False,
    # install_requires, author, etc.
)