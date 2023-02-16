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
sys.path.insert(0, os.path.abspath('/home/docs/checkouts/readthedocs.org/user_builds/vapor/conda/pythonapi2/lib/python3.9/site-packages/vapor'))

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

# Set plotly renderer to capture _repr_html_ for sphinx-gallery
try:
    import plotly.io as pio
    pio.renderers.default = 'sphinx_gallery'
except ImportError:
    pass

# suppress warnings
import warnings

# filter Matplotlib 'agg' warnings
warnings.filterwarnings("ignore",
                        category=UserWarning,
                        message='Matplotlib is currently using agg, which is a'
                        ' non-GUI backend, so cannot show the figure.')

# filter seaborn warnings
warnings.filterwarnings("ignore",
                        category=UserWarning,
                        message='As seaborn no longer sets a default style on'
                        ' import, the seaborn.apionly module is'
                        ' deprecated. It will be removed in a future'
                        ' version.')

# Configure sphinx-gallery plugin
from sphinx_gallery.sorting import ExampleTitleSortKey

sphinx_gallery_conf = {
    'examples_dirs': ['dataFormatRequirements/netCDF', 'vaporApplicationReference/imageRenderer'],  # path to your example scripts
    'gallery_dirs': ['dataFormatRequirements/netCDF/examples', 'vaporApplicationReference/imageRenderer'],  # path to where to save gallery generated output
    'within_subsection_order': ExampleTitleSortKey,
    'matplotlib_animations': True,
}

#
# Create Python API Reference materials
#

userModules = [
    'animation',
    'annotations',
    'camera',
    'dataset',
    'renderer',
    'session',
    'transferfunction',
    'transform'
]

devModules = [
    'cmake',
    'common',
    'config',
    'cppyyDoxygenWrapper',
    'link',
    'params',
    'smartwrapper'
]

condaPrefix = os.environ.get('CONDA_PREFIX')
pwd = os.path.dirname(os.path.realpath(__file__))

devModulesCommand = "sphinx-apidoc -f --separate --output-dir " + pwd + "/pythonAPIReference/devModules " + condaPrefix + "/lib/python3.9/site-packages/vapor "
for module in userModules:
    devModulesCommand += condaPrefix + "/lib/python3.9/site-packages/vapor/" + module + ".py "
print("devModulesCommand " + devModulesCommand)
ret = subprocess.run(devModulesCommand, capture_output=True, shell=True)

# Replace sphinx-apidoc default header name
with open(pwd + "/pythonAPIReference/devModules/vapor.rst", 'r+') as f:
    content = f.read().splitlines(True)
    content[0] = "Developer Modules\n"
    content[1] = "=================\n"
with open(pwd + "/pythonAPIReference/devModules/vapor.rst", 'w') as f:
    f.writelines(content)

userModulesCommand = "sphinx-apidoc -f --separate --output-dir " + pwd + "/pythonAPIReference/userModules " + condaPrefix + "/lib/python3.9/site-packages/vapor "
for module in devModules:
    userModulesCommand += condaPrefix + "/lib/python3.9/site-packages/vapor/" + module + ".py "
print("userModulesCommand " + userModulesCommand)
ret = subprocess.run(userModulesCommand, capture_output=True, shell=True)
print("userModulesResult " + str(ret.stdout.decode()))

# Replace sphinx-apidoc default header name
with open(pwd + "/pythonAPIReference/userModules/vapor.rst", 'r+') as f:
    content = f.read().splitlines(True)
    content[0] = "User Modules\n"
    content[1] = "============\n"
with open(pwd + "/pythonAPIReference/userModules/vapor.rst", 'w') as f:
    f.writelines(content)
