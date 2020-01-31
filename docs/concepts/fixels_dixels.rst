.. _fixels_dixels:

"Fixels" (and "Dixels")
=======================

Internally we have created a couple of new terms that we find
invaluable when discussing diffusion MRI processing methods and
statistics. We'd like to share these with our user base in the hope that
others will gain advantages from using the same terminology, and also so
that we all know what everyone else is talking about! Anyone using
*MRtrix3* to develop their own software may also see these terms scattered
throughout the library code, so will need to know what they represent.

All *MRtrix3* users should be familiar with the terms 'pixel' and 'voxel';
these are abbreviations of "picture element" and "volume element",
corresponding to the smallest element within a 2D picture and 3D volume
respectively. However in Diffusion MRI we also deal with *orientation*
information within each image volume element; so we wanted terminology
to allow us to convey the types of discrete elements that we deal with
on a daily basis.

We have settled on the following two terms. The first of these, 'fixel',
will appear frequently throughout the *MRtrix3* documentation and in
online discussions, and will therefore satisfy the requirements of the
majority of users. The second, 'dixel', is typically reserved for internal
technical discussion; however due to its occasional usage (and its inconsistent
use in early presentations, see final note below), we are additionally
providing its full definition here for interested readers.

'Fixel': *Fibre bundle element*
-------------------------------

The term *fixel* refers to a *specific fibre bundle* within a *specific
voxel*. Alternatively, consistently with the definitions of 'pixel' and
'voxel', it can be thought of as a "fibre bundle element": the smallest
discrete component of a fibre bundle. Each fixel is parameterized by
the voxel in which it resides, the estimated mean orientation of the
underlying fibres attributed to that bundle, a fibre density (or partial
volume fraction), and potentially other metrics.

In reality, fixels have been used in the field of Diffusion MRI for a
long time: multi-tensor fitting, ball-and-sticks, any diffusion model
that is capable of fitting multiple anisotropic elements to each image
voxel, can be considered as estimating fixels. However in the past,
researchers have resorted either to lengthy descriptive labels in an
attempt to express the nature of the data being manipulated, or have
adopted existing terms, which can lead to confusion with the original sense of
the terms. Furthermore, these labels are not applied inconsistently
between publications; we hope that the term 'fixel', being unambiguous with
other interpretations of "fibre bundle" or "fascicle" or other examples,
will slowly become the standard term for describing these data.

Historically, in MRtrix we are accustomed to dealing with FODs that are
*continuous* functions on the sphere, rather than having a discrete number
of fibre directions in each voxel. However, if the FOD is *segmented* in
any way (either through peak-finding as shown in `this paper <http://onlinelibrary.wiley.com/doi/10.1002/hbm.22099/abstract>`_
and implemented in the ``sh2peaks`` command, the segmentation algorithm
described in the appendices of the `SIFT NeuroImage paper <http://www.sciencedirect.com/science/article/pii/S1053811912011615>`_
and provided in the ``fod2fixel`` command, or more advanced methods), each
discrete feature of a particular FOD can be labelled a 'fixel', as each
represents a set of fibres within that voxel that form a coherent bundle
in orientation space.

The term 'fixel' has now appeared in the literature with the publication
of the statistical method,
`Connectivity-based Fixel Enhancement <http://www.sciencedirect.com/science/article/pii/S1053811915004218>`_,
as well as the more general framework of `Fixel-Based Analysis <http://www.sciencedirect.com/science/article/pii/S1053811916304943>`_,
which together allow for the inference of group differences not just at
the voxel level, but the *fixel* level; that is, if only one fibre bundle
within a crossing-fibre voxel is affected in a cohort, we hope to both
identify the bundle affected, and quantify the group effect that is specific
to that bundle.

'Dixel': *Directional Element*
------------------------------

This term is used less frequently, and hence may not be relevant for all
readers. If you have not seen it used before, you may in fact prefer to
avoid the following text in order to keep things simple...

Imagine a single image voxel, the data for which is in fact a function
on the sphere (i.e. varies with orientation). We now take samples of
that function along a set of pre-defined directions on the unit sphere.
Each of those samples is referred to as a '*dixel*': a *directional element*
within a *specific voxel*. Each dixel is described by the voxel in which
it resides, the direction along which the relevant spherical function
was sampled, and the intensity of the function in that direction.

Importantly, it is the *combination* of the voxel location and sampling
direction that describe the dixel. If a different direction were used to
sample the spherical function, this would be a different dixel with a
different associated value; likewise, if the spherical function in an
adjacent voxel were sampled along the same direction, that would also be a
different dixel with a different associated value. Each dixel is a unique
sample of a particular spatially-varying spherical function.

Most commonly, the term 'dixel' is used to refer to the situation where a
*set of directions* on the unit sphere has been used to sample some function;
for instance, sampling the amplitudes of a Fibre Orientation Distribution
(FOD), which is otherwise a continuous function expressed in the Spherical
Harmonic (SH) basis. However, by the definition of the term,
'dixel' could also be used to describe a single voxel within a
particular image volume in a HARDI experiment; if the HARDI signal in a
single voxel is considered to be discrete samples of the orientation
dependence of the diffusion signal in that voxel, then each of those
samples could be labelled a 'dixel'.

Therefore, the fundamental disambiguation between 'fixels' and 'dixels' is
as follows:

-  A 'dixel' is typically assumed to represent a sample of a spherical
   function along some pre-determined direction, where that direction
   belongs to some dense *basis set* of equally-distributed unit directions
   that has been used to sample an otherwise continuous (hemi-)spherical
   function.

-  'Fixel', on the other hand, is used to describe a *set of fibres* within
   a voxel that are sufficiently similar in orientation that they are
   indistinguishable from one another, and therefore form a fibre 'bundle'
   within that voxel.

Some observations / contexts in which the term 'dixel' may be useful:

-  The ``mrview`` "ODF overlay" tool is capable of loading "Dixel ODFs".
   These can be either a set of direction-based samples on the sphere, or
   it can be used to directly visualise the diffusion signal within a
   particular *b*-value shell, since both of these cases correspond to a
   set of directions on the unit hemisphere, where each direction has
   associated with it an 'intensity' / 'amplitude'.

-  In the original Apparent Fibre Density (AFD) `manuscript <http://www.sciencedirect.com/science/article/pii/S1053811911012092>`_,
   the statistical analysis was performed by performing a t-test in each
   of 200 directions in each voxel, and then detecting connected clusters
   in position & orientation space. This can be thought of as "dixel-based
   cluster statistics".

-  In the FOD segmentation method provided in the ``fod2fixel`` command
   mentioned earlier, the algorithm first samples the amplitude of the
   FOD along a set of 1,281 directions, before identifying fixels based
   on accumulating these directions / samples. So this process can be
   thought of as converting the FOD from a continuous SH representation,
   to a dixel representation, then finally to a fixel representation.

.. NOTE::

   During the development of many of the aforementioned methods,
   `a presentation <http://dev.ismrm.org/2013/0841.html>`_ was made at
   ISMRM demonstrating "Tractographic threshold-free cluster enhancement"
   (this is now referred to as "Connectivity-based Fixel Enhancement (CFE)").
   During the presentation itself, the term 'dixel' was used to refer to a
   specific direction within a specific voxel; but a direction that
   corresponds to a particular fible bundle in that voxel. You may observe
   that this definition is in fact consistent with what we have labelled
   here as a 'fixel', rather than a 'dixel'; this is because at the time
   when this presentation was made, these two terms had not yet been
   disambiguated. The definitions made within this documentation page are
   what will be used from now on by the *MRtrix3* developers; and we hope
   by the wider community as well.
