# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# http://www.sphinx-doc.org/en/master/config

import os
import sys
import sphinx_rtd_theme
import subprocess
import pkgutil

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#

# Add vapor_utils to path and vapor_wrf modules for python engine documentation
sys.path.insert(0, os.path.abspath('vaporApplicationReference/otherTools'))

# -- Project information -----------------------------------------------------

project = ' '
copyright = '2023 University Corporation for Atmospheric Research'
author = ''

# The short X.Y version
version = ''
# The full version, including alpha/beta/rc tags
release = '3.8.0'


#breathe_projects = { "myproject": "/Users/pearse/vapor2/targets/common/doc/library/xml" }
#breathe_default_project = "myproject"

extensions = [
    'sphinx.ext.imgmath', 
    'sphinx.ext.todo', 
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx_gallery.gen_gallery',
    #'jupyter_sphinx.execute'
    #'breathe'
    #'wheel'
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
# source_suffix = ['.rst', '.md']
source_suffix = '.rst'

# The master toctree document.
master_doc = 'index'

language = 'en'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = None


html_logo = "_images/vaporLogoBlack.png"
html_favicon = "_images/vaporVLogo.png"

html_theme = "sphinx_book_theme"
html_theme_options = dict(
    # analytics_id=''  this is configured in rtfd.io
    # canonical_url="",
    repository_url="https://github.com/NCAR/VAPOR",
    repository_branch="main",
    path_to_docs="doc",
    use_edit_page_button=True,
    use_repository_button=True,
    use_issues_button=True,
    home_page_in_toc=False,
    extra_navbar="",
    navbar_footer_text="",
    extra_footer=""
)

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']


# -- Options for HTMLHelp output ---------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'Vapordoc'

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    (master_doc, 'Vapor.tex', 'Vapor Documentation',
     'John Clyne, Scott Pearse, Samuel Li, Stanislaw Jaroszynski', 'manual'),
]


# -- Options for manual page output ------------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    (master_doc, 'vapor', 'Vapor Documentation',
     [author], 1)
]


# -- Options for Texinfo output ----------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
    (master_doc, 'Vapor', 'Vapor Documentation',
     author, 'Vapor', 'One line description of project.',
     'Miscellaneous'),
]

# -- Options for Epub output -------------------------------------------------

# Bibliographic Dublin Core info.
epub_title = project

###
### Configure Sphinx-gallery
###

# Download example data 
import requests
from pathlib import Path
home = str(Path.home())
simpleNC = home + "/simple.nc"
dataFile = "https://github.com/NCAR/VAPOR-Data/raw/main/netCDF/simple.nc"
response = requests.get(dataFile, stream=True)
with open(simpleNC, "wb") as file:
  for chunk in response.iter_content(chunk_size=1024):
    if chunk:
      file.write(chunk)
