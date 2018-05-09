.. _global_tractography:

Global tractography
===================

Introduction
------------

Global tractography is the process of finding the full track
configuration that best explains the measured DWI data. As opposed to
streamline tracking, global tractography is less sensitive to noise, and
the density of the resulting tractogram is directly related to the data
at hand.

As of version 3.0, MRtrix supports global tractography using a
multi-tissue spherical convolution model, as introduced in [Christiaens2015]_.
This method extends the method of [Reisert2011]_ to multi-shell response
functions, estimated from the data, and adopts the multi-tissue model
presented in [Jeurissen2014]_ to account for partial voluming.





User guide
----------

Prerequisites
^^^^^^^^^^^^^

This global tractography implementation relies on *multi-shell* high angular
resolution diffusion imaging (HARDI) data, containing at least 3 unique
*b*-values (i.e 2 shells along with the *b*\=0 volumes). 

In addition, this command expects that suitable *multi-shell multi-tissue
response functions* have already been computed. A number of approaches are
available for this, please refer to the :ref:`response_function_estimation`
page for details.



Invocation
^^^^^^^^^^

For multi-shell DWI data, the most common use will be:

.. code-block:: console

   tckglobal dwi.mif wm_response.txt -riso csf_response.txt -riso gm_response.txt -mask mask.mif -niter 1e9 -fod fod.mif -fiso fiso.mif tracks.tck

In this example, ``dwi.mif`` is the input dataset, including the
gradient table, and ``tracks.tck`` is the output tractogram. ``wm_response.txt``, 
``gm_response.txt`` and ``csf_response.txt`` are the corresponding tissue
response functions (as estimated in a previous
:ref:`response_function_estimation` step). 
Optional output images ``fod.mif`` and ``fiso.mif`` contain the 
predicted WM fODF and isotropic tissue fractions of CSF and GM 
respectively, estimated as part of the global optimization and thus 
affected by spatial regularization. 



Parameters
~~~~~~~~~~

``-niter``: The number of iterations in the optimization. Although the
default value is deliberately kept low, a full brain reconstruction will
require at least 100 million iterations.

``-lmax``: Maximal order of the spherical harmonics basis.

``-length``: Length of each track segment (particle), which determines
the resolution of the reconstruction.

``-weight``: Weight of each particle. Decreasing its value by a factor
of two will roughly double the number of reconstructed tracks, albeit at
increased computation time.

Particle potential ``-ppot``: The particle potential essentially
associates a *cost* to each particle, relative to its weight. As such,
we are in fact trying to reconstruct the data as well as possible, with
as few particles as needed. This ensures that there is sufficient
*proof* for each individual particle, and hence avoids that a bit of
noise in the data spurs generation of new (random) particles. Think of
it as a parameter that balances sensitivity versus specificity. A higher
particle potential requires more *proof* in the data and therefore leads
to higher specificity; a smaller value increases sensitivity.

Connection potential ``-cpot``: The connection potential is the driving
force for connecting segments and hence building tracks. Higher values
increase connectivity, at the cost of increased invalid connections.




Ancillary outputs
~~~~~~~~~~~~~~~~~

``-fod``: Outputs the predicted fibre orientation distribution function 
(fODF) as an image of spherical harmonics coefficients. 
This fODF is estimated as part of the global track optimization, and
therefore incorporates the spatial regularization that it imposes.
Internally, the fODF is represented as a discrete sum of apodized point
spread functions (aPSF) oriented along the directions of all particles in
the voxel, akin to track orientation distribution imaging (TODI, 
[Dhollander2014]_). This internal representation 
is used to predict the DWI signal upon every change to the particle 
configuration.

``-fiso``: Outputs the estimated density of all isotropic tissue
components, as multiple volumes in one 4-D image in the same order as
their respective ``-riso`` kernels were provided.

``-eext``: Outputs the residual data energy image, including the
L1-penalty imposed by the particle potential.


