.. _constrained_spherical_deconvolution:

Constrained spherical deconvolution
===================================

Introduction
------------

Constrained Spherical Deconvolution (CSD) [Tournier2007]_ estimates a
white matter fibre Orientation Distribution Function (fODF) based on an
estimate of the signal expected for a single-fibre white matter population (the
so-called *response function*). This is used as the kernel in a deconvolution
operation to extract a white matter fODF from dMRI signal measured within
each voxel. 

User guide
----------


Prerequisites
^^^^^^^^^^^^^

Constrained Spherical Deconvolution as defined in [Tournier2007]_ relies on
*single-shell* high angular resolution diffusion imaging (HARDI) data,
containing at least one non-zero *b*-value. Ideally, the *b*-value used
should be in the region of 2,500 -- 3,000 s/mm² (at least for *in vivo*
human brains), although good results have sometimes been obtained using
*b* = 1000 s/mm² data.

In addition, this command expects that a suitable *single-shell single-tissue
response function* has already been computed.
Please refer to the :ref:`response_function_estimation` page for details.


Invocation
^^^^^^^^^^

Constrained Spherical Deconvolution can be performed as:

.. code-block:: console

  dwi2fod csd dwi.mif response.txt fod.mif

where:

- ``dwi.mif`` is the dwi data set (input)

- ``response.txt`` is the response function (input)

- ``fod.mif`` is the resulting fODF (output)

Typically, you will also want to use the ``-mask`` option to avoid unnecessary computations in non-brain voxels:

.. code-block:: console

   dwi2fod csd -mask mask.mif dwi.mif response.txt fod.mif

The resulting WM fODFs can be displayed together with the mean fODF amplitude map using:

.. code-block:: console

   mrview fod.mif -odf.load_sh fod.mif

