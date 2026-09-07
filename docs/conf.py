# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# Sphinx reads these module-level names by convention; their lower-case naming
#   and apparent non-use (they are consumed by Sphinx, not this module) are
#   inherent to the configuration format.
# pylint: disable=invalid-name,redefined-builtin,unused-variable

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'MRtrix3'
copyright = '2023, MRtrix contributors'
author = 'MRtrix contributors'
release = '3.0'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [ 'sphinx.ext.mathjax', 'notfound.extension' ]


templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

rst_prolog = """
.. |br| raw:: html

  <br/>
"""
