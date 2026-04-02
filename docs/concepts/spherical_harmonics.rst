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
