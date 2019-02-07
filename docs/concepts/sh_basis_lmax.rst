Maximum spherical harmonic degree *l*:sub:`max`
-----------------------------------------------

What determines *l*:sub:`max` for my image data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For any command or script operating on data in the spherical harmonic
basis, it should be possible to manually set the maximum harmonic degree
of the output using the ``-lmax`` command-line option. If this is *not*
provided, then an appropriate value will be determined automatically.

The mechanisms by which this automatic determination of *l*:sub:`max` occurs
are as follows:

-  Determine the maximum value for *l*:sub:`max` that is supported by the number
   of DWI volumes in the shell being processed (or the total number of
   non-*b*\=0 volumes in a single-shell acquisition). This is the number of
   coefficients required to store an antipodally-symmetric spherical
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

-  If *l*:sub:`max` exceeds 8, reduce to 8. This is primarily based on the
   findings in `this paper <http://onlinelibrary.wiley.com/doi/10.1002/nbm.3017/abstract>`__.

-  Check the condition of the transformation between DWIs and spherical
   harmonics. If the transformation is ill-conditioned (usually indicating
   that the diffusion sensitisation gradient directions are not evenly
   distributed over the sphere or half-sphere), reduce *l*:sub:`max` until the
   transformation is well-conditioned.

   As an example: concatenating two repeats of a 30 direction acquisition
   to produce 60 volumes will *not* support an *l*:sub:`max`\=8 fit: the angular
   resolution of the data set is equivalent to 30 *unique* directions, and
   so *l*:sub:`max`\=6 would be selected (and this would be accompanied by a
   command-line warning to the user).

-  In the case of spherical deconvolution, the *l*:sub:`max` selected for FOD
   estimation will also be reduced if *l*:sub:`max` of the provided response
   function is less than that calculated as above.

The *exception* to these rules is the new ``amp2response`` command, which
is now called by default in all ``dwi2response`` script algorithms. This
command converts *amplitudes* on the half-sphere (most likely in the form
of raw DWI image intensities) into a *response function* intended for use
in spherical deconvolution. This command behaves differently for two
reasons in combination:

-  The image data from multiple voxels are combined together in a single
   fitting procedure, therefore having a much greater number of samples
   when performing the transformation.

-  The data are transformed not to the spherical harmonic basis, but 
   directly to the *zonal* spherical harmonic basis (this is the spherical
   harmonic basis containing only the ``m = 0`` terms). This basis requires
   far fewer coefficients for any given value of *l*:sub:`max`: 2 for
   *l*:sub:`max`\=2, 3 for *l*:sub:`max`\=4, 4 for *l*:sub:`max`\=6, 5 for
   *l*:sub:`max`\=8 and so on.

The value of *l*:sub:`max` that can be used in this command is therefore
practically unconstrained; though the power in higher harmonic degrees
is much smaller than that in lower degrees. The command is currently
configured to select *l*:sub:`max`\=10 by default, regardless of *b*-value;
interested readers can find the discussion `here <https://github.com/MRtrix3/mrtrix3/pull/786>`__.

Reduced *l*:sub:`max` in particular subjects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you find that certain subjects within a cohort have a reduced *l*:sub:`max`
compared to the rest of the cohort when using any command relating to
spherical harmonics, the most likely cause is premature termination of the
diffusion sequence during scanning of those subjects, resulting in a reduced
number of diffusion volumes, and therefore a reduced *l*:sub:`max` according to
the table above.

Setting *l*:sub:`max` in different applications
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The range of permissible values for *l*:sub:`max` depends on the particular
command being used; e.g.:

-  For any command that maps image data directly to spherical harmonics, it
   is impossible to set *l*:sub:`max` to a value higher than that supported by the
   image data. The transformation from DWI data to spherical harmonics simply
   cannot be done in such a case, as the problem is under-determined. You can
   of course set *l*:sub:`max` to a lower value than that supported by the data.

-  In spherical deconvolution, it *is* possible to set a higher *l*:sub:`max`
   than that supported by the data - so-called *super-resolved* spherical
   deconvolution. Here, additional information is provided by the non-negativity
   constraint to make estimation of additional spherical harmonic coefficients
   possible.

-  If performing Track Orientation Density Imaging (TODI) using
   ``tckmap -tod``, then the apodized point spread functions (aPSFs) can be
   generated at any value of *l*:sub:`max` for which aPSF data are available
   (currently *l*:sub:`max`\=16, since the angular resolution of the original image
   data is not a limiting factor here.

-  As described previously, the ``amp2response`` command is a special case,
   and the maximum permissible *l*:sub:`max` is vastly greater than the maximum
   practical value.
