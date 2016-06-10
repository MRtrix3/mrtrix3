.. _sift:

Spherical-deconvolution Informed Filtering of Tractograms (SIFT)
================================================================

SIFT, or 'Spherical-deconvolution Informed Filtering of Tractograms', is
a novel approach for improving the quantitative nature of whole-brain
streamlines reconstructions. By producing a reconstruction where the
streamlines densities are proportional to the fibre densities as
estimated by spherical deconvolution throughout the white matter, the
number of streamlines connecting two regions becomes a proportional
estimate of the cross-sectional area of the fibres connecting those two
regions. We therefore hope that this method will attract usage in a
range of streamlines tractography applications.

The actual usage of SIFT can be found in the help page of the
``tcksift`` command. In this page I'll outline some issues that are
worth thinking about if you are looking to apply this method.

References
----------

For full details on SIFT, please refer to the following journal article:

    `Smith, R. E., Tournier, J.-D., Calamante, F., & Connelly, A.
    (2013). SIFT: Spherical-deconvolution informed filtering of
    tractograms. NeuroImage, 67, 298–312.
    doi:10.1016/j.neuroimage.2012.11.049 <http://www.ncbi.nlm.nih.gov/pubmed/23238430>`__

If you use SIFT in your research, please cite the article above in your
manuscripts.

DWI bias field correction
-------------------------

DWI volumes often have a non-negligible *B1* bias field, mostly due to
high-density receiver coils. If left uncorrected, SIFT will incorrectly
interpret this as a spatially-varying fibre density. Therefore bias
field correction is highly recommended. We generally estimate the bias
field based on the mean *b=0* image, and apply the estimated field to
all DWI volumes. This can currently be achieved using the
``dwibiascorrect`` script, which can employ either the FAST tool in FSL
or the N4 algorithm in ANTS to perform the field estimate.

Number of streamlines pre / post SIFT
-------------------------------------

In diffusion MRI streamlines tractography, we generate discrete samples
from a continuous fibre orientation field. The more streamlines we
generate, the better our reconstruction of that field. Furthermore, the
greater number of streamlines we generate, the less influence the
discrete quantification of connectivity has on the connectome (e.g.
would rather be comparing 1,000 v.s. 2,000 streamlines to 1 v.s. 2; it's
less likely to be an artefact of random / discrete sampling). So the
more streamlines the better, at the cost of execution speed & hard drive
consumption.

However we also have the added confound of SIFT. The larger the number
of streamlines that can be fed to SIFT the better, as it can make better
choices regarding which streamlines to keep/remove; but it also
introduces a memory constraint. SIFT can deal with approximately 4-8
million streamlines per GB of RAM (depending on the seeding mechanism
used and the spatial resolution of your diffusion images), so ideally
you'll want access to dedicated high-performance computing hardware. On
top of this, there's the issue of how many streamlines to have remaining
in the reconstruction after SIFT; the more streamlines that SIFT
removes, the better the streamlines reconstruction will fit the image
data, but the more likely you are to run into quantisation issues with
the resulting tractogram.

So when you design your image processing pipeline, you need to consider
the compromise between these factors:

-  Initially generating a larger number of streamlines is beneficial for
   both the quality and the density of the filtered reconstruction, at
   the expense of longer computation time (both in generating the
   streamlines, and running SIFT), and a higher RAM requirement for
   running SIFT.
-  Filtering a greater number of streamlines will always produce a
   superior fit to the image data, at the expense of having a
   lower-density reconstruction to work with afterwards, and a slightly
   longer computation time.

Unfortunately there's no single answer of how many streamlines are
required, as it will depend on the diffusion model, tractography
algorithm, and spatial extent of your target regions / connectome
parcellation granularity. There are a couple of papers / abstracts on
the topic if you look hard enough, but nothing definitive, and nothing
involving SIFT. I would recommend testing using your own data to find
numbers that are both adequate in terms of test-retest variability, and
computationally reasonable.

Personally I have been using a FreeSurfer parcellation (84 nodes),
generating 100 million streamlines and filtering to 10 million using
SIFT (I'm a physicist; I like orders of magnitude). In retrospect, I
would say that when using white matter seeding, filtering by a factor of
10 is inadequate (i.e. the fit of the reconstruction to the data is not
good enough); and with grey matter - white matter interface seeding, a
final number of 10 million is inadequate (the streamlines are mostly
very short, so the appearance of the reconstruction is quite sparse).
Another alternative is 'dynamic seeding', which uses the SIFT model
during tractogram generation to only seed streamlines in pathways that
are poorly reconstructed (see the ``-seed_dynamic`` option in
``tckgen``); this provides a better initial estimate, so the percentage
of streamlines that need to be removed in order to achieve a good fit is
reduced. I will leave it to the end user to choose numbers that they
deem appropriate (unless we do a paper on the topic, in which case you
will use our published values without question).

Normalising connection density between subjects
-----------------------------------------------

An ongoing issue with our Apparent Fibre Density (AFD) work is how to
guarantee that a smaller FOD in a subject actually corresponds to a
reduced density of fibres. Structural connectome studies have a similar
issue with regards to streamline counts; Even if SIFT is applied, this
only guarantees correct proportionality between different connection
pathways within a subject, not necessarily between subjects. The
simplest and most common solution is simply to use an identical number
of streamlines for every subject in connectome construction; however
this isn't perfect:

-  The distribution of streamlines lengths may vary between subjects,
   such that the reconstructed streamlines 'density' differs.
-  A subject may have decreased fibre density throughout the brain, but
   be morphologically normal; if the same number of streamlines are
   generated, this difference won't be reflected in the tractogram
   post-SIFT.
-  If the white matter volume varies between subjects, but the actual
   number of fibres within a given volume is consistent, then the
   subject with a larger brain may have an elevated total number of
   fibre connections; this would also be missed if the number of
   streamlines were fixed between subjects.

It's also possible to scale by the total white matter volume of each
subject; this would however fail to take into account any differences in
the density of fibres within a fixed volume between subjects.

An alternative approach is to try to achieve normalisation of FOD
amplitudes across subjects, as is done using AFD. This requires a couple
of extra processing steps, namely inter-subject intensity normalisation
and use of a group average response function, which are also far from
error-free. But if this can be achieved, it means that a fixed density
of streamlines should be used to reconstruct a given FOD amplitude
between subjects, and then the cross-sectional area of fibres
represented by each streamline is also identical between subjects; this
can be achieved by terminating SIFT at a given value of the
proportionality coefficient using the ``-term_mu`` option. One potential
disadvantage of this approach (in addition to the issues associated with
intensity normalisation) is that using a group average response function
instead of the individual subject response may result in spurious peaks
or incorrect relative volume fractions in the FODs, which could
influence the tracking results.

Ideally, a diffusion model would provide the absolute partial volume of
each fibre population, rather than a proportional quantity: this could
then be used directly in SIFT. However the diffusion models that do
provide such information tend to get the crossing fibre geometry wrong
in the first place...

If anyone has any ideas on how to solve this pickle, let us know.

No DWI distortion correction available
--------------------------------------

SIFT should ideally be used in conjunction with ACT; by passing the ACT
5TT image to ``tcksift`` using the ``-act`` option, the command will
automatically derive a processing mask that will limit the contribution
of non-pure-white-matter voxels toward the model. Without this
information, non-pure-white-matter voxels adversely affect both
streamlines tractography, and the construction of the SIFT model.

If you are looking to apply SIFT without correction of DWI geometric
distortions (and therefore without reliable high-resolution
co-registered anatomical image data), these are some points that you may
wish to consider:

-  The spatial extent of the DWI mask may have a large influence on your
   streamlines tractography results. Therefore greater care should
   perhaps be taken to validate this mask, including manual editing if
   necessary.

-  It is possible to manually provide a processing mask to ``tcksift``
   using the ``-proc_mask`` option. If users are capable of
   heuristically generating an approximate white matter partial volume
   image from the DWI data alone, this may be appropriate information to
   provide to the SIFT model.

Use of SIFT for quantifying pathways of interest
------------------------------------------------

In some circumstances, researchers may be interested in the connection
density of one or two specific pathways of interest, rather than that of
the whole brain. SIFT is still applicable in this scenario; however the
SIFT algorithm itself is only applicable to whole-brain fibre-tracking
data. Therefore, the workflow in this scenario should be: \* Generate a
whole-brain tractogram; \* Apply SIFT; \* Extract the pathway(s) of
interest using ``tckedit``. \* Get the streamline count using
``tckinfo``.

The SIFT algorithm is *not directly applicable to targeted tracking
data*. The underlying biophysical model in SIFT assumes that the
estimated density of each fibre population in every voxel of the image
should be proportionally reconstructed by streamlines; if only a subset
of pathways in the brain are permitted to be reconstructed by the
tractography algorithm, this will clearly not be the case, so
appplication of SIFT in this instance will provide erroneous results.

