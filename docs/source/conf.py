# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import sys
import os
import shutil
from pathlib import Path
sys.path.insert(0, os.path.abspath('../..'))


os.environ['INFO_PATH'] = ''
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


autodoc_default_options = {
    # "members": True,        # ← NO
    "private-members": False,
    "special-members": "",
}
autodoc_member_order = "bysource"
autodocsumm_member_order = "bysource"

autodoc_mock_imports = [
    'os', 
    'json', 
    'JSONDecodeError', 
    'load', 
    'logger', 
    'qiskit', 
    'dateutil', 
    'glob',
    'argparse',
    'logging',
    'numpy',
    'cunqa.qclient',
    'QClient',
    'cunqa.logger',
    'qmiotools',
    'qiskit_aer',
    'Pandoc',
    'cunqa.fakeqmio',
    'subprocess',
    'typing',
    'insert',
    'copy'
]


templates_path = ['_templates']
exclude_patterns = []


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme' 
# pygments_style = 'sphinx'  this color scheme displays code blocks with a green background and lively colors. maybe too much
html_static_path = ['_static']
html_css_files = [
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
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

napoleon_google_docstring = True
napoleon_preprocess_types = True
napoleon_numpy_docstring = False


# theoretically this module is loaded?
import sphinx.ext.napoleon.docstring as ndoc

_old_process_type = ndoc._convert_type_spec

def processing_lists_type(part,aliases):
    part = part[5:-1]
    init = f"list["; end = "]"
    while part.startswith("list[") and part.endswith("]"):
        part = part[5:-1]
        init+="list["; end+="]"

    return " `"+init+"` "+_old_process_type(part.strip(), aliases)+" `"+end+"` "
    

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



# napoleon_include_init_with_doc = False  # Incluye __init__ si tiene docstring
# # napoleon_include_private_with_doc = False  # Incluye miembros privados con docstring
# napoleon_include_special_with_doc = False  # Incluye miembros especiales con docstring
# napoleon_type_aliases = {}  # Puedes mapear tipos personalizados aquí
# napoleon_attr_annotations = True  # Procesa anotaciones de atributos
# napoleon_custom_sections = None  # Puedes definir secciones personalizadas en docstrings

highlight_options = {
    'linenos': 'inline',  # 'table' para columna separada, 'inline' para inline numbers
}


def setup(app):
    #Copy jupyter notebooks (+ .py) to folder docs/source/_examples so nbsphinx can read them for our gallery
    here = Path(__file__).resolve() # CUNQA/docs/source/
    project_root = here.parents[2]  # Goes from /docs/source to project root
    source_notebooks_dir = project_root / 'examples/jupyter'
    source_py_dir = project_root / 'examples/python'
    dest_dir = here.parent / '_examples'  # This will be docs/source/_examples
    dest_dir_2 = dest_dir / 'py_file_examples'

    dest_dir.mkdir(exist_ok=True) 
    dest_dir_2.mkdir(exist_ok=True)


    for notebook in source_notebooks_dir.glob('*.ipynb'):
        shutil.copy(notebook, dest_dir / notebook.name)
    for py_file in source_py_dir.glob('*.py'):
        shutil.copy(py_file, dest_dir_2 / py_file.name)

    # Automatically create files that include each of the .py examples
    for code_file in dest_dir_2.glob('*.py'):
        filename = code_file.stem # Get the filename without the extension

        # Create the Sphinx source file
        source_file = dest_dir_2 / f'{filename}.rst'
        with source_file.open('w') as f:
            f.write(f'{filename}\n')
            f.write('=' * len(filename) + '\n\n')
            f.write(f'.. literalinclude:: {filename}.py\n')
            f.write('   :language: python\n')
            f.write('   :linenos:\n')
