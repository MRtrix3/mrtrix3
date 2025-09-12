.. _msmt_csd:

Multi-shell multi-tissue constrained spherical deconvolution
============================================================

Introduction
------------

Multi-Shell Multi-Tissue Constrained Spherical Deconvolution (MSMT-CSD)
exploits the unique b-value dependencies of the different macroscopic
tissue types (WM/GM/CSF) to estimate a multi-tissue orientation distribution
function (ODF) as explained in [Jeurissen2014]_ As it includes separate
compartments for each tissue type, it can produce a map of the WM/GM/CSF signal
contributions directly from the DW data. In addition, the more complete
modelling of the DW signal results in more accurate apparent fiber density
(AFD) measures and more precise fibre orientation estimates at the tissue
interfaces.

User guide
----------

Prerequisites
^^^^^^^^^^^^^

MSMT-CSD relies on *multi-shell* high angular resolution diffusion imaging
(HARDI) data, containing multiple *b*-values. The number of tissue types that can
be resolved is limited by the number of *b*-values in the data (including
*b*\=0). To resolve the three primary tissue types in the brain (WM, GM & CSF),
the acquisition should contain at least 2 shells along with the *b*\=0 volumes
(i.e. 3 unique *b*-values).

In addition, this command expects that suitable *multi-shell multi-tissue response functions*
have already been computed. A number of approaches are available for this,
please refer to the :ref:`response_function_estimation` page for details.



Three-tissue deconvolution
^^^^^^^^^^^^^^^^^^^^^^^^^^

Multi-shell multi-tissue CSD using the common three macroscopic tissue components can be performed as:

.. code-block:: console

  dwi2fod msmt_csd dwi.mif wm_response.txt wmfod.mif gm_response.txt gm.mif csf_response.txt csf.mif

where:

- ``dwi.mif`` is the dwi data set (input)

- ``<tissue>_response.txt`` is the tissue-specific response function (input)

- ``<tissue>.mif`` is the tissue-specific ODF (output), typically full FODs for WM and a single scalars for GM and CSF

Note that input response functions and their corresponding output ODFs need to be specified at the command line in pairs.

Typically, you will also want to use the ``-mask`` option to avoid unnecessary computations in non-brain voxels:

.. code-block:: console

   dwi2fod msmt_csd -mask mask.mif dwi.mif wm_response.txt wmfod.mif gm_response.txt gm.mif csf_response.txt csf.mif

RGB tissue signal contribution maps can be obtained as follows:

.. code-block:: console

   mrconvert -coord 3 0 wm.mif - | mrcat csf.mif gm.mif - vf.mif

The resulting WM FODs can be displayed together with the tissue signal contribution map as:

.. code-block:: console

   mrview vf.mif -odf.load_sh wm.mif


.. _msmt_with_single_shell_data:

Single-shell data
^^^^^^^^^^^^^^^^^

The MSMT algorithm is typically understood as a solution for decomposing the DWI signal into three
macroscopic tissue compartments based on multi-shell DWI data. It is generally well understood that this
algorithm cannot intrinsically estimate these three components based on conventional "single-shell" DWI data
(i.e. at least one *b*=0 volume and one *b*>0 shell). The MSMT algorithm can nevertheless still be utilised
with such data as an alternative to the original :ref:`constrained_spherical_deconvolution` algorithm;
potential benefits include:

-   Application of a *hard* non-negativity constraint, such that negative amplitudes are almost entirely
    absent from the output ODF(s), whereas the original CSD implementation constrains negative amplitudes
    to be present but small.

-   Possibility to estimate from the data *two* tissue ODFs rather than one, taking advantage of the
    additional contrast present in the *b*=0 image data (which are not utilised at all in the original
    CSD implementation). It is typically recommended in this instance to utilise WM-like and CSF-like
    response functions, in order to estimate a WM-like ODF to which contamination from free-water-like
    contributions to the DWI signal is reduced.
