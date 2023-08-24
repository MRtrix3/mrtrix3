How to customise this template
==============================

#. Name your repository with the name fileformats-<SUBPACKAGE-TO-ADD-EXTRAS-TO>
#. Rename the `fileformats/mrtrix3` directory to the name of the fileformats subpackage the extras are for
#. Search and replace "mrtrix3" with the name of the fileformats subpackage the extras are to be added
#. Replace name + email placeholders in `pyproject.toml` for developers and maintainers
#. Add the extension file-format classes
#. Ensure that all the extension file-format classes are imported into the extras package root, i.e. `fileformats/mrtrix3`
#. Delete these instructions

...

FileFormats Extension - mrtrix3
====================================
.. image:: https://github.com/arcanaframework/fileformats-mrtrix3/actions/workflows/tests.yml/badge.svg
    :target: https://github.com/arcanaframework/fileformats-mrtrix3/actions/workflows/tests.yml
.. image:: https://codecov.io/gh/arcanaframework/fileformats-mrtrix3/branch/main/graph/badge.svg?token=UIS0OGPST7
    :target: https://codecov.io/gh/arcanaframework/fileformats-mrtrix3
.. image:: https://img.shields.io/github/stars/ArcanaFramework/fileformats-mrtrix3.svg
    :alt: GitHub stars
    :target: https://github.com/ArcanaFramework/fileformats-mrtrix3
.. image:: https://img.shields.io/badge/docs-latest-brightgreen.svg?style=flat
    :target: https://arcanaframework.github.io/fileformats/
    :alt: Documentation Status

This is the "mrtrix3" extension module for the
[fileformats](https://github.com/ArcanaFramework/fileformats-core) package


Quick Installation
------------------

This extension can be installed for Python 3 using *pip*::

    $ pip3 install fileformats-mrtrix3

This will install the core package and any other dependencies

License
-------

This work is licensed under a
`Creative Commons Attribution 4.0 International License <http://creativecommons.org/licenses/by/4.0/>`_

.. image:: https://i.creativecommons.org/l/by/4.0/88x31.png
  :target: http://creativecommons.org/licenses/by/4.0/
  :alt: Creative Commons Attribution 4.0 International License
