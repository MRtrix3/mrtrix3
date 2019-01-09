.. _global-intensity-normalisation:


Global intensity normalisation
==============================

Several existing DWI models derive quantitative measures by fitting a model to the
ratio of the DW signal to the b=0 signal within each voxel. Such a voxel-wise division
by the original b=0 signal removes intensity variations due to T2-weighting and RF
inhomogeneity. However, unless all compartments within white matter (e.g. intra- and
extra-axonal space, myelin, cerebral spinal fluid (CSF) and grey matter partial
volumes) are modelled accurately (i.e. with appropriate assumptions/modelling of both
the compartment diffusion and T2), the proportion of one compartment in a voxel may
influence another. For *example*, if CSF partial volume (e.g. at the border of white
matter and the ventricles) is not taken into account, then a voxel-wise division by the
b=0 (which has a long T2 and appears much brighter in CSF than in white matter in the
T2-weighted b=0 image), will artificially overreduce the DW signal from the white
matter intra-axonal (restricted) compartment, ultimately changing several derived
quantitative measures.

A previous work investigating differences in Apparent Fibre Density (AFD) [Raffelt2012]_
opted to instead perform a *global* intensity normalisation between subjects. This
avoids the aforementioned issues, but also comes with its own set of challenges and
assumptions inherent to specific strategies to deal with intensity normalisation for
diffusion MRI data. Aside from the problem of how to define a reference region for
global intensity normalisation (that is unbiased with respect to the groups in the
analysis), the data must also be *bias field* corrected, to eliminate low frequency
(spatially smooth) intensity inhomogeneities across the image.

In theory, an approach to global intensity normalisation could for example be to
normalise using the median CSF b=0 intensity for each subject as a reference (under the
assumption that the CSF T2 is unlikely to be affected by pathology). However, in
practice it is surprisingly difficult to obtain a robust partial-volume-free estimate of
the CSF intensity due to the typical low resolution of DW images. For healthy
participants less than 50 years old, reasonably small ventricles make it quite difficult
to identify *pure* CSF voxels at 2-2.5mm resolutions. Therefore, performing global
intensity normalisation using the median *white matter* b=0 intensity may be easier to
achieve. While the white matter b=0 intensity may be influenced by pathology-induced
changes in T2, the assumption then becomes that such changes would be (spatially) quite
local and therefore have little influence on the median white matter b=0 value.

The :ref:`dwiintensitynorm` script is included in MRtrix to perform an automatic global
normalisation using the *median white matter b=0 intensity*. The script input requires
two folders: a folder containing all DW images in the study (in .mif format) and a
folder containing the corresponding whole brain mask images (with the same filename
prefix). The script runs by first computing diffusion tensor Fractional Anisotropy (FA)
maps, registering these to a groupwise template, then thresholding the template FA map
to obtain an *approximate* white matter mask. The mask is then transformed back into the
space of each subject image and used in the :ref:`dwinormalise` command to normalise the
input DW images to have the same b=0 white matter median value. All intensity normalised
data will be output in a single folder. As previously mentioned, all DWI data must be
bias field corrected *before* applying :ref:`dwiintensitynorm`, for example using
:code:`dwibiascorrect`.  Users are well advised to (manually) check the results
of :ref:`dwiintensitynorm` closely though, as occasional instabilities have been
observed in the outcomes of particular subjects.

In case of pipelines that include a multi-tissue spherical deconvolution algorithm
yielding compartment estimates for multiple different tissues [Jeurissen2014]_
[Dhollander2016a]_, a *new* command called :ref:`mtnormalise` can be used instead, which
performs multi-tissue informed intensity normalisation in the log-domain, correcting
simultaneously for both global intensity differences as well as bias fields. The benefit
of the :ref:`mtnormalise` command is that normalisation can be performed *independently*
on each subject, and therefore does *not* require a computationally expensive (and
potentially not entirely accurate) registration step to a group template.

