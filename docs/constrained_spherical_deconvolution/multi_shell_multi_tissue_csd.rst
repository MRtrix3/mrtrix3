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

Three-tissue decomposition
^^^^^^^^^^^^^^^^^^^^^^^^^^

Multi-shell multi-tissue CSD using the common three macroscopic tissue components can be performed as:

.. code-block:: console

  dwi2fod msmt_csd dwi.mif wm_response.txt wmfod.mif gm_response.txt gm.mif csf_response.txt csf.mif

where:

- ``dwi.mif`` is the dwi data set (input)

- ``<tissue>_response.txt`` is the tissue-specific response function (input)

- ``<tissue>.mif`` is the tissue-specific ODF (output), typically full FODs for WM and a single scalars for GM and CSF

Note that input response functions and their corresponding output ODFs need to be specified at the command line in pairs.
The order of the tissues relative to one another is not consequential here;
only that each tissue is defined as an input response function text file followed by the corresponding output ODF image.

It is possible to use the ``-mask`` option to avoid unnecessary computations:

.. code-block:: console

   dwi2fod msmt_csd -mask mask.mif dwi.mif wm_response.txt wmfod.mif gm_response.txt gm.mif csf_response.txt csf.mif

Some caution should however be exercised in this respect.
By neglecting to fit the model to the image data immediately adjacent to such a mask,
interpolation of values at sub-voxel locations inside of that mask
will be detrimentally affected by sampling from those adjacent zero-filled values.
Further, while fitting the model to image voxels that do not contribute to downstream processing
may only result in an increase in computation time,
exclusion of voxels from model fitting may have unrecoverable detrimental effects.
It may therefore be preferable to, for instance,
utilise a dilated mask in this context
to mitigate the chance of downstream issues.

RGB tissue signal contribution maps can be obtained as follows:

.. code-block:: console

   mrconvert -coord 3 0 wm.mif - | mrcat csf.mif gm.mif - vf.mif

This follows prior precedent wherein the CSF, GM and ODF signal contributions
are interpreted as red, green and blue colour components respectively.

The resulting WM FODs can be displayed together with the tissue signal contribution map as:

.. code-block:: console

   mrview vf.mif -odf.load_sh wm.mif

.. _msmt_with_single_shell_data:

Single-shell data
^^^^^^^^^^^^^^^^^

The MSMT algorithm is typically understood as a solution
for decomposing the DWI signal into three macroscopic tissue compartments
based on multi-shell DWI data.
It is generally well understood that this algorithm
cannot intrinsically estimate these three components based on conventional "single-shell" DWI data
(i.e. at least one *b*=0 volume and one *b*>0 shell).
The MSMT algorithm can nevertheless still be utilised with such data
as an alternative to the original :ref:`constrained_spherical_deconvolution` algorithm.

There are two key benefits available:

-   Where the original CSD algorithm imposes a *soft* non-negativity constraint,
    regularising the negative values to be small in magnitude,
    the MSMT CSD algorithm imposes a *hard* non-negativity constraint,
    forbidding negative values entirely.

    To make use of *exclusively* this functionality requires two factors:

    -   The singular input WM tissue response function
        must contain just one row,
        corresponding to the input zonal spherical harmonic coefficients for a single *b*-value.
        This could be achieved either by utilising a response function algorithm
        that intrinsically yields a response function of this form (eg. ``dwi2response tournier``),
        or by manually extracting a single row from a multi-shell response function.

    -   The DWI data to be processed must consist *exclusively* of volumes from a single *b*-value.
        Unlike the ``dwi2fod csd`` algorithm,
        which intrinsically only operates on data with a single *b*-value
        and therefore by default extracts data from those volumes during processing,
        the ``dwi2fod msmt_csd`` algorithm is capable of handling data across multiple *b*-values,
        and therefore intervention is necessary if only data from a single *b*-value is to be used.

        This could be achieved either by explicitly extracting those DWI volumes
        corresponding to the *b*-value of interest prior to executing ``dwi2fod msmt_csd``,
        most likely using the ``dwiextract`` command,
        or by utilising the ``-shells`` option in ``dwi2fod``.

-   Given that "single-shell" data are always accompanied by some number of *b*=0 volumes,
    and the image data within those volumes can be interpreted as a "shell"
    despite not possessing any signal anisotropy,
    it is possible to perform a *two*-tissue decompoisition from single-shell data using the MSMT algorithm
    (unlike the ``dwi2fod csd`` algorithm, which exclusively operates on a single *b*>0 shell).

    It is typically recommended in this instance to utilise WM-like and CSF-like response functions,
    in order to estimate WM-like ODFs 
    that are minimally contaminated from free-water-like contributions to the DWI signal.
