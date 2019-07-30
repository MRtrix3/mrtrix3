Motivation for ``afdconnectivity``
==================================


Due to the interest in the :ref:`afdconnectivity` command, I thought I'd
explain the reasoning behind the approach, the rationale behind the
improvements made in commit 40ccdb62, and the argument for why we
recommend the use of :ref:`sift` as an alternative if possible.

The ``afdconnectivity`` command was originally written as a 'hack' for a
colleague who wanted to obtain quantitative measures of 'connectivity'
in the absence of EPI distortion correction. Without EPI distortion
correction :ref:`act` cannot
be applied, and consequently streamlines may terminate within white
matter. Streamline count (as a measure of connectivity) between two grey
matter regions will therefore not include those streamlines that
terminate in white matter (and therefore the estimated connectivity may
not be accurate).

The afdconnectivity command attempts to get around this issue by
estimating a measure of 'connectivity' as follows:

-  The integral of a discrete lobe of an FOD
   (:ref:`fixel <fixels_dixels>`) is proportional to the volume of
   the MR-visible tissue (intra-cellular at high *b*-value) aligned in
   that direction.

-  By taking a set of streamlines corresponding to a pathway of
   interest, and summing the integrals of all FOD lobes traversed by the
   bundle, you obtain an estimate of the total fibre volume of the
   pathway of interest.

-  If you then divide by the length of the bundle (taken as the mean
   streamline length), you get an estimate of the cross-sectional area
   of the bundle, which is a measure of 'connectivity' independent of
   fibre length.

The major problem with this approach is the assumption that *all* of the
fibre volume in each fixel traversed by the streamlines of interest
belong to the bundle of interest; clearly not the case in various
circumstances. The changes I have made to ``afdconnectivity`` are aimed
at improving the behaviour in the presence of partial volume and
erroneous streamlines.

The default behaviour is as before: determine a fixel mask using some
bundle of streamlines, sum the apparent fibre density (a volume) of the
fixels within the mask, and divide by mean streamline length (to get an
estimate of cross-sectional area of the pathway).

Now, you can optionally provide a whole-brain fibre-tracking data set
using the ``-wbft`` option (your bundle .tck file should then be a
subset of this tractogram). In this case, the program determines the
total streamlines density attributed to each fixel, and for those fixels
traversed by the streamlines of interest, some fraction of the fibre
volume of that fixel is contributed to the result. This fraction is
determined for each fixel by the ratio of streamlines density from the
bundle of interest, to the total streamlines density from the
tractogram. The fibre volume of each fixel is therefore divided 'fairly'
between the bundle of interest and the rest of the tractogram.

Although this may be an improvement in many circumstances, it's still
not our recommended method. Effectively what's happening in this
scenario is that for each streamline, a fibre volume is determined,
based on its 'fair share' of each fixel it traverses. However this means
that the effective cross-sectional area of that streamline is *allowed
to vary drastically along its length*; this is clearly not physically
realistic. Furthermore, due to the relative over- or
under-reconstruction of different pathways in whole-brain
fibre-tracking, there's no guarantee that this proportional 'sharing' of
fibre volume between streamlines is biologically accurate.

Now consider the alternative: filtering a tractogram using
:ref:`sift`, then selecting a subset of the remaining streamlines
corresponding to your pathway of interest. By the model underlying SIFT,
each streamline represents a constant cross-sectional area of fibres; so
the *streamline count* becomes your estimate of bundle cross-sectional
area and therefore 'connectivity' (with the SIFT proportionality
coefficient providing the conversion between streamline count and AFD if
you so choose).

This argument also holds if you are looking to use the image output from
``afdconnectivity``, which provides the estimated fibre volume of the
pathway of interest within each voxel. I have already stated why this is
a poor interpretation with the default ``afdconnectivity`` behaviour;
it's improved with use of the ``-wbft`` option, but is noisy in regions
where fixels are traversed by very few streamlines, and still may not
share the fibre volume of each fixel appropriately. Again, SIFT provides
the better alternative: an equivalent map can be produced by selecting
your streamlines of interest post-SIFT, and running ``tckmap -precise``
(sums streamline lengths within each voxel rather than counting
streamlines). Remember: a product of cross-sectional area and length gives a volume!

This is also an important message for interpretation of AFD results,
both in this context and others. FOD amplitude (in any guise) is *in no
way* a measure of "tissue integrity", no matter how many quotation marks
you use; it's a measure of *density*. This is the reasoning behind the
modulation step in
`AFD <http://www.sciencedirect.com/science/article/pii/S1053811911012092>`__,
and is the entire premise behind the SIFT method.

Anyways, rant over. We are considering writing a technical note that
will discuss this issue, so we are trusting the *MRtrix3* beta user base
not to do anything scientifically unethical with this information /
command until we can create the relevant article for citation.

