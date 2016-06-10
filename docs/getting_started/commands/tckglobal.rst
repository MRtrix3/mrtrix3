.. _tckglobal:

tckglobal
===========

Synopsis
--------

::

    tckglobal [ options ]  source response tracks

-  *source*: the image containing the raw DWI data.
-  *response*: the response of a track segment on the DWI signal.
-  *tracks*: the output file containing the tracks generated.

Description
-----------

Multi-Shell Multi-Tissue Global Tractography.

This command will reconstruct the global white matter fibre tractogram that best explains the input DWI data, using a multi-tissue spherical convolution model.

Example use: 

 $ tckglobal dwi.mif wmr.txt -riso csfr.txt -riso gmr.txt -mask mask.mif    -niter 1e8 -fod fod.mif -fiso fiso.mif tracks.tck 

in which dwi.mif is the input image, wmr.txt is an anisotropic, multi-shell response function for WM, and csfr.txt and gmr.txt are isotropic response functions for CSF and GM. The output tractogram is saved to tracks.tck; ancillary output images fod.mif and fiso.mif contain the WM fODF and isotropic tissue fractions of CSF and GM respectively.

Options
-------

Input options
^^^^^^^^^^^^^

-  **-grad scheme** specify the diffusion encoding scheme (required if not supplied in the header).

-  **-mask image** only reconstruct the tractogram within the specified brain mask image.

-  **-riso response** set one or more isotropic response functions. (multiple allowed)

Parameters
^^^^^^^^^^

-  **-lmax order** set the maximum harmonic order for the output series. (default = 8)

-  **-length size** set the length of the particles (fibre segments). (default = 1mm)

-  **-weight w** set the weight by which particles contribute to the model. (default = 0.1)

-  **-ppot u** set the particle potential, i.e., the cost of adding one segment, relative to the particle weight. (default = 0.05)

-  **-cpot v** set the connection potential, i.e., the energy term that drives two segments together. (default = 0.5)

-  **-t0 start** set the initial temperature of the metropolis hastings optimizer. (default = 0.1)

-  **-t1 end** set the final temperature of the metropolis hastings optimizer. (default = 0.001)

-  **-niter n** set the number of iterations of the metropolis hastings optimizer. (default = 10M)

Output options
^^^^^^^^^^^^^^

-  **-fod odf** filename of the resulting fODF image.

-  **-noapo** disable spherical convolution of fODF with apodized PSF.

-  **-fiso iso** filename of the resulting isotropic fractions image.

-  **-eext eext** filename of the resulting image of the residual external energy.

-  **-etrend stats** internal and external energy trend and cooling statistics.

Advanced parameters, if you really know what you're doing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-balance b** balance internal and external energy. (default = 0Negative values give more weight to the internal energy, positive to the external energy.

-  **-density lambda** set the desired density of the free Poisson process. (default = 1)

-  **-prob prob** set the probabilities of generating birth, death, randshift, optshift and connect probabilities respectively. (default = 0.25,0.05,0.25,0.1,0.35)

-  **-beta b** set the width of the Hanning interpolation window. (in [0, 1], default = 0)If used, a mask is required, and this mask must keep at least one voxel distance to the image bounding box.

-  **-lambda lam** set the weight of the internal energy directly. (default = 1)If provided, any value of -balance will be ignored.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Christiaens, D.; Reisert, M.; Dhollander, T.; Sunaert, S.; Suetens, P. & Maes, F. Global tractography of multi-shell diffusion-weighted imaging data using a multi-tissue model. NeuroImage, 2015, 123, 89-101

--------------



**Author:** Daan Christiaens (daan.christiaens@kuleuven.be)

**Copyright:** Copyright (C) 2015 KU Leuven, Dept. Electrical Engineering, ESAT/PSI,
Herestraat 49 box 7003, 3000 Leuven, Belgium 

This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

