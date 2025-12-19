# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os, sys
import shutil
from pathlib import Path

sys.path.insert(0, os.path.abspath('../..'))

os.environ['CUNQA_PATH'] = ''
os.environ['HOSTNAME'] = ''
os.environ['QPUS_FILEPATH'] = ''
os.environ['SLURMD_NODENAME'] = ''
os.environ['SLURM_JOB_ID'] = ''
os.environ['STORE'] = ''

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'CUNQA'
copyright = '2025, Álvaro Carballido, Marta Losada, Jorge Vázquez, Daniel Expósito'
author = 'Álvaro Carballido, Marta Losada, Jorge Vázquez, Daniel Expósito'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx_copybutton',
    'nbsphinx',
    'sphinx_mdinclude',
    'sphinx.ext.githubpages',
    'sphinx.ext.napoleon',
    "autodocsumm",
    "sphinx_toolbox.more_autosummary"
]

source_suffix = ['.rst', '.md']

autosummary_generate = True
autosummary_generate_overwrite = True

nbsphinx_execute = "never" # Never execute the Jupyter notebooks

autodoc_default_options = {
    # "members": True,        # ← NO
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
    #'QClient',
    #'qiskit',
    #'qiskit_aer',
    'random',
    'string',
    'subprocess',
    'sys',
    'time',
    'typing'
]


templates_path = ['_templates']
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme' 
# pygments_style = 'sphinx'  this color scheme displays code blocks with a green background and lively colors. maybe too much
html_static_path = ['_static']
html_css_files = [
    'custom.css',
    'table.css',
    'hide_bases_object.js',
]

html_logo = "_static/logo_cesga_blanco.png"
html_favicon = "_static/favicon.ico"
html_theme_options = {
    'logo_only': False,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'vcs_pageview_mode': '',
    'flyout_display': 'hidden',
    'version_selector': True,
    'language_selector': True,
    # Toc options
    'collapse_navigation': False,
    'sticky_navigation': True,
    'navigation_depth': 3,
    'includehidden': True,
    'titles_only': False
}

napoleon_google_docstring = True
napoleon_preprocess_types = True
napoleon_numpy_docstring = False


# theoretically this module is loaded?
import sphinx.ext.napoleon.docstring as ndoc

_old_process_type = ndoc._convert_type_spec

def processing_lists_type(part, aliases):
    original = part
    part = part[5:-1]
    init = "list["
    end = "]"
    while part.startswith("list[") and part.endswith("]"):
        part = part[5:-1]
        init += "list["
        end += "]"
    # Common Python types
    valid_types = [
        "int",      "float",       
        "str",      "bool", 
        "list",     "dict", 
        "tuple",    "set", 
        "None",     "bytes", 
        "complex",  "object", 
        "callable"
    ]

    if part not in valid_types:
        return _old_process_type(init.strip(),aliases)  + \
               _old_process_type(part.strip(), aliases) + \
               _old_process_type(end.strip(),aliases)
    else:
        return _old_process_type(original.strip(), aliases)
    

def _custom_process_type(name, aliases={}):
    # Split the name by "|" and process each part
    processed = []
    parts = name.split("|")
    for part in parts:
        part = part.strip()
        if part.startswith("list[") and part.endswith("]"):
            processed.append(processing_lists_type(part,aliases))
        else:
            processed.append(_old_process_type(part, aliases))

    return " | ".join(processed)

# Monkeypatch
ndoc._convert_type_spec = _custom_process_type

highlight_options = {
    'linenos': 'inline',  # 'table' para columna separada, 'inline' para inline numbers
}