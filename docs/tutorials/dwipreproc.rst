DWI distortion correction using ``dwipreproc``
==============================================

The ``dwipreproc`` script, responsible for performing general pre-processing of DWI series, has been completely re-designed as part of the *MRtrix3* 0.3.16 update. Although the 'guts' of the script are completely new, the fundamental operation - eddy current-induced distortion correction, motion correction, and (optionally) susceptibility-induced distortion correction, using FSL's ``eddy`` / ``topup`` / ``applytopup`` tools, remains the same. While the user interface remains reasonably similar to that provided previously (examples to come), they are slightly different.

The major benefit of the new design is that *MRtrix3* is now capable of not only *capturing the relevant phase encoding information* from DICOM headers, but also *using that information* within ``dwipreproc`` to internally generate the necessary phase encoding table files in order to run these FSL tools. This makes it possible to acquire and process a wider range of DWI acquisition designs, without requiring that the user laboriously construct the phase encoding tables that these FSL tools require. It also means that automated pre-processing pipelines (e.g. these two `works <https://github.com/BIDS-Apps/FibreDensityAndCrosssection>`_ -in- `progress <https://github.com/BIDS-Apps/MRtrix3_connectome>`_ ) can be applied to provided data without requiring manual intervention to specify this information.

  .. NOTE::
  
  Although the ``dwipreproc`` script is provided as part of *MRtrix3* in the hope that users will find it useful, the major image processing steps undertaken by this script are still performed using tools developed at FMRIB and provided as part of FSL. It is therefore *essential* that the appropriate references be cited whenever this script is used!

The ``dwipreproc`` script now has four major 'modes' of operation, that can be selected at the command-line using the ``-rpe_*`` options:

1. **No variation in phase encoding**

  All DWI volumes are acquired with precisely the same phase encoding direction and EPI readout time, and no additional spin-echo *b*=0 images are acquired over and above those interleaved within the DWI acquisition. It is therefore impossible to estimate the inhomogeneity field using ``topup``, and `eddy` will perform motion and eddy current correction only.

  *Old usage* (i.e. prior to *MRtrix* 0.3.16):

    dwipreproc AP <input_DWI> <output_DWI> -rpe_none

  *New usage*:

    dwipreproc <input_DWI> <output_DWI> -rpe_none -pe_dir AP [-readout_time 0.1]

2. **Reversed phase encode *b*=0 pair(s)**

  All DWI volumes are acquired with precisely the same phase encoding direction and EPI readout time. In addition, one or more pairs of spin-echo *b*=0 EPI volumes are provided, where half of these volumes have the same phase encoding direction and readout time as the DWIs, and the other half have precisely the *opposite* phase encoding direction (but the same readout time). These additional images are therefore used to estimate the inhomogeneity field, but do not form part of the output DWI series.

  *Old usage* (i.e. prior to *MRtrix* 0.3.16):

    dwipreproc AP <input_DWI> <output_DWI> -rpe_pair <AP_b=0_image(s)> <PA_b=0_image(s)>

  *New usage*:

    mrcat <AP_b=0_image(s)> <PA_b=0_image(s)> b0s.mif -axis 3
    dwipreproc <input_DWI> <output_DWI> -pe_dir AP -rpe_pair -se_epi b0s.mif [-readout_time 0.1]

3. Reversed phase encoding for all DWIs
  For all diffusion gradient directions & *b*-values, two image volumes are obtained, with the opposite phase encoding direction with respect to one another. This allows for the combination of the two volumes corresponding to each unique diffusion gradient direction & strength into a single volume, where the relative compression / expansion of signal between the two volumes is exploited.

  *Old usage* (i.e. prior to *MRtrix* 0.3.16):

    dwipreproc AP <AP_input_DWI> <output_DWI> -rpe_all <PA_input_DWI>

  *New usage*:

    mrcat <AP_input_DWI> <PA_input_DWI> all_DWIs.mif -axis 3
    dwipreproc all_DWIs.mif <output_DWI> -pe_dir AP -rpe_all [-readout_time 0.1]
    
  Note that in this particular example, the dwipreproc script will in fact extract the *b*=0 volumes from the input DWIs and use those to estimate the inhomogeneity field with topup. If additional *b*=0 images are also acquired, and it is desired to instead use those images to estimate the inhomogeneity field only, the ``-se_epi`` option can be used.

4. Arbitrary phase encoding acquisition

  In cases where either: 

  - An up-to-date version of *MRtrix3* has been used to convert from DICOM, such that phase encoding information is embedded in the image header; or:

  - Image data of unknown origin are to be processed by an automated pipeline without user intervention, and therefore phase encoding information must be provided using data files associated with the input images (such as `JSON <http://www.json.org/>`_ files in the `BIDS standard <http://bids.neuroimaging.io/>`_);

  , it is possible for the ``dwipreproc`` script to automatically determine the appropriate steps to perform based on the phase encoding configuration of the image data presented to it.

  *Usage*:

    dwipreproc <all_input_DWIs> <output_DWI> -rpe_header [-se_epi <extra_b=0_volumes>]

  .. WARNING::

    With regards to Option 4 (using phase encoding information as it is stored in the header), note that this functionality is preliminary and should not be trusted blindly. It is impossible for us to check and test all possible usage scenarios. Furthermore, if this information is imported or exported to/from the image header, this requires reorientation due to the way in which *MRtrix3* handles image orientations internally, which introduces additional mechanisms by which the tracking of phase encoding orientations may go awry. Results should therefore be checked manually if using / testing this mechanism.

When one of the options 1-3 are used, internally the ``dwipreproc`` script *generates the effective phase encoding table* given the user's images and command-line input; this is what is passed to ``topup`` / ``applytopup`` / ``eddy``. If one of these options is used, but there is actually phase encoding information found within the image header(s), the script will *compare* the user's phase encoding specification against the header contents, and produce a warning if it detects a mismatch (since either the phase encoding design is not what you think it is, or the import of phase encoding information from DICOM is awry; either warrants further investigation).

