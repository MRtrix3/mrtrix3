Orthonormal Spherical Harmonic basis
====================================

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
-----------

For Spherical Deconvolution (SD) as implemented in MRtrix, processing is
done in the Spherical Harmonic (SH) basis; this mathematical formulation
provides a smooth representation of data distributed on the sphere. When
we do SD, the resulting Fibre Orientation Distributions (FODs) are
written to an image. These FOD images contain coefficients in this SH
basis, that when interpreted correctly, produce the FOD butterflies we
all know and love. If you've ever looked at the raw image volumes from
an FOD image, you'll know that all but the first one are basically not
interpretable.

Here's where it gets tricky. In all previous versions of MRtrix, there
was a 'bug' in the SH basis functions. Mathematically, the basis was
'non-orthonormal'; you don't necessarily need to know what this means,
just appreciate that the formulation of this mathematical basis was not
optimal.

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
------------

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

Problematic data
----------------

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

