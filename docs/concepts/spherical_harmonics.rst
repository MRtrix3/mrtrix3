.. _spherical_harmonics:

Spherical Harmonics
===================

For Spherical Deconvolution (SD) as implemented in MRtrix, processing is
done in the Spherical Harmonic (SH) basis; this mathematical formulation
provides a smooth representation of data distributed on the sphere. When
we do SD, the resulting Fibre Orientation Distributions (FODs) are
written to an image. These FOD images contain coefficients in this SH
basis, that when interpreted correctly, produce the FOD butterflies we
all know and love. If you've ever looked at the raw image volumes from
an FOD image, you'll know that all but the first one are basically not
interpretable.

What are Spherical Harmonics?
-----------------------------

Spherical harmonics are special functions defined on the surface of a sphere.
They form a complete orthonormal set and can therefore be used to represent any
well-behaved spherical function. In many ways, they are the equivalent to the
Fourier series for functions defined over spherical (rather than
Cartesian) coordinates. They are defined as:

.. math::

   Y_l^m(\theta,\phi) = \sqrt{\frac{(2l+1)}{4\pi}\frac{(l-m)!}{(l+m)!}} P_l^m(\cos \theta) e^{im\phi}

with integer *order* :math:`l` and *phase* :math:`m`, where :math:`l \geq 0`
and :math:`-l \leq m \leq l` (note that the terms *degree* and *order* are
also commonly used to denote :math:`l` & :math:`m` respectively), and
associated Legendre polynomials :math:`P_l^m`. The harmonic order :math:`l`
corresponds to the angular frequency of the basis function; for example,
all :math:`l=2` SH basis functions feature 2 full oscillations around some
equator on the sphere. The harmonic phase :math:`m` correspond to different
orthogonal modes at this frequency; e.g. where the oscillations occur
around a different plane.

Any well-behaved function on the sphere :math:`f(\theta,\phi)` can be expressed
as its spherical harmonic expansion:

.. math::

   f(\theta,\phi) = \sum_{l=0}^{\infty} \sum_{m=-l}^{l} c_l^m Y_l^m(\theta,\phi)

For smooth functions that have negligible high angular frequency content, the
series can be truncated at some suitable maximum harmonic order
:math:`l_\text{max}` with little to no loss of accuracy:

.. math::

   f(\theta,\phi) = \sum_{l=0}^{l_\text{max}} \sum_{m=-l}^{l} c_l^m Y_l^m(\theta,\phi)

The spherical harmonic series therefore provides a compact represention for
smooth functions on the sphere. Moreover, due to its formulation, it has many
compelling mathematical properties that simplify otherwise complex operations,
including notably spherical (de)convolution.

Formulation used in *MRtrix3*
-----------------------------

Due to the nature of the problems addressed in diffusion MRI, in *MRtrix3* a
simplified version of the SH series is used:

1. The data involved are real (the phase information is invariably discarded
   due to its instability to motion), so we can use a real basis with no
   imaginary components.

2. The problems involved all exhibit antipodal symmetry (i.e. symmetry about
   the origin, :math:`f(\mathbf{x}) = f(-\mathbf{x})`), so we can ignore all
   odd order terms in the series (since these correspond to strictly
   antisymmetric terms).

The SH basis functions :math:`Y_{lm}(\theta,\phi)` used in *MRtrix3* are
therefore:

.. math::

   Y_{lm}(\theta,\phi) = \begin{cases}
   0 & \text{if $l$ is odd}, \\
   \sqrt{2} \: \text{Im} \left[ Y_l^{-m}(\theta,\phi) \right] & \text{if $m < 0$},\\
   Y_l^0(\theta,\phi) & \text{if $m = 0$},\\
   \sqrt{2} \: \text{Re} \left[ Y_l^m(\theta,\phi) \right] & \text{if $m > 0$},\\
   \end{cases}


Storage conventions
^^^^^^^^^^^^^^^^^^^

Images that contain spherical harmonic coefficients are stored as 4-dimensional
images, with each voxel's coefficients stored along the fourth axis. Only the even
degree coefficients are stored (since odd :math:`l` coefficients are assumed to
be zero).

The SH coefficients :math:`c_{lm}` corresponding to the basis functions
:math:`Y_{lm}(\theta,\phi)` are stored in the corresponding image volume at
index :math:`V_{lm}` according to the following equation:

:math:`V_{lm} = \frac{1}{2} l(l+1) + m`

The first few volumes of the image therefore correspond to SH coefficients as follows:

=====================  =========================
Volume :math:`V_{lm}`        :math:`c_{lm}`
=====================  =========================
          0            :math:`l=0`, :math:`m=0`
          1            :math:`l=2`, :math:`m=-2`
          2            :math:`l=2`, :math:`m=-1`
          3            :math:`l=2`, :math:`m=0`
          4            :math:`l=2`, :math:`m=1`
          5            :math:`l=2`, :math:`m=2`
          6            :math:`l=4`, :math:`m=-4`
          7            :math:`l=4`, :math:`m=-3`
         ...                     ...
=====================  =========================

The total number of volumes *N* in the image depends on the highest
angular frequency band included in the series, referred to as the maximal
spherical harmonic order :math:`l_\text{max}`:

:math:`N= \frac{1}{2} (l_\text{max}+1) (l_\text{max}+2)`

====================  =========
:math:`l_\text{max}`  :math:`N`
====================  =========
         0                1
         2                6
         4                15
         6                28
         8                45
        10                66
        12                91
       ...               ...
====================  =========

Representation of response functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The response functions required when performing spherical (de)convolution
correspond to axially symmetric kernels (they typically represent the ideal
signal for a coherently aligned bundle of fibres aligned with the :math:`z`
axis). Due to this symmetry, all :math:`m \neq 0` coefficients can be assumed
to be zero. Therefore, response functions can be fully represented using only
their even order :math:`l`, zero phase :math:`m=0` coefficients (the so-called
*zonal* harmonics).

Response functions are stored in plain text files. A vector of values stored
on one row of such a file are interpreted as these even :math:`l`,
:math:`m=0` terms. The number of coefficients stored for a response function
of a given maximal harmonic order :math:`l_\text{max}` is :math:`1+l_\text{max}/2`.

Response files can contain multiple rows, in which case they are assumed to
represent a *multi-shell* response, with one set of coefficients per *b*-value,
in order of increasing *b*-value (i.e. the first row would normally correspond
to the :math:`b=0` 'shell', with all :math:`l>0` terms set to zero). The
*b*-values themselves are not stored in the response file, but are assumed to
match the values in the DW encoding of the diffusion MRI dataset to be
processed.



Differences with previous version of MRtrix
-------------------------------------------

An important difference between the old (0.2.x) and new (0.3.x and 3.x.x)
versions of MRtrix is a change to the Spherical Harmonic (SH) basis
functions. This change has important consequences for data that may be used
interchangeably between the two versions.

**Important:** note that although it is possible to use and display FODs
generated using MRtrix 0.2.x in the newer *MRtrix3* applications (and
vice-versa), the FODs will *NOT* be correct. Moreover, it is very
difficult to tell the difference by simple visual inspection - the FODs
may still *look* reasonable, but will give incorrect results if used
for tractography or in quantitative analyses. To ensure your images are
correct, you should use the :ref:`shbasis` application included in *MRtrix3*,
as described below.

The problem
^^^^^^^^^^^

Here's where it gets tricky. In all previous versions of MRtrix, there
was a 'bug' in the SH basis functions. Mathematically, the basis was
'non-orthonormal' (although still orthogonal), due to the ommission of the
:math:`\sqrt{2}` terms in the definitions above. You don't necessarily need to
know what this means, just appreciate that this formulation of the
basis, although entirely self-consistent, was not optimal for some
operations.

This 'bug' didn't actually cause any problems; the previous version
of MRtrix was self-consistent in its handling of the issue throughout
the code. It was annoying for any users transferring data between MRtrix
and other packages though. For the release of the new *MRtrix3*, we have
decided to correct the underlying error in the SH basis once and for
all, as there are various mathematical operations that are greatly
simplified when the basis is orthonormal. This does however introduce a
problem for anyone that has done prior image processing using the old
MRtrix 0.2 and wants to be able to use that data with *MRtrix3*: if you
have image data that was generated using the *old* SH basis, but read it
using MRtrix code that was compiled using the *new* SH basis, the data
will *not be interpreted correctly*.

The solution
^^^^^^^^^^^^

There is a solution, but it takes a bit of manual labour on your part.
We have provided a new command called ``shbasis``. This command
will read your image data, and tell you which SH basis it thinks your
image data are stored in (or if it's unable to make this decision).

Furthermore, it includes a command-line option for *changing* the SH
basis of the underlying image data: ``-convert``. The most important
choice for this option is ``-convert native``. This option identifies
the SH basis that *MRtrix3* is compiled for (this is the
new orthonormal basis by default); and if the image data is not
currently stored in this basis, it *modifies the image data in-place* so
that it conforms to the correct basis.

Any data that you generate after this update has occurred will
automatically be produced in the new SH basis, and therefore will not
need to be converted using ``shbasis``. However if you are uncertain
whether or not a particular image does or does not need to be converted,
``shbasis`` can always be used to verify whether or not the image data
are in the correct SH basis; and if you provide the ``-convert native``
option despite the image data already being in the new SH basis, no
modification of the image data will take place.

My recommendation is therefore as follows. When you commit to using the
new version of MRtrix, you should go through *all* of your diffusion
image data on *all* systems that you use, and run
``shbasis -convert native`` on all images that contain spherical
harmonic data (only FOD images; raw DWIs / response functions / TDIs /
etc. do not need to be converted).

Also: Remember that data previously generated will not be
interpreted correctly by *MRtrix3* commands without the SH basis
conversion? The same applies in the other direction. So if you load
FOD images that have either been generated using *MRtrix*, or have
been previously converted using ``shbasis``, commands from the previous
version of MRtrix (0.2) won't interpret them correctly. We hope that
once we have feature completeness in *MRtrix3*, the old version
will no longer be necessary, and therefore this will not be a problem.

Dealing with problematic data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In some circumstances, the ``shbasis`` command will give an error
something like this:

``shbasis [WARNING]: Cannot make unambiguous decision on SH basis of image csd.mif (power ratio regressed to l=0 is 1.58446)``

``shbasis`` uses a data-driven approach to automatically determine the
SH basis that the image data are currently stored in; however a number
of issues can arise that lead to a breakdown of the numerical assumption
that it is based on, and it can no longer make this decision.

If this occurs, but you are confident that your image data are in the
old non-orthonormal basis and need to be converted to the new
orthonormal basis, you can run:
``shbasis <image> -convert force_oldtonew``. This will inform
``shbasis`` that even though it's unable to determine the current SH
basis, you're confident that you do know it, and therefore it should
perform the conversion anyway. It will give you a couple of loud
warnings just to make sure you appreciate the danger in what you're
doing, so you should only ever use this setting for problematic data;
for the vast majority of conversions, ``-convert native`` is much
better.

