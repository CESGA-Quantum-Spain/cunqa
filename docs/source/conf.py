# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os, sys
import shutil
from pathlib import Path

sys.path.insert(0, os.path.abspath('../..'))


sys.path = [
    p for p in sys.path
    if not os.path.abspath(p).startswith(
        "/mnt/netapp1/Store_CESGA/home/cesga/jvazquez/works/cunqa"
    )
]

print(sys.path)



os.environ['CUNQA_PATH'] = ''
os.environ['HOSTNAME'] = ''
os.environ['QPUS_FILEPATH'] = ''
os.environ['SLURMD_NODENAME'] = ''
os.environ['SLURM_JOB_ID'] = ''
os.environ['STORE'] = ''

project = 'CUNQA'
copyright = '2025, Álvaro Carballido, Marta Losada, Jorge Vázquez, Daniel Expósito'
author = 'Álvaro Carballido, Marta Losada, Jorge Vázquez, Daniel Expósito'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx_copybutton',
    #'nbsphinx',
    "sphinx_multiversion",
    'sphinx_mdinclude',
    'sphinx.ext.githubpages',
    "autodocsumm",
    "sphinx_toolbox.more_autosummary"
]

source_suffix = ['.rst']

# ---------------  VERSIONING OPTIONS ---------------
smv_tag_whitelist = r'^(0\.(2|[3-9])\.\d+|[1-9]\d*\.\d+\.\d+)$'
smv_branch_whitelist = r"^62-testssss-automatic$"   
smv_latest_version = ''

# --------------- AUTOSUMMARY OPTIONS ---------------
autosummary_generate = True
autosummary_generate_overwrite = True
autodoc_default_options = {
    "private-members": False,
    "special-members": "",
}
autodoc_member_order = "bysource"
autodocsumm_member_order = "bysource"
autodoc_mock_imports = [
    'argparse',
    'collections',
    'copy',
    'cunqa.constants',
    'cunqa.logger',
    'cunqa.qclient',
    'dateutil',
    'fcntl',
    'glob',
    'insert',
    'inspect',
    'JSONDecodeError',
    'json',
    'load',
    'logging',
    'logger',
    'operator',
    'os',
    'random',
    'string',
    'subprocess',
    'sys',
    'time',
    'typing'
]

nbsphinx_execute = "never" # Never execute the Jupyter notebooks

templates_path = ['_templates']
exclude_patterns = ['tutorial/*']


html_theme = 'sphinx_rtd_theme' 
html_static_path = ['_static']
html_css_files = [
    'css/nbgallery.css',
    'css/table.css',
    'css/sidebar.css'
]

html_logo = "_static/logo_cesga_blanco.png"
html_favicon = "_static/cunca.png"
html_theme_options = { }

html_static_path = ["_static"]
