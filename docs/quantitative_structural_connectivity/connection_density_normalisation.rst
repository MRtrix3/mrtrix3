.. _connection_density_normalisation:

Inter-subject connection density normalisation
==============================================

The SIFT [Smith2013]_ and SIFT2 [Smith2015]_ methods were originally advertised
as addressing the issue where the number of streamlines reconstructed
for different bundles within a brain were not concordant with the underlying
densities of white matter fibres constituting those connections.
What was not initially described in those manuscripts was the way in which
these methods could be used to perform meaningful comparison
of the densities of a given white matter bundle *between* individuals.

More recently, a manuscript [Smith2022]_ has been published that:

-   *Defines* the "Fibre Bundle Capacity (FBC)" metric for performing such comparisons;
-   Explains on a conceptual level *why* this is a suitable metric for inter-subject comparisons;
-   Explains the quantitative corrections necessary to achieve this
    "*inter-subject connection density normalisation*";
-   Shows the *relationship* between the SIFT and SIFT2 methods
    and the older ``afdconnectivity`` command
    (for which an older documentation page can be found here<motivation_for_afdconnectivity>_).

The purpose of this documentation page is to give a more pragmatic explanation
of how to obtain the desired quantitative properties
when performing quantitative structural connectivity.

Global intensity normalisation for Apparent Fibre Density (AFD)
---------------------------------------------------------------

The SIFT(2) commands currently accept as input a white matter FOD image,
and computes fixel-wise AFD values from that image.
As such, any subsequent calculations relating to overall scaling of connection density
are intrinsically dependent on the scaling of AFD.
The topic of global intensity normalisation is explained in greater detail here<global-intensity-normalisation>_.

For the vast majority of software users,
the simplest way to deal with these confounds
is to use the same approach as that of the Fixel-Based Analysis (FBA) pipeline
for the analysis of AFD:

-   Use the same response function(s) for every participant.

-   Perform global intensity normalisation of DWI data.
    This is most commonly achieved through the ``mtnormalise`` command,
    which applies the requisite scaling to the previously estimated Orientation Distribution Function(s) (ODFs)
    rather than the DWI data directly.
    Command ``dwibiasnormmask`` achieves a similar goal
    (indeed its internal operation is driven primarily by ``mtnormalise``),
    but differs in that:
    -   It applies intensity normalisation to the DWI data directly
        rather than just the resulting ODFs;
    -   The "reference" that is used to modulate the global image intensity
        is not that which achieves close to unity sum of tissue ODFs,
        but rather the b=0 signal intensity in CSF.
    The ``dwibiasnormmask`` command is discussed in more detail here<dwibiasnormmask>_.

# TODO Move dwibiasnormmask out of the DWI masking page and into its own page

SIFT and the proportionality coefficient $mu$
---------------------------------------------

Within the SIFT model,
parameter $/mu$ provides a global scaling factor
that facilitates direct comparison between streamlines density and AFD.
As explained in the more recent manuscript [Smith2022]_,
this parameter has units of AFD/mm:
this is the fibre volume contributed by the streamline per unit length.
Further, the physical dimensions of this parameter are $L<sup>2</sup>$: a cross-section.

The following suggestions apply to the ``tcksift`` command:

-   If DWI data for all participants possess the same spatial resolution,
    then multiplying streamline counts by the value that parameter $/mu$ takes
    *at the completion of filtering*
    is recommended.
    This will appropriately correct for inter-subject global differences in tractogram density
    (which can occur even if the *number* of streamlines is equivalent)
    and/or typical fibre densities within the WM.

-   If DWI data have different spatial resolutions between different participants / reconstructions,
    then it is recommended to multiply streamline counts by *both* parameter $/mu$
    *and* the volume of the DWI voxels in $mm<sup>3</sup>$.

SIFT2 and the units of streamline weights
-----------------------------------------

Because the ``tcksift2`` command exports a set of streamline weights
rather than a modified tractogram,
it is possible for the software to include or exclude such scaling factors in that output.
The interface for doing so involves setting the *units* of the output weights.
What follows is an explanation of the relationship between the choice of these units,
and what quantitative properties the resulting data will have.

SIFT2 operation
^^^^^^^^^^^^^^^

Unlike ``tcksift``, in ``tcksift2`` the value of $/mu$ *does not change*.

One consequence of determining the initial scaling via this parameter
is that when every streamline is initialised with a naive weight of 1.0,
the sum of squared differences between tractogram and fibre densities
should be close to the minimum achievable in the absence of per-streamline weighting.
As SIFT2 performs its optimisation,
different streamlines will take weights that are greater or lesser than 1.0,
but as a whole they will remain distributed approximately around 1.0.
For the purpose of this article I will refer to this as the *native scaling* of the streamline weights:
this is the form in which the optimisation of the streamline weights is performed;
any subsequent scaling of these weights is performed *only* at the export stage.

SIFT2 units & scaling factors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The relevant software interface for modifying the normalisation behaviour of SIFT2
involves setting the *units* of the output streamline weights.
The relationship between these choices of units,
and what scaling factors are applied to the data,
are as follows:

-   "*Number of Streamlines (NoS)*":
    The streamline weights are written to file with their native scaling.
    Since these weights are naturally distributed approximately around unity,
    subsequent quantitative data derived from these weights will have approximately
    the same overall scaling as would a naive number-of-streamlines measure
    using the same tractogram.

-   "*AFD/mm*":
    The streamline weights written to file are pre-multiplied by the value of the
    proportionality coefficient $/mu$.
    This therefore duplicates the internal scaling that is used by the SIFT model
    when comparing, for any given fixel,
    the tractogram density to the AFD ascribed to that fixel.

-   "*mm<sup>2</sup>*":
    The streamline weights written to file are multiplied *both* by the value of $/mu$
    *and* by the volume of the image voxel in $mm<sup>3</sup>$.
    While this quantification is not used internally by either SIFT command,
    it is the preferred units for quantification of FBC
    given that it is independent of the resolution of the FOD grid
    and therefore provides robust scaling even if different subjects
    have DWI data of different spatial resolutions.

    Note that while this choice purports to yield physical units,
    and therefore one may be tempted to infer that there may have ben physical constraints
    imposed upon the tractogram,
    this interpretation is made with some liberty.
    To achieve the export of data in these units,
    the AFD of each fixel is interpreted as a voxel partial volume,
    such that the product of the AFD and the voxel size yields an absolute fibre volume in $mm<sup>2</sup>$.
    This is somewhat contrary to the fact that the AFD measure is not constrained to be no greater than 1.0.
    It is therefore entirely possible for the tractogram density within an image voxel
    (computed as, for every streamline, the product of the streamline weight and the length of the streamline-voxel intersection,
    summed across all intersecting streamlines)
    to be greater than the physical volume of that voxel.

SIFT & SIFT2 units / scaling demonstration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this section,
I present data intended to demonstrate exactly what the data look like
under different use cases,
and contrast the relative global scaling of different quantitative parameters
between those use cases.

This first set of images are data that apply to all reconstruction / normalisation approaches.

1.   WM *l*=0 image

    .. figure:: normalisation/wmfod_l0.png
       :alt: normalisation\_wmfod_lzero

    Note that the intensity of this image in the middle of the white matter
    is an unintuitive value of approximately 0.28;
    this corresponds to $1//sqrt(4*pi)$ (0.282095),
    and arises due to the spherical harmonic basis of the FOD.

2.  WM *l*=0 image x sqrt(4pi)

    .. figure:: normalisation/wmfod_scaled.png
       :alt: normalisation\_wmfod_scaled

    This image was generated using the following command,
    where "``wmfod_norm.mif``" is the image displayed in point 1. above::

        mrcalc 4.0 pi -mult -sqrt wmfod_norm.mif -mult wmfod_norm_scaled.mif

    By explicitly multiplying the white matter ODF *l*=0 term by sqrt(4pi),
    one obtains an image where the intensity in voxels containing only WM
    should be approximately 1.0.
    While this is technically not a signal or volume *fraction*,
    given that it is not explicitly constrained to not exceed unity,
    these data are nevertheless similar in both interpretation and scale
    to the concept of fibre volume fractions.

3.  Voxel-wise sum of fixel AFDs

    .. figure:: normalisation/afd_sum.png
       :alt: normalisation\_afd_sum

    This image shows, for each voxel,
    the *sum* of Apparent Fibre Densities (AFD)
    for all fixels within that voxel.
    This was computed using the following::

        fod2fixel wmfod_norm.mif fixels/ -afd afd.mif
        fixel2voxel fixels/afd.mif sum fixels/afdsum.mif

    Important features of this image are as follows:

    -   The overall intensity scaling of this image
        is close to that of the image shown in point 2. above.
        This is due to calibration of the algorithm that performs segmentation of the FODs
        (as described in Appendix 2 of [Smith2013]_).
        For an image voxel where the DWI signal is equivalent to the WM response function,
        segmentation of the resulting FOD should yield a single FOD lobe,
        for which numerical integration of that lobe yields an AFD for that fixel of 1.0.

    -   The brightness of this image is however almost exclusively less than that
        of the image associated with point 2. above.
        This is because the segmentation of FODs into fixels
        can only *remove* AFD as a consequence of FOD lobes failing to satisfy
        the thresholds imposed in order to survive as fixels.
        Indeed, the only way for this image to be brighter
        is if the FOD contains directions with negative amplitudes,
        for which the corresponding negative-AFD lobes will be excluded from the fixel output.

    -   This image is slightly noisier than that in point 2. above.
        While the DWI signal intensity that is attributed to the WM FOD
        is relatively consistent and smooth between adjacent voxels,
        it is possible for the thresholds associated with FOD segmentation
        to yield different results between adjacent voxels
        (eg. both possess an FOD lobe in a similar direction,
        but that lobe is supra-threshold in one voxel but sub-threshold in the other).
        As such, it is generally suggested to use the WM *l*=0 term
        to encode the estimated total Apparent Fibre Density [Calamante2015]_.

Case 1: Unmodified tractogram
"""""""""""""""""""""""""""""

Here, the "unmodified tractogram" is defined as a tractogram where,
while the SIFT model has been *imposed*,
no tractogram modification in order to *optimise* that model has been performed.

1.  Tractogram weights:

    ``[1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, ...]``

    ie. All streamlines contribute equally to one another;
    an uninformative weight of unity is inferred,
    as is used for initialisation for the SIFT2 method.

2.  Proportionality coefficient (AFD/mm):
    # TODO

3.  Tractogram density maps:
    # TODO
    # Consider doing:
    # - Without -precise
    # - With -precise
    # - With -precise & multiplying by mu
    # - Comparison against AFD sum
    Note the display scaling in this image;
    because of the number of streamlines reconstructed,
    WM voxels contain values that are orders of magnitude greater
    than those images previously presented.

4.  Connectomes:
    # TODO
    # - Without modulation; ie. literally streamline count
    # - With modulation by mu

Case 2: Tractogram filtered using SIFT
""""""""""""""""""""""""""""""""""""""

1.  Tractogram weights:

    ``[0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0, ...]``

    The primary interface of the ``tcksift`` command
    involves export of a new tractogram file
    that contains only the set of streamlines retained.
    This can however be equivalently expressed as a vector of weights,
    one value per streamline,
    with values of 1 for retained streamlines and 0 for removed streamlines
    (this information can be accessed using ``tcksift -out_selection``).
    This conceptualisation better harmonises the operation of the original SIFT method [Smith2013]_
    with subsequent related methods
    as well as the overarching logic presented in this document.

2.  Proportionality coefficient (AFD/mm):
    # TODO
    Note that the value here is greater in magnitude than that of case 1.
    This is due to the design of the SIFT algorithm:
    as streamlines are removed from the tractogram,
    the proportionality coefficient is increased in proportion to that removal
    in order for the global scaling between tractogram and fibre densities to remain valid.
    This also preserves the capability for interpreting the outcomes of the SIFT method
    as a vector of ones and zeroes corresponding to preserved and removed streamlines
    as per point 1 above.

3.  Tractogram density maps:
    # TODO
    # - Without normalisation; is. NoS
    # - With normalisation

    -   For these density maps,
        there are two different ways in which this can be achieved.
        The logical approach is to provide as input to the ``tckmap`` command
        the filtered tractogram that is created by the ``tcksift`` command.
        The equivalent effect can however be obtained by providing
        the comprehensive tractogram to the ``tckmap`` command,
        but then providing the output of the ``tcksift -out_selection``
        as input via the ``-tck_weights_in`` option.
        This causes retained streamlines to contribute to the density map with a value of 1,
        and removed streamlines to not contribute to the density map
        (given their ascribed weight of 0).

    -   Note that for the unmodulated tractogram,
        the overall intensity of the image is around an order of magnitude less
        than the unmodified (& unmodulated) tractogram shown in Case 1 above;
        this is the result of the removal of streamlines by the SIFT method.
        The intensity of this image is however nevertheless reflective of
        the "number of streamlines" as a density metric.

    -   The modulated tractogram shows a density map with an overall intensity
        that is comparable to the sum of AFD per voxel from the diffusion model.
        This highlights the importance of use of the value of the proportionality coefficient
        *at completion of filtering*
        to achieve the appropriate intensity normalisation.

4.  Connectomes:
    # TODO
    # - Without modulation; ie. literally streamline count
    # - With modulation

Case 3: SIFT2 with units "Number of Streamlines (NoS)"
""""""""""""""""""""""""""""""""""""""""""""""""""""""

1.  Tractogram weights:

    # TODO Weights in "native" units

    As described earlier,
    these exported streamline weights possess the "native" scaling
    upon which the SIFT2 optimisation is performed.
    While these data do not literally represent a "number" of streamlines,
    given that each individual streamline contributes some real numerical value,





# TODO:
For each of [Before SIFT, After SIFT, No SIFT2, SIFT2 w. NoS units, SIFT2 w. AFD/mm units, SIFT2 w. mm^2 units]:
    Show head of streamline weights file (or list of 1's in the No SIFT2 case / 0's & 1's in the SIFT case?)
    Show TDI; compare to appropriately scaled WM ODF l=0, fixel AFDs?
    Show connectome matrix; highlight scale

Could generate lobar 8x8 connectomes? & lower spatial resolution image data?

The following series of images exemplify the quantitative behaviour
of measures commonly derived from tractograms following either SIFT method.
Particular attention should be paid to the relative scales of colour bars,
as these assist in communicating any global differences in intensities.

WM l=0
WM l=0 * sqrt(4pi)
Voxel-wise sum of fixel AFDs

              Tractogram modulation     Proportionality coefficient (AFD/mm)  Tractogram density map         Explicitly modulated tractogram density map       Connectome             Explicitly modulated connectome
Before SIFT:  column of 1's,            init_mu,                              tractogram density,            tractogram density * init_mu,                     connectome             connectome * mu
After SIFT:   column of 1's and 0's,    final_mu,                             tractogram density,            tractogram density * final_mu,                    connectome             connectome * mu
SIFT2 NoS:    column of native weights, mu,                                   weighted tractogram density,   weighted tractogram density * mu,                 weighted connectome    weighted connectome * mu
SIFT2 AFD/mm: column of scaled weights, mu,                                   weighted tractogram density,   N/A,                                              scaled connectome      N/A
SIFT2 mm^2:   column of scaled weights, mu,                                   weighted tractogram density,   N/A,                                              scaled connectome      N/A

Notes:

-   As noted in the relevant manuscript [Smith2022]_,
    the removal of streamlines by the SIFT method is mathematically equivalent
    to setting the per-streamline weights in the SIFT2 model to 1.0 for retained streamlines
    and 0.0 for removed streamlines.
    While this may not be reflected in the interface of the ``tcksift`` command,
    whose output is a tractogram file containing only the retained streamlines,
    this particular interpretation is nevertheless beneficial in this context.

-   For SIFT2 units mm^2,
    the difference in scaling

