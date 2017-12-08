.. _fod2dec:

fod2dec
===================

Synopsis
--------

Generate FOD-based DEC maps, with optional panchromatic sharpening and/or luminance/perception correction

Usage
--------

::

    fod2dec [ options ]  input output

-  *input*: The input FOD image (spherical harmonic coefficients).
-  *output*: The output DEC image (weighted RGB triplets).

Description
-----------

By default, the FOD-based DEC is weighted by the integral of the FOD. To weight by another scalar map, use the outputmap option. This option can also be used for panchromatic sharpening, e.g., by supplying a T1 (or other sensible) anatomical volume with a higher spatial resolution.

Options
-------

-  **-mask image** Only perform DEC computation within the specified mask image.

-  **-threshold value** FOD amplitudes below the threshold value are considered zero.

-  **-outputmap image** Weight the computed DEC map by a provided outputmap. If the outputmap has a different grid, the DEC map is first resliced and renormalised. To achieve panchromatic sharpening, provide an image with a higher spatial resolution than the input FOD image; e.g., a T1 anatomical volume. Only the DEC is subject to the mask, so as to allow for partial colouring of the outputmap. Default when this option is *not* provided: integral of input FOD, subject to the same mask/threshold as used for DEC computation.

-  **-no_weight** Do not weight the DEC map (reslicing and renormalising still possible by explicitly providing the outputmap option as a template).

-  **-lum** Correct for luminance/perception, using default values Cr,Cg,Cb = 0.3,0.5,0.2 and gamma = 2.2 (*not* correcting is the theoretical equivalent of Cr,Cg,Cb = 1,1,1 and gamma = 2).

-  **-lum_coefs values** The coefficients Cr,Cg,Cb to correct for luminance/perception. Note: this implicitly switches on luminance/perception correction, using a default gamma = 2.2 unless specified otherwise.

-  **-lum_gamma value** The gamma value to correct for luminance/perception. Note: this implicitly switches on luminance/perception correction, using a default Cr,Cg,Cb = 0.3,0.5,0.2 unless specified otherwise.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Dhollander T, Smith RE, Tournier JD, Jeurissen B, Connelly A. Time to move on: an FOD-based DEC map to replace DTI's trademark DEC FA. Proc Intl Soc Mag Reson Med, 2015, 23, 1027.

Dhollander T, Raffelt D, Smith RE, Connelly A. Panchromatic sharpening of FOD-based DEC maps by structural T1 information. Proc Intl Soc Mag Reson Med, 2015, 23, 566.

--------------



**Author:** Thijs Dhollander (thijs.dhollander@gmail.com)

**Copyright:** Copyright (C) 2014 The Florey Institute of Neuroscience and Mental Health, Melbourne, Australia. This is free software; see the source for copying conditions. There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

