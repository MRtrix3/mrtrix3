Welcome to the MRtrix user documentation!
=========================================

*MRtrix* provides a large suite of tools for image processing, analysis and visualisation, with a focus on the analysis of white matter using diffusion-weighted MRI. Features include the estimation of fibre orientation distributions using constrained spherical deconvolution ([Tournier2004]_; [Tournier2007]_; [Jeurissen2014]_), visualisation of these via directionally-encoded colour maps ([Dhollander2015a]_) and panchromatic sharpening ([Dhollander2015b]_), a probabilisitic streamlines algorithm for fibre tractography of white matter ([Tournier2012]_), fixel-based analysis of apparent fibre density and fibre cross-section ([Raffelt2012]_; [Raffelt2015]_; [Raffelt2016]_), quantitative structural connectivity analysis ([Smith2012]_; [Smith2013]_; [Smith2015]_; [Christiaens2015]_), and non-linear spatial registration of fibre orientation distribution images ([Raffelt2011]_).

These applications have been written from scratch in C++, using the functionality provided by `Eigen`_, and `Qt`_. The software is currently capable of handling DICOM, NIfTI and AnalyseAVW image formats, amongst others. The source code is distributed under the `Mozilla Public License`_.


.. _Eigen: http://eigen.tuxfamily.org/
.. _Qt: http://qt-project.org/
.. _Mozilla Public License: http://mozilla.org/MPL/2.0/


.. toctree::
   :maxdepth: 1
   :caption: Install

   installation/before_install
   installation/linux_install
   installation/mac_install
   installation/windows_install
   installation/deployment
   installation/hpc_clusters_install

.. toctree::
   :maxdepth: 1
   :caption: Getting started

   getting_started/key_features
   getting_started/commands_and_scripts
   getting_started/beginner_dwi_tutorial
   getting_started/image_data
   getting_started/command_line
   getting_started/config

.. toctree::
   :maxdepth: 1
   :caption: DWI Pre-processing

   dwi_preprocessing/denoising
   dwi_preprocessing/dwipreproc

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
   :caption: Spatial Normalisation

   spatial_normalisation/warping_images_with_warps_from_other_packages
..   spatial_normalisation/warp_file_formats
..   spatial_normalisation/transforming_streamlines

.. toctree::
   :maxdepth: 1
   :caption: Concepts

   concepts/dw_scheme
   concepts/pe_scheme
   concepts/global_intensity_normalisation
   concepts/orthonormal_sh_basis
   concepts/sh_basis_lmax
   concepts/fixels_dixels
   concepts/afd_connectivity

.. toctree::
   :maxdepth: 1
   :caption: Tips and Tricks

   tips_and_tricks/dicom_handling
   tips_and_tricks/batch_processing_with_foreach

.. toctree::
   :maxdepth: 1
   :caption: Troubleshooting

   troubleshooting/performance_and_crashes
   troubleshooting/display_issues
   troubleshooting/FAQ
   troubleshooting/advanced_debugging

.. toctree::
   :maxdepth: 1
   :caption: Reference

   reference/commands_list
   reference/scripts_list
   reference/config_file_options
   reference/mrtrix2_equivalent_commands
   reference/references

