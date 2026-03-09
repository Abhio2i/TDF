# Configuration file for the Sphinx documentation builder.

import os
import sys
sys.path.insert(0, os.path.abspath('.'))

# -- Project information -----------------------------------------------------

project = 'TDF Help Documentation'
author = 'O2I'
release = '2.0'

# -- General configuration ---------------------------------------------------

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',
    'sphinx.ext.todo',
    'sphinx.ext.autosectionlabel',
    'sphinx.ext.viewcode',
]

templates_path = ['_templates']
exclude_patterns = []

language = 'en'

# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'

html_static_path = ['_static']

html_title = "TDF Help"
html_logo = None   # Add later if you want

# Enable numbering inside pages but NOT in sidebar
numfig = True

# Allow :ref: linking without prefix
autosectionlabel_prefix_document = False

# Support for images folder
html_extra_path = ['images']

