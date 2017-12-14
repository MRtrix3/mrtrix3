Beginner DWI tutorial
=====================

This tutorial will hopefully provide enough information for a novice
user to get from the raw DW image data to performing some streamlines
tractography. It may also be useful for experienced MRtrix users in
terms of identifying some of the new command names.

For all *MRtrix3* scripts and commands, additional information on the
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
for this correct are not yet implemented in *MRtrix3*, though we do provide
a script called ``dwipreproc`` for interfacing with the relevant FSL tools.
Due to the nuances of the operation of this script, the reader is referred
to the :ref:`dwipreproc_page` page.

DWI brain mask estimation
~~~~~~~~~~~~~~~~~~~~~~~~~

In previous versions of MRtrix, a heuristic was used to derive this mask;
a dedicated command is now provided:

.. code::

    $ dwi2mask <Input DWI> <Output mask>
    $ mrview <Input DWI> -roi.load <Output mask>

Note that if you are working with ex-vivo data, this command will likely
not give the desired results. It can also give inconsistent results in
cases of low SNR, strong B1 bias field, or even with good-quality images;
it is recommended that the output of this command should *always* be
checked (and corrected if necessary) before proceeding with further
processing.

Response function estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To perform spherical deconvolution, the DWI signal emanating from a
single coherently-oriented fibre bundle must be estimated. We provide a
script for doing this, which has :ref:`a range of algorithms and
parameters <response_fn_estimation>`. This example will use
fairly sensible defaults:

.. code::

    $ dwi2response tournier <Input DWI> <Output response text file>
    $ shview <Output response text file>

Fibre Orientation Distribution estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This command performs Constrained Spherical Deconvolution (CSD) based on
the response function estimated previously.

.. code::

    $ dwi2fod csd <Input DWI> <Input response text file> <Output FOD image> -mask <Input DWI mask>
    $ mrview <Input DWI> -odf.load_sh <Output FOD image>

Whole-brain streamlines tractography
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For the sake of this tutorial, we will perform whole-brain streamlines
tractography, using default reconstruction parameters.

.. code::

    $ tckgen <Input FOD image> <Output track file> -seed_image <Input DWI mask> -mask <Input DWI mask> -select <Number of tracks>
    $ mrview <Input DWI> -tractography.load <Output track file>

Note: Loading a very large number of tracks can inevitably make the ``mrview`` software run very slowly. When this occurs, it may be preferable to instead view only a subset of the generated tracks, e.g.:

.. code::

    $ tckedit <Track file> <Smaller track file> -number <Smaller number of tracks>
    $ mrview <Input DWI> -tractography.load <Smaller track file>

Track Density Imaging (TDI)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

TDI can be useful for visualising the results of tractography,
particularly when a very large number of streamlines is generated.

.. code::

    $ tckmap <Input track file> <Output TDI> -vox <Voxel size in mm>
    $ mrview <Output TDI>

