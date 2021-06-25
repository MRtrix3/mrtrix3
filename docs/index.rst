Welcome to the *MRtrix3* user documentation!
============================================

*MRtrix3* provides a large suite of tools for image processing, analysis and
visualisation, with a focus on the analysis of white matter using
diffusion-weighted MRI ([Tournier2019]_). Features include the estimation of
fibre orientation distributions using constrained spherical deconvolution
([Tournier2004]_; [Tournier2007]_; [Jeurissen2014]_), a probabilisitic
streamlines algorithm for fibre tractography of white matter ([Tournier2012]_),
fixel-based analysis of apparent fibre density and fibre cross-section
([Raffelt2012]_; [Raffelt2015]_; [Raffelt2017]_), quantitative structural
connectivity analysis ([Smith2012]_; [Smith2013]_; [Smith2015]_;
[Christiaens2015]_), and non-linear spatial registration of fibre orientation
distribution images ([Raffelt2011]_). *MRtrix3* also offers comprehensive
visualisation tools in :ref:`mrview`.

These applications have been written from scratch in C++, using the
functionality provided by `Eigen`_, and `Qt`_. The software is currently
capable of handling DICOM, NIfTI and AnalyseAVW image formats, amongst others.
The source code is distributed under the `Mozilla Public License`_.

Use of the *MRtrix3* software package in published works should be accompanied
by the following citation:

    J.-D. Tournier, R. E. Smith, D. Raffelt, R. Tabbara, T. Dhollander, M.
    Pietsch, D. Christiaens, B. Jeurissen, C.-H. Yeh, and A. Connelly.
    *MRtrix3*: A fast, flexible and open software framework for medical image
    processing and visualisation. NeuroImage, 202 (2019), pp. 116–37.


.. TIP::

  Make sure to use the version of this documentation that matches your version
  of this software. You can select the version on the lower left of this page.



Getting help
~~~~~~~~~~~~

There are a variety of sources of help and information to bring you up to speed
with *MRtrix3*. These include:

- the `main MRtrix3 documentation <https://mrtrix.readthedocs.org/>`__ (these
  pages);

- our `Introduction to the Unix command-line <https://command-line-tutorial.readthedocs.io/>`__
  if you're unfamiliar with the terminal (though you'll readily find plenty of
  excellent tutorials online);

- our `Community Forum <http://community.mrtrix.org/>`__ for support and general
  discussion about the use of *MRtrix3* -- you can address all
  *MRtrix3*-related queries there, using your GitHub or Google login to post
  questions.

- our `Frequently Asked Questions <http://community.mrtrix.org/c/wiki>`__, hosted
  as a user-editable ``wiki`` category within our forum.






Key features
~~~~~~~~~~~~

While *MRtrix3* is primarily intended to be used for the analysis of
diffusion MRI data, at its fundamental level it is designed as a
general-purpose library for the analysis of *any* type of MRI data. As such,
it provides a back-end to simplify a large number of operations, many of
which will be invisible to the end-user. Specifically, *MRtrix3* features:

-  a consistent :ref:`command-line interface <command-line-interface>`, with
   inline documentation for each command;

-  universal import/export capabilities when
   :ref:`accessing image data <image_handling>` across all *MRtrix3* applications;

-  :ref:`multi_file_image_file_formats` to load multiple images as a
   single multi-dimensional dataset;

-  efficient use of :ref:`unix_pipelines` for complex workflows;

-  high performance on modern multi-core systems, with multi-threading
   used extensively throughout *MRtrix3*;

-  available on all common modern operating systems (GNU/Linux,
   MacOSX, Windows);

-  a consistent :ref:`image_coord_system` with most
   operations performed in scanner/world coordinates where possible.


Installation
~~~~~~~~~~~~

*MRtrix3* runs on GNU/Linux, macOS, Microsoft Windows platforms, and other Unix
platforms. For most users, the simplest way to install *MRtrix3* is to use one
of the pre-compiled packages. For details, please refer `the main MRtrix
website <https://www.mrtrix.org/download/>`__.

If the precompiled packages are not available, we provide specific instructions
for building the software from source. This is normally a simple process, but
does require more compute resources and expertise. See the :ref:`relevant pages
<before_installing>` for details.


Commands
~~~~~~~~

The *MRtrix3* software package includes a suite of tools for image analysis and
visualisation. With the exception of :ref:`mrview` and :ref:`shview`, all
*MRtrix3* executables are designed to be run via a terminal using a consistent
:ref:`command-line interface <command-line-interface>`. While many of the tools
and features are discussed within tutorials found in this documentation, a
comprehensive :ref:`list-of-mrtrix3-commands` can be found in the reference
section. These lists provide links to the help page (manual) for each
executable, which can also be accessed by typing the :code:`-help` option after
the executable name on the terminal.

.. TIP::

  Some proficiency with the Unix command-line is required to make the best use
  of this software. There are many resources online to help you get
  started if you are not already familiar with it. We also recommend our own
  `Introduction to the Unix command-line
  <https://command-line-tutorial.readthedocs.io/>`__, which was written with a
  particular focus on the types of use that are common when using *MRtrix3*.




.. _Eigen: http://eigen.tuxfamily.org/
.. _Qt: http://qt-project.org/
.. _Mozilla Public License: http://mozilla.org/MPL/2.0/


Table of Contents
=================

.. toctree::
   :maxdepth: 1
   :caption: Install

   installation/before_install
   installation/package_install
   installation/build_from_source
   installation/deployment
   installation/hpc_clusters_install
   installation/using_containers

.. toctree::
   :maxdepth: 1
   :caption: Getting started

   getting_started/beginner_dwi_tutorial
   getting_started/image_data
   getting_started/command_line
   getting_started/config

.. toctree::
   :maxdepth: 1
   :caption: DWI Pre-processing

   dwi_preprocessing/denoising
   dwi_preprocessing/dwifslpreproc

.. toctree::
   :maxdepth: 1
   :caption: Constrained Spherical Deconvolution

   constrained_spherical_deconvolution/response_function_estimation
   constrained_spherical_deconvolution/constrained_spherical_deconvolution
   constrained_spherical_deconvolution/multi_shell_multi_tissue_csd

.. toctree::
   :maxdepth: 1
   :caption: Fixel-Based Analysis

   fixel_based_analysis/st_fibre_density_cross-section
   fixel_based_analysis/mt_fibre_density_cross-section
   fixel_based_analysis/fixel_directory_format
   fixel_based_analysis/mitigating_brain_cropping
   fixel_based_analysis/computing_effect_size_wrt_controls
   fixel_based_analysis/displaying_results_with_streamlines

.. toctree::
   :maxdepth: 1
   :caption: Quantitative Structural Connectivity

   quantitative_structural_connectivity/act
   quantitative_structural_connectivity/sift
   quantitative_structural_connectivity/structural_connectome
   quantitative_structural_connectivity/connectome_tool
   quantitative_structural_connectivity/labelconvert_tutorial
   quantitative_structural_connectivity/global_tractography
   quantitative_structural_connectivity/ismrm_hcp_tutorial

.. toctree::
   :maxdepth: 1
   :caption: Concepts

   concepts/dw_scheme
   concepts/pe_scheme
   concepts/global_intensity_normalisation
   concepts/spherical_harmonics
   concepts/sh_basis_lmax
   concepts/fixels_dixels
   concepts/afd_connectivity

.. toctree::
   :maxdepth: 1
   :caption: Tips and Tricks

   tips_and_tricks/dicom_handling
   tips_and_tricks/batch_processing_with_foreach
   tips_and_tricks/external_modules
   tips_and_tricks/troubleshooting

.. toctree::
   :maxdepth: 1
   :caption: Reference

   reference/commands_list
   reference/config_file_options
   reference/environment_variables
   reference/mrtrix2_equivalent_commands
   reference/references

