Welcome to the MRtrix user documentation!
==================================

*MRtrix* provides a set of tools to perform diffusion-weighted MRI white matter tractography in the presence of crossing fibres, using Constrained Spherical Deconvolution (`Tournier et al.. 2004`_; `Tournier et al. 2007`_), and a probabilisitic streamlines algorithm (`Tournier et al., 2012`_). These applications have been written from scratch in C++, using the functionality provided by `Eigen`_, and `Qt`_. The software is currently capable of handling DICOM, NIfTI and AnalyseAVW image formats, amongst others. The source code is distributed under the `Mozilla Public License`_.

.. _Tournier et al.. 2004: http://www.ncbi.nlm.nih.gov/pubmed/15528117
.. _Tournier et al. 2007: http://www.ncbi.nlm.nih.gov/pubmed/17379540
.. _Tournier et al., 2012: http://onlinelibrary.wiley.com/doi/10.1002/ima.22005/abstract
.. _Eigen: http://eigen.tuxfamily.org/
.. _Qt: http://qt-project.org/
.. _Mozilla Public License: http://mozilla.org/MPL/2.0/


.. toctree::
   :maxdepth: 2
   :caption: 

.. toctree::
   :maxdepth: 2
   :caption: Install

   installation/before_install
   installation/linux_install
   installation/mac_install
   installation/windows_install
   installation/hpc_clusters_install

.. toctree::
   :maxdepth: 2
   :caption: Getting started
   
   getting_started/key_features
   getting_started/config
   getting_started/image_data
   getting_started/command_line
   getting_started/troubleshooting
   
.. toctree::
   :maxdepth: 2
   :caption: Tutorials
   
   tutorials/basic_dwi_processing
   tutorials/denoising
   tutorials/dwipreproc
   tutorials/hcp_connectome
   tutorials/connectome_tool
   tutorials/labelconvert
   tutorials/advanced_debugging
   tutorials/warping_images_with_warps_from_other_packages
   tutorials/FAQ

.. toctree::
   :maxdepth: 2
   :caption: Workflows

   workflows/DWI_preprocessing_for_quantitative_analysis
   workflows/fixel_based_analysis
   workflows/act
   workflows/sift
   workflows/structural_connectome
   workflows/global_tractography
   workflows/multi_tissue_csd

.. toctree::
   :maxdepth: 2
   :caption: Concepts
  
   concepts/orthonormal_basis
   concepts/dixels_fixels
   concepts/afd_connectivity
   concepts/response_function_estimation

.. toctree::
   :maxdepth: 1
   :caption: Reference
   
   reference/commands_list
   reference/scripts_list
   reference/config_file_options
   reference/mrtrix2_equivalent_commands

