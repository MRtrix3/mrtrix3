.. _single_tissue_csd:

Single-tissue constrained spherical deconvolution
=================================================

Introduction
------------

Single-tissue Constrained Spherical Deconvolution (CSD) estimates the fibre
Orientation Distribution Function (fODF) based on an estimate of the signal
expected for a single-fibre population (the so-called *response function*) [Tournier2007]_.
This is used as the kernel in a deconvolution operation to extract the fODF
from dMRI signal measured within each voxel. 

User guide
----------


Prerequisites
^^^^^^^^^^^^^

Single-tissue CSD relies on *single-shell* high angular resolution diffusion
imaging (HARDI) data, containing at least one non-zero *b*-value. Ideally, the
*b*-value used should be in the region of 2,500 -- 3,000 s/mm² (for *in vivo*
human brains), although good results can still be obtained using *b* =
1000 s/mm² data.

In addition, this command expects that a suitable *single-shell single-tissue
response function* has already been computed.
Please refer to the :ref:`response_function_estimation` page for details.


Invocation
^^^^^^^^^^

Single-tissue CSD can be performed as:

.. code-block:: console

  dwi2fod csd dwi.mif response.txt fod.mif

where:

- ``dwi.mif`` is the dwi data set (input)

- ``response.txt`` is the response function (input)

- ``fod.mif`` is the resulting fODF (output)

Typically, you will also want to use the ``-mask`` option to avoid unnecessary computations in non-brain voxels:

.. code-block:: console

   dwi2fod csd -mask mask.mif dwi.mif response.txt fod.mif

The resulting WM fODFs can be displayed together with the mean fODF maps using:

.. code-block:: console

   mrview fod.mif -odf.load_sh fod.mif

