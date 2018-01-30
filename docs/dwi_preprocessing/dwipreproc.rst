.. _dwipreproc_page:


DWI distortion correction using dwipreproc
==========================================

The ``dwipreproc`` script, responsible for performing general pre-processing of
DWI series, has been completely re-designed as part of the *MRtrix3*
``3.0_RC1`` update. Although the 'guts' of the script are completely new, the
fundamental operation - eddy current-induced distortion correction, motion
correction, and (optionally) susceptibility-induced distortion correction,
using FSL's ``eddy`` / ``topup`` / ``applytopup`` tools, remains the same.
While the user interface remains reasonably similar to that provided
previously (examples to come), they are slightly different.

The major benefit of the new design is that *MRtrix3* is now capable of not
only *capturing the relevant phase encoding information* from DICOM headers,
but also *using that information* within ``dwipreproc`` to internally generate
the necessary phase encoding table files in order to run these FSL tools. This
comes with a number of benefits:

- It makes it possible to acquire and process a wider range of DWI acquisition
  designs, without requiring that the user laboriously manually construct the
  phase encoding tables that these FSL tools require.

- It means that automated pre-processing pipelines (e.g. these two `works
  <https://github.com/BIDS-Apps/FibreDensityAndCrosssection>`__\-in-\ `progress
  <https://github.com/BIDS-Apps/MRtrix3_connectome>`__) can be applied to
  provided data without requiring manual intervention to specify this
  information.

- Over time, as *MRtrix* 0.3.16 code is used to import DICOMs (and hence
  capture the phase encoding information) and the relevant code is thoroughly
  tested, there will be less onus on users to track and specify the type of
  phase encoding acquisition performed.

.. NOTE::

  Although the ``dwipreproc`` script is provided as part of *MRtrix3* in the
  hope that users will find it useful, the major image processing steps
  undertaken by this script are still performed using tools developed at FMRIB
  and provided as part of FSL. It is therefore *essential* that the appropriate
  references be cited whenever this script is used!

The ``dwipreproc`` script now has four major 'modes' of operation, that can be
selected at the command-line using the ``-rpe_*`` options. Note that exactly
*one* of these options ***must*** be provided. The following are example use
cases; specific parameters, file names etc. may need to be altered to reflect
your particular data.

1. **No variation in phase encoding**

  All DWI volumes are acquired with precisely the same phase encoding direction
  and EPI readout time, and no additional spin-echo *b*\=0 images are acquired
  over and above those interleaved within the DWI acquisition. It is therefore
  impossible to estimate the inhomogeneity field using ``topup``, and ``eddy``
  will perform motion and eddy current correction only.

  *Example DICOM image data*:

  .. code-block:: console

       002_-_DWI_phaseAP/

  *Old usage* (i.e. prior to *MRtrix* 0.3.16):

  .. code-block:: console

       $ dwipreproc AP 002_-_DWI_phaseAP/ dwi_preprocessed.mif -rpe_none

  *New usage*:

  .. code-block:: console

       $ dwipreproc 002_-_DWI_phaseAP/ dwi_preprocessed.mif -rpe_none -pe_dir AP [ -readout_time 0.1 ]

  Note that here (and in subsequent examples), providing the EPI readout time
  manually is optional (if omitted, the 'sane' default of 0.1s will be
  assumed). The precise scaling of this parameter is not expected to influence
  results provided that the readout time is equivalent for all *b*\=0 / DWI
  volumes.

2. **Reversed phase encode b=0 pair(s)**

  All DWI volumes are acquired with precisely the same phase encoding direction
  and EPI readout time. In addition, one or more pairs of spin-echo b=0 EPI
  volumes are provided, where half of these volumes have the same phase
  encoding direction and readout time as the DWIs, and the other half have
  precisely the *opposite* phase encoding direction (but the same readout
  time). These additional images are therefore used to estimate the
  inhomogeneity field, but do not form part of the output DWI series.

  *Example DICOM image data*:

  .. code-block:: console

        002_-_ep2dse_phaseAP/
        003_-_ep2dse_phasePA/
        004_-_DWI_phaseAP/

  *Old usage* (i.e. prior to *MRtrix* 0.3.16):

  .. code-block:: console

        $ dwipreproc AP 004_-_DWI_phaseAP/ dwi_preprocessed.mif -rpe_pair 002_-_ep2dse_phaseAP/ 003_-_ep2dse_phasePA/

  *New usage*:

  .. code-block:: console

        $ mrcat 002_-_ep2dse_phaseAP/ 003_-_ep2dse_phasePA/ b0s.mif -axis 3
        $ dwipreproc 004_-_DWI_phaseAP/ dwi_preprocessed.mif -pe_dir AP -rpe_pair -se_epi b0s.mif [ -readout_time 0.1 ]

3. **Reversed phase encoding for all DWIs**

  For all diffusion gradient directions & *b*-values, two image volumes are
  obtained, with the opposite phase encoding direction with respect to one
  another. This allows for the combination of the two volumes corresponding to
  each unique diffusion gradient direction & strength into a single volume,
  where the relative compression / expansion of signal between the two volumes
  is exploited.

  *Example DICOM image data*:

  .. code-block:: console

        002_-_DWI_64dir_phaseLR/
        003_-_DWI_64dir_phaseRL/

  *Old usage* (i.e. prior to *MRtrix* 0.3.16):

  .. code-block:: console

        $ dwipreproc LR 002_-_DWI_64dir_phaseLR/ dwi_preprocessed.mif -rpe_all 003_-_DWI_64dir_phaseRL/

  *New usage*:

  .. code-block:: console

        $ mrcat 002_-_DWI_64dir_phaseLR/ 003_-_DWI_64dir_phaseRL/ all_DWIs.mif -axis 3
        $ dwipreproc all_DWIs.mif dwi_preprocessed.mif -pe_dir LR -rpe_all [ -readout_time 0.1 ]

  Note that in this particular example, the dwipreproc script will in fact
  extract the *b*\=0 volumes from the input DWIs and use those to estimate the
  inhomogeneity field with topup. If additional *b*\=0 images are also acquired,
  and it is desired to instead use those images to estimate the inhomogeneity
  field only, the ``-se_epi`` option can be used.

4. **Arbitrary phase encoding acquisition**

  In cases where either:

  - An up-to-date version of *MRtrix3* has been used to convert from DICOM,
    such that phase encoding information is embedded in the image header; or:

  - Image data of unknown origin are to be processed by an automated pipeline
    without user intervention, and therefore phase encoding information must be
    provided using data files associated with the input images (such as `JSON
    <http://www.json.org/>`_ files in the `BIDS standard
    <http://bids.neuroimaging.io/>`_),

  it is possible for the ``dwipreproc`` script to automatically determine the
  appropriate steps to perform based on the phase encoding configuration of the
  image data presented to it.

  *Usage*:

  .. code-block:: console

        $ mrcat <all_input_DWIs> all_dwis.mif -axis 3
        $ mrcat <all_extra_b=0_volumes> all_b0s.mif -axis 3   (optional)
        $ dwipreproc all_dwis.mif dwi_preprocessed.mif -rpe_header [ -se_epi all_b0s.mif ]

  .. WARNING::

    With regards to Option 4 (using phase encoding information as it is stored
    in the header), note that this functionality is preliminary and should not
    be trusted blindly. It is impossible for us to check and test all possible
    usage scenarios. Furthermore, if this information is imported or exported
    to/from the image header, this requires reorientation due to the way in
    which *MRtrix3* handles image orientations internally, which introduces
    additional mechanisms by which the tracking of phase encoding orientations
    may go awry. Results should therefore be checked manually if using /
    testing this mechanism.

When one of the options 1-3 are used, internally the ``dwipreproc`` script
*generates the effective phase encoding table* given the user's images and
command-line input; this is what is passed to ``topup`` / ``applytopup`` /
``eddy``. If one of these options is used, but there is actually phase encoding
information found within the image header(s), the script will *compare* the
user's phase encoding specification against the header contents, and produce a
warning if it detects a mismatch (since either the phase encoding design is not
what you think it is, or the import of phase encoding information from DICOM is
awry; either warrants further investigation).


Using ``eddy``'s slice-to-volume correction capability
------------------------------------------------------

As of September 2017, FSL's ``eddy`` tool has the capability of not only
estimating and correcting motion _between_ DWI volumes, but also motion
_within_ volumes. Details of this method can be found within the relevant
`publication <https://www.sciencedirect.com/science/article/pii/S1053811917301945>`__.
*MRtrix3* is capable of supporting this underlying `eddy`` functionality
within the wrapping ``dwipreproc`` script. Below are a few relevant details
to assist users in getting this working:

-  At time of writing, only the CUDA version of the ``eddy`` executable
   provides the slice-to-volume correction capability. Therefore, this
   version must be installed on your system, CUDA itself must be appropriately
   set up, and ``dwipreproc`` must be provided with the ``-cuda`` command-line
   option.

-  ``eddy``'s slice-to-volume correction is triggered by the presence of the
   ``--mporder=#`` command-line option. Therefore, to trigger this behaviour,
   the contents of the ``-eddy_options`` command-line option passed to
   ``dwipreproc`` must additionally contain this entry.

-  The timing of acquisition of each slice must be known in order to perform
   slice-to-volume correction. This is provided to ``eddy`` via the
   ` ``--slspec`` <https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/eddy/UsersGuide#A--slspec>`__
   command-line option, where a text file is provided that defines the order
   in which the slices are acquired within each volume. In ``dwipreproc``,
   there are two ways in which this information can be provided:

   -  If you include the string ``--slspec=path/to/file.txt`` within the
      contents of the ``--eddy_options`` command-line option, then ``dwipreproc``
      will find the file to which you have provided the path, and pass that
      file through to ``eddy``.

   -  If DICOM conversion & all subsequent processing is performed solely
      using *MRtrix3* commands, and _header_keyvalue_pairs_ are preserved,
      then ``dwipreproc`` will use the fields "``SliceEncodingDirection``"
      and "``SliceTiming``" captured during DICOM import, and use these to
      internally generate the "``slspec``" file required by ``eddy``. Note
      that the naming of these fields is consistent with the
      `BIDS specification <http://bids.neuroimaging.io/>`__.
