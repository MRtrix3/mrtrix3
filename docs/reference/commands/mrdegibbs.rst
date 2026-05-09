.. _mrdegibbs:

mrdegibbs
===================

Synopsis
--------

Remove Gibbs ringing artefacts

Usage
--------

::

    mrdegibbs [ options ]  in out

-  *in*: the input image.
-  *out*: the output image (at the input image's resolution by default; at twice the in-plane resolution if -method tgv -superresolution is given).

Description
-----------

This command removes Gibbs ringing artefacts from MRI images. Two families of methods are provided, selected via the -method option.

The local subvoxel-shift methods ("kellner" and "bautista") use the approach proposed by Kellner et al. The default "kellner" choice is the original slice-wise 2D formulation; "bautista" selects the 3D volume-wise extension proposed by Bautista et al. (see references below).

The "tgv" method targets each 2D slice independently by reconstructing it at twice the in-plane spatial resolution under a regularised inverse problem. The data fidelity term enforces consistency between the input k-space and the central window of the high-resolution Fourier transform of the reconstruction; the regulariser is second-order total generalised variation (TGV2) on the symmetrised spatial derivative. Unlike the local subvoxel-shift methods, this approach extrapolates the unsampled high frequencies of the slice, yielding an image at double the in-plane resolution. The cost is convex; minimisation uses a first-order primal-dual (Chambolle-Pock) algorithm whose data-term proximal step is diagonal in high-resolution k-space. By default, each high-resolution reconstruction is downsampled back to the input resolution by direct 2x2 voxel aggregation, so that the output image matches the input grid; the -superresolution option exports the high-resolution reconstruction directly (in-plane sizes doubled, voxel spacing halved, affine origin shifted by half a voxel along each in-plane axis so that the field-of-view is preserved).

This command is designed to run on data directly after it has been reconstructed by the scanner, before any interpolation of any kind has taken place. You should not run this command after any form of motion correction (e.g. not after dwifslpreproc). If however you intend to run a thermal denoising step (eg. dwidenoise), you should do so before this command to not alter the noise structure, which would impact on denoising performance.

For best results, any form of filtering performed by the scanner should be disabled, whether performed in the image domain or k-space. This includes elliptic filtering and other filters  that are often applied to reduce Gibbs ringing artifacts. While this method can still safely be applied to such data, some residual ringing artefacts may still be present in the output.

Note that these methods are designed to work on images acquired with full k-space coverage. If executed on data acquired with partial Fourier (eg. "half-scan") acceleration, they may not fully remove all ringing artifacts, and you may observe residuals of the original artifact in the partial Fourier direction. Nonetheless, application of the method is still considered safe and worthwhile. Users are however encouraged to acquired full-Fourier data where possible.

As these methods are based on utilisation of the Fourier shift theorem, they operate best if provided with complex-valued image data; in this use case the output image will also be complex-valued. The "tgv" method requires complex-valued input, and always produces complex-valued output.

Options
-------

Method selection
^^^^^^^^^^^^^^^^

-  **-method choice** select the algorithm used to remove Gibbs ringing (one of: kellner, bautista, tgv; default: kellner). The "kellner" choice selects the original 2D subvoxel-shift method by Kellner et al.; "bautista" selects its 3D volume-wise extension by Bautista et al.; "tgv" selects 2D TGV-regularised super-resolution k-space extrapolation.

-  **-axes list** select the in-plane (slice) axes for the 2D methods (kellner, tgv) (default: 0,1 - i.e. x-y); ignored for the bautista method.

Options for the local subvoxel-shift methods (kellner, bautista)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-nshifts value** discretization of subpixel spacing (default: 20).

-  **-minW value** left border of window used for TV computation (default: 1).

-  **-maxW value** right border of window used for TV computation (default: 3).

Options for the TGV super-resolution method (tgv)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-superresolution** export the super-resolution reconstruction (in-plane sizes doubled, voxel sizes halved, affine origin shifted by half a voxel along each in-plane axis to preserve the field of view) rather than the default LR result obtained by 2x2 voxel aggregation.

-  **-mu value** data-fidelity / regularisation balance coefficient (default: 5e-3).

-  **-alpha list** comma-separated TGV2 weights (alpha1, alpha0) on the gradient and symmetric-derivative terms respectively (default: 1.0,2.0).

-  **-window choice** k-space apodisation window applied during cropping (default: rect).

-  **-niter value** maximum primal-dual iterations per slice (default: 500).

-  **-tol value** relative tolerance on primal change for early termination (default: 1e-4).

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices are: float16, float16le, float16be, float32, float32le, float32be, float64, float64le, float64be, int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32, int32le, uint32le, int32be, uint32be, int16, uint16, int16le, uint16le, int16be, uint16be, cfloat16, cfloat16le, cfloat16be, cfloat32, cfloat32le, cfloat32be, cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Kellner, E; Dhital, B; Kiselev, V.G & Reisert, M. Gibbs-ringing artifact removal based on local subvoxel-shifts. Magnetic Resonance in Medicine, 2016, 76, 1574-1581.

Bautista, T; O'Muircheartaigh, J; Hajnal, JV; & Tournier, J-D. Removal of Gibbs ringing artefacts for 3D acquisitions using subvoxel shifts. Proc. ISMRM, 2021, 29, 3535.

Knoll, F.; Bredies, K.; Pock, T.; & Stollberger, R. Second order total generalized variation (TGV) for MRI. Magnetic Resonance in Medicine, 2011, 65, 480-491.

Bredies, K.; Kunisch, K.; & Pock, T. Total generalized variation. SIAM Journal on Imaging Sciences, 2010, 3, 492-526.

Chambolle, A. & Pock, T. A first-order primal-dual algorithm for convex problems with applications to imaging. Journal of Mathematical Imaging and Vision, 2011, 40, 120-145.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Ben Jeurissen (ben.jeurissen@uantwerpen.be), J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2026 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozilla Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.


