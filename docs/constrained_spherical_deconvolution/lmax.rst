Maximum spherical harmonic degree ``lmax``
------------------------------------------

What determines ``lmax`` for my image data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For any command or script operating on data in the spherical harmonic
basis, it should be possible to manually set the maximum harmonic degree
of the output using the ``-lmax`` command-line option. If this is *not*
provided, then an appropriate value will be determined automatically.

The mechanisms by which this automatic determination of ``lmax`` occurs
are as follows:

-  Determine the maximum value for ``lmax`` that is supported by the number
   of DWI volumes in the shell being processed (or the total number of
   non-*b*=0 volumes in a single-shell acquisition). This is the number of
   coefficients required to store an anitipodally-symmetric spherical
   harmonic function:

+------+------------------+
| lmax | Required volumes |
+======+==================+
|    2 | 6                |
+------+------------------+
|    4 | 15               |
+------+------------------+
|    6 | 28               |
+------+------------------+
|    8 | 45               |
+------+------------------+
|   10 | 66               |
+------+------------------+
|   12 | 91               |
+------+------------------+
|  ... | ...              |
+------+------------------+

-  If ``lmax`` exceeds 8, reduce to 8. This is primarily based on the
   findings in `this paper <http://onlinelibrary.wiley.com/doi/10.1002/nbm.3017/abstract>`__.

-  Check the condition of the transformation between DWIs and spherical
   harmonics. If the transformation is ill-conditioned (usually indicating
   that the diffusion sensitisation gradient directions are not evenly
   distributed over the sphere or half-sphere), reduce ``lmax`` until the
   transformation is well-conditioned.

   As an example: concatenating two repeats of a 30 direction acquisition
   to produce 60 volumes will *not* support an ``lmax``=8 fit: the angular
   resolution of the data set is equivalent to 30 *unique* directions, and
   so ``lmax``=6 would be selected (and this would be accompanied by a
   command-line warning to the user).

-  In the case of spherical deconvolution, the ``lmax`` selected for FOD
   estimation will also be reduced if ``lmax`` of the provided response
   function is less than that calculated as above.

Reduced ``lmax`` in particular subjects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you find that certain subjects within a cohort have a reduced ``lmax``
compared to the rest of the cohort (usually spotted by checking the number
of coefficients in the response function), the most likely cause is
premature termination of the diffusion sequence during scanning of that
subject, resulting in a reduced number of diffusion volumes and therefore
a reduced ``lmax`` according to the table above.

Setting ``lmax`` in different applications
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The range of permissible values for ``lmax`` depends on the particular
command being used; e.g.:

-  The way that response function estimation is currently implemented, it
   is impossible to set ``lmax`` to a value higher than that supported by the
   image data. The transformation from DWI data to spherical harmonics simply
   cannot be done in such a case, as the problem is under-determined. You can
   of course set ``lmax`` to a lower value than that supported by the data.

-  In spherical deconvolution, it *is* possible to set a higher ``lmax``
   than that supported by the data - so-called *super-resolved* spherical
   deconvolution. Here, additional information is provided by the non-negativity
   constraint to make estimation of additional spherical harmonic coefficients
   possible. However this is not guaranteed: sometimes the algorithm will fail
   in particular voxels, in cases where there are an insufficient number of
   directions in which the initial FOD estimate is negative, as the problem
   remains under-determined.

-  If performing Track Orientation Density Imaging (TODI) using
   ``tckgen -tod``, then the apodized point spread functions (aPSFs) can be
   generated at any value of ``lmax``, since the angular resolution of the
   original image data is not a limiting factor here.
