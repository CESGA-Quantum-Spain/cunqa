import os, sys
sys.path.insert(0, os.path.abspath('../..'))

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
    "sphinx_design",
    'sphinx_copybutton',
    'nbsphinx',
    "sphinx.ext.napoleon",
    'sphinx.ext.githubpages',
    "autodocsumm",
    "sphinx_toolbox.more_autosummary",
    "sphinx_inline_tabs"
]

source_suffix = ['.rst']

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

autodoc_member_order = 'bysource'
autodoc_typehints = "description"
add_function_parentheses = False

napoleon_google_docstring = True
napoleon_numpy_docstring = False
add_module_names = False


html_theme = 'sphinx_rtd_theme' 
html_static_path = ['_static']
html_css_files = [
    'css/nbgallery.css',
    'css/table.css',
    'css/sidebar.css', 
    'css/python_domain.css',
    'css/general.css'
]

html_logo = "_static/logo_cesga_blanco.png"
html_favicon = "_static/cunca.png"
html_theme_options = { }

html_static_path = ["_static"]
