Basic DWI processing
====================

This tutorial will hopefully provide enough information for a novice
user to get from the raw DW image data to performing some streamlines
tractography. It may also be useful for experienced MRtrix users in
terms of identifying some of the new command names.

For all MRtrix scripts and commands, additional information on the
command usage and available command-line options can be found by
invoking the command with the ``-help`` option. Note that this tutorial
includes commands and scripts for which there are relevant journal
articles for citation; these are listed on the help pages also.

DWI geometric distortion correction
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If the user has access to reversed phase-encode spin-echo image data,
this can be used to correct the susceptibility-induced geometric
distortions present in the diffusion images, as well as any eddy
current-induced distortions and inter-volume subject motion. Procedures
for this correct are not yet implemented in MRtrix, though we do provide
a script for interfacing with the relevant FSL tools:

``dwipreproc <Input DWI series> <Output corrected DWI series>``

For more details, see the header of the ``scripts/dwipreproc`` file. In
particular, it is necessary to manually specify what type of reversed
phase-encoding acquisition has taken place (if any), and provide the
relevant input images.

DWI brain mask estimation
~~~~~~~~~~~~~~~~~~~~~~~~~

In previous versions of MRtrix a heuristic was used to derive this mask;
a dedicated command is now provided:

``dwi2mask <Input DWI> <Output mask>``

Note that if you are working with ex-vivo data, this command will likely
not give the desired results.

Response function estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To perform spherical deconvolution, the DWI signal emanating from a
single coherently-oriented fibre bundle must be estimated. We provide a
script for doing this, which has `a range of algorithms and
parameters <Response-function-estimation>`__. This example will use
fairly sensible defaults:

``dwi2response tournier <Input DWI> <Output response text file>``

Fibre Orientation Distribution estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This command performs Constrained Spherical Deconvolution (CSD) based on
the response function estimated previously.

``dwi2fod <Input DWI> <Input response text file> <Output FOD image> -mask <Input DWI mask>``

Whole-brain streamlines tractography
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For the sake of this tutorial, we will perform whole-brain streamlines
tractography, using default reconstruction parameters.

``tckgen <Input FOD image> <Output track file> -seed_image <Input DWI mask> -mask <Input DWI mask> -number <Number of tracks>``

Track Density Imaging (TDI)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

TDI can be useful for visualising the results of tractography,
particularly when a very large number of streamlines is generated.

``tckmap <Input track file> <Output TDI> -vox <Voxel size in mm>``

