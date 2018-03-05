The :code:`dwipreproc` script is provided for performing general pre-processing of diffusion image data - this includes eddy current-induced distortion correction, motion correction, and (possibly) susceptibility-induced distortion correction. Commands for performing this pre-processing are not yet implemented in *MRtrix3*; the :code:`dwipreproc` script in its current form is in fact an interface to the relevant commands that are provided as part of the `FSL <http://fsl.fmrib.ox.ac.uk/>`_ package. Installation of FSL (including `eddy <http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/EDDY>`_) is therefore required to use this script, and citation of the relevant articles is also required (see the :ref:`dwipreproc` help page).

Usage of this script varies depending on the specific nature of the DWI acquisition with respect to EPI phase encoding - full details are available within the :ref:`dwipreproc_page` page, and the :ref:`dwipreproc` help file.

Here, only a simple example is provided, where a single DWI series is acquired where all volumes have an anterior-posterior (A>>P) phase encoding direction::

    foreach * : dwipreproc IN/dwi_denoised_unringed.mif IN/dwi_denoised_unringed_preproc.mif -rpe_none -pe_dir AP
