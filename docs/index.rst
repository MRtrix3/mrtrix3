.. MRtrix documentation master file, created by
   sphinx-quickstart on Fri Jan 29 15:02:17 2016.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to the MRtrix user documentation!
==================================

MRtrix provides a set of tools to perform diffusion-weighted MRI white matter tractography in the presence of crossing fibres, using Constrained Spherical Deconvolution (`Tournier et al.. 2004`_; `Tournier et al. 2007`_), and a probabilisitic streamlines algorithm (`Tournier et al., 2012`_). These applications have been written from scratch in C++, using the functionality provided by the `GNU Scientific Library`_, and `Qt`_. The software is currently capable of handling DICOM, NIfTI and AnalyseAVW image formats, amongst others. The source code is distributed under the `GNU General Public License`_.

.. _Tournier et al.. 2004: http://www.ncbi.nlm.nih.gov/pubmed/15528117
.. _Tournier et al. 2007: http://www.ncbi.nlm.nih.gov/pubmed/17379540
.. _Tournier et al., 2012: http://onlinelibrary.wiley.com/doi/10.1002/ima.22005/abstract
.. _GNU Scientific Library: http://www.gnu.org/software/gsl/
.. _Qt: http://qt-project.org/
.. _GNU General Public License: http://www.gnu.org/copyleft/gpl.html

Development
----------------------------------

For those interested in contributing to MRtrix3, or just using it as a base to develop their own applications, please consult the `Doxygen-generated developer documentation`_.

.. _Doxygen-generated developer documentation: http://MRtrix3.github.io/developer-documentation/

Acknowledging this work
----------------------------------

If you wish to include results generated using the MRtrix package in a publication, please include a line such as the following to acknowledge this work:

* Fibre-tracking was performed using the MRtrix package (J-D Tournier, Brain Research Institute, Melbourne, Australia, https://github.com/MRtrix3/mrtrix3) (Tournier et al. 2012)

Any other relevant references will be listed on the specific application's page.

Warranty
----------------------------------

The software described in this manual has no warranty, it is provided "as is". It is your responsibility to validate the behavior of the routines and their accuracy using the source code provided, or to purchase support and warranties from commercial redistributors. Consult the `GNU General Public License`_ for further details.

.. _GNU General Public License: http://www.gnu.org/copyleft/gpl.html

License
----------------------------------

MRtrix is free software: you can redistribute it and/or modify it under the terms of the `GNU General Public License`_ as published by the `Free Software Foundation`_, either version 3 of the License, or (at your option) any later version.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the `GNU General Public License`_ for more details.
You should have received a copy of `GNU General Public License`_ along with MRtrix. If not, see `http://www.gnu.org/licenses/`_.

.. _Free Software Foundation: http://www.fsf.org/
.. _http://www.gnu.org/licenses/: http://www.gnu.org/licenses/


.. toctree::
   :maxdepth: 4
   :caption: Common tasks

   common_tasks/basic_dwi_processing
   common_tasks/act
   common_tasks/sift
   common_tasks/structural_connectome
   common_tasks/hcp_connectome






