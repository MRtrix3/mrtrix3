.. _dix_fix:

Dixels and Fixels
=================

So internally we have created a couple of new terms that we find
invaluable when discussing diffusion MRI processing methods and
statistics. We'd like to share these with our user base in the hope that
others will gain advantages from using the same terminology, and also so
that we all know what everyone else is talking about! Anyone using
*MRtrix3* to develop their own software may also see these terms scattered
throughout the library code, so will need to know what they represent.

All MRtrix users should be familiar with the terms 'pixel' and 'voxel';
these correspond to 'picture element' and 'volume element' respectively.
However in Diffusion MRI we also deal with orientation information
within each image volume element, so we wanted terminology to allow us
to convey the types of discrete elements that we deal with on a daily
basis.

We have settled on the following terms; note that this may conflict with
presentations that we have done in the past, but this is now what we are
sticking to.

'Dixel': *Directional Element*
------------------------------

Imagine a single image voxel, the data for which is in fact a function
on the sphere (i.e. varies with orientation). We now take samples of
that function along a set of pre-defined directions on the unit sphere.
Each of those samples is referred to as a *dixel*: a directional element
within a specific voxel. Each dixel is described by the voxel in which
it resides, the direction along which the relevant spherical function
was sampled, and the intensity of the function in that direction.

Importantly, it is the *combination* of the voxel location and sampling
direction that describe the dixel. If a different direction were used to
sample the spherical function, that would be a different dixel with a
different value; likewise, if the spherical function in an adjacent
voxel were sampled along the same direction, that would also be a
different dixel with a different value. Each dixel is a unique sample of
a spatially-varying spherical function.

Most commonly, the term dixel is used to refer to the situation where a
set of directions on the unit sphere has been used to sample a Fibre
Orientation Distribution (FOD) that is otherwise continuous as expressed
in the Spherical Harmonic basis. However, by the definition of the term,
'dixel' could also be used to describe a single voxel within a
particular image volume in a HARDI experiment; if the HARDI signal in a
single voxel is considered to be discrete samples of the orientation
dependence of the diffusion signal in that voxel, then each of those
samples could be labelled a dixel.

Although we find this term useful in our internal discussions, and the
original Apparent Fibre Density (AFD) statistical method was based
around this concept, it is not a term that we expect to be adopted by
others, as its applicability for the end user is limited.

'Fixel': *Fibre bundle element*
-------------------------------

It will be more common to hear use of the term *fixel*; this refers to a
specific fibre bundle within a specific voxel. Each fixel is therefore
parametrized by the voxel in which it resides, the estimated mean
direction of the underlying fibres attributed to that bundle, a fibre
density (or partial volume fraction), and potentially other metrics.

At this point it is important to distinguish between 'dixel' and
'fixel'. A 'dixel' is typically assumed to represent a sample of a
spherical function along some pre-determined direction, where that
direction belongs to some basis set of equally-distributed unit
directions that has been used to sample an otherwise continuous
spherical function. 'Fixel', on the other hand, is used to describe a
set of fibres within a voxel that are sufficiently similar in
orientation that they are indistinguishable from one another, and
therefore form a fibre 'bundle' within that voxel.

In reality, fixels have been used in the field of Diffusion MRI for a
long time: multi-tensor fitting, ball-and-sticks, any diffusion model
that is capable of fitting multiple anisotropic elements to each image
voxel, can be considered as providing fixels. We've just resorted to
long-winded explanations to describe what we're on about. With MRtrix we
are historically more accustomed to dealing with FODs that are
continuous functions on the sphere, and are utilised as such during
processing; however, if the FOD is *segmented* in any way (either
through peak-finding, the segmentation approach as described in the
appendices of the
`SIFT NeuroImage paper <www.sciencedirect.com/science/article/pii/S1053811912011615>`_,
or more advanced methods), each discrete feature of a particular FOD can
be labelled a fixel, as each represents a set of fibres within that voxel
that form a coherent bundle in orientation space.

The term 'fixel' has now appeared in the literature with the publication
of our new statistical method,
`Connectivity-based Fixel Enhancement <http://www.sciencedirect.com/science/article/pii/S1053811915004218>`_,
which allows for the inference of group differences not just at the voxel
level, but the *fixel* level; that is, if only one fibre bundle within a
crossing-fibre voxel is affected in a cohort, we hope to both identify the
bundle affected, and quantify the group effect that is specific to that bundle.

