.. _struct_connectome_construction:

Structural connectome construction
==================================

Included in this new version of MRtrix are some useful tools for
generating structural connectomes based on streamlines tractography.
Here I will describe the steps taken to produce a connectome, and some
issues that should be taken into consideration. Note that I will **not**
be going into appropriate parcellations or network measures or anything
like that; once you've generated your connectomes, you're on your own.

Preparing a parcellation image for connectome generation
--------------------------------------------------------

Parcellations are typically provided as an integer image, where each
integer corresponds to a particular node, and voxels where there is no
parcellation node have a value of 0. However, for all of the
parcellation schemes I've looked at thus far, the values used for the
nodes do not increase monotonically from 1, but rather have some
non-linear distribution; a text file (or 'lookup table') is then
provided that links node indices to structure names. This is however
undesirable for connectome construction; it would be preferable for the
node indices to increase monotonically from 1, so that each integer
value corresponds to a row/column position in the connectome matrix.

This functionality is provided in the command ``labelconvert``. It takes
as its input a parcellation image that has been provided by some other
software package, and converts the label indices; this is done so
that the code that actually generates the connectome can be 'dumb and
blind', i.e. the integer values at the streamline endpoints correspond
to the row & column of the connectome matrix that should be incremented.
In addition, this processing chain design provides flexibility in terms
of both the source of the parcellation data, and the way in which the
user wishes to customise the layout of their connectome.

Please consult the tutorial :ref:`labelconvert_tutorial` for a guide on
how to use the ``labelconvert`` command.

Generating the connectome
-------------------------

The command ``tck2connectome`` is responsible for converting the
tractogram into a connectome matrix, based on the provided parcellation
image. By default, the streamline count is used as the connectivity
metric; run ``tck2connectome -help`` to see alternative heuristics /
measures.

A factor in structural connectome production commonly overlooked or not
reported in the literature is the mechanism used to assign streamlines
to grey matter parcels. If done incorrectly this can have a large
influence on the resulting connectomes. This is one aspect where
:ref:`act` really shines; because streamlines can only terminate precisely at the grey matter -
white matter interface, within sub-cortical grey matter, or at the
inferior edge of the image, this assignment becomes relatively trivial.
The default assignment mechanism is a radial search outwards from the
streamline termination point, out to a maximum radius of 2mm; and the
streamline endpoint is only assigned to the first non-zero node index.
If you do not have the image data necessary to use the ACT framework,
see the 'No DWI distortion correction available' section below.

SIFT and the structural connectome
----------------------------------

If you are generating structural connectomes, you should be using
:ref:`sift`.

Extracting pathways of interest from a connectome
-------------------------------------------------

The command ``connectome2tck`` can be used to extract specific
connections of interest from a connectome for further interrogation or
visualisation. Note that since the resulting connectome matrix does not
encode precisely which parcellation node pair each streamline was
assigned to, the streamlines are re-assigned to parcellation nodes as
part of this command. Run ``connectome2tck -help`` to see the various
ways in which streamlines may be selected from the connectome.

Also: Beware of running this command on systems with distributed network
file storage. This particular command uses an un-buffered file output
when writing the streamlines files, which re-opens the output file and
writes data for individual streamlines at a time (necessary as many
files may be generated at once); such systems tend to be optimised for
large-throughput writes, so this command may cause performance issues.

No DWI distortion correction available
--------------------------------------

If you can't perform DWI susceptibility distortion correction, it
severely limits how accurately you can estimate the structural
connectome. If this is the case for you, below is a few points that are
worth considering.

Non-linear registration
~~~~~~~~~~~~~~~~~~~~~~~

Rather than actually correcting the DWI geometric distortions, some
people try to do a non-linear registration between DWI and T1 images. In
general I'm against this: the registration is fairly ill-posed due to
the differing contrasts, and an off-the-shelf non-linear registration
will have too many degrees of freedom. Pursue at your own risk.

Grey matter parcellation
~~~~~~~~~~~~~~~~~~~~~~~~

With good spatial alignment, parcellations that highlight only the
cortial ribbon (e.g. FreeSurfer) are highly accurate and effective, and
the assignment of streamlines to those parcellations will also be robust
if ACT is used. But without these, residual registration errors may have
a large influence, and assigning streamlines to parcellations only as
thick as the cortex may also be erroneous (streamlines may terminate
prior to the parcel, or travel through and extend well beyond it). A
parcellation with large-volume nodes that is based on atlas registration
(e.g. AAL) is likely more appropriate in this case.

Assignment of streamlines to parcellation nodes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Without ACT, streamlines will terminate pretty much anywhere within the
DWI brain mask. Not only this, but they may traverse multiple
parcellation nodes, turn around within a node and traverse elsewhere,
terminate just prior to entering a node, all sorts of weirdness. I have
provided a few assignment mechanisms that you can experiment with - run
``tck2connectome -help`` to see the list and parameters for each.
Alternatively if anyone has a better idea for how this could potentially
be done, I'd love to hear it.


