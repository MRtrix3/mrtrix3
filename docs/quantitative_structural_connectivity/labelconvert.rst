.. _labelconvert_tutorial:

``labelconvert``: Explanation & demonstration
=============================================

The ``labelconvert`` (previously ``labelconfig``) step in
:ref:`struct_connectome_construction` has proven to be a hurdle for
many. It may be a 'unique' step in so far as that other software
packages probably deal with this step implicitly, but in MRtrix we
prefer things to be explicit and modular. So here I'll go through an
example to demonstrate exactly what this command does.

Worked example
--------------

For this example, let's imagine that we're going to generate a
structural connectome for Bert, the quintessential FreeSurfer subject.
Also, we're going to generate the connectome based on the
Desikan-Killiany atlas. The default FreeSurfer pipeline provides the
volumetric image aparc+aseg.mgz; this is the file that will be used to
define the nodes of our connectome.

.. figure:: https://cloud.githubusercontent.com/assets/5637955/3505536/d67b65ba-0660-11e4-80a2-3906a9f047be.png
   :alt: labelconvert\_before

Looking at the raw image itself, each node possesses a particular
intensity, corresponding to a particular integer value. If we focus on
the superior frontal gyrus in the right hemisphere, we can see that the
image intensity is 2028 for this structure.

This immediately presents a problem for constructing a connectome: if
any streamline encountering this region were written to row/column 2028,
our connectome would be enormous, and consist mostly of zeroes (as most
indices between 1 and 2028 do not correspond to any structure). Therefore,
what we'd prefer is to map the unique integer index of this structure to
a particular row/column index of the connectome; this should be done in
such a way that all structures of interest have a unique integer value
between 1 and *N*, where *N* is the number of nodes in the connectome.

Now looking at the file ``FreeSurferColorLUT.txt`` provided with FreeSurfer,
we see the following:

::

    ...
    2026    ctx-rh-rostralanteriorcingulate     80  20  140 0
    2027    ctx-rh-rostralmiddlefrontal         75  50  125 0
    2028    ctx-rh-superiorfrontal              20  220 160 0
    2029    ctx-rh-superiorparietal             20  180 140 0
    2030    ctx-rh-superiortemporal             140 220 220 0
    ...

This gives us a *meaningful name* for this structure based on the
integer index. It also gives us some colour information, but let's not
worry about that for now.

Our goal then is to determine a *new integer index* for this structure,
that will determine the row/column of our connectome matrix that this
structure corresponds to. This is dealt with by mapping the structure
indices of this lookup table to a *new* lookup table. For this example,
let's imagine that we're using the default MRtrix lookup table for the
FreeSurfer Desikan-Killiany atlas segmentation: this is provided at
``shared/mrtrix3/labelconvert/fs_default.txt``.Examining this file in detail,
we see the following:

::

    ...
    74    R.RACG  ctx-rh-rostralanteriorcingulate 80  20  140 255
    75    R.RMFG  ctx-rh-rostralmiddlefrontal     75  50  125 255
    76    R.SFG   ctx-rh-superiorfrontal          20  220 160 255
    77    R.SPG   ctx-rh-superiorparietal         20  180 140 255
    78    R.STG   ctx-rh-superiortemporal         140 220 220 255
    ...

(This file is in a slightly different format to
``FreeSurferColorLUT.txt``; don't worry about this for the time being)

This file contains the *same structure name* as the FreeSurfer look-up
table, but it is assigned a *different integer index* (76)! What's going
on?

The following is what the ``labelconvert`` command is actually going to
do under the bonnet, using these two lookup table files:

1. Read the integer value at each voxel of the input image

2. Convert the integer value into a string, based on the *input lookup
   table file* (``FreeSurferColorLUT.txt``)

3. Find this string in the *output lookup table file*
   (``fs_default.txt``)

4. Write the integer index stored in the *output lookup table file*
   for this structure to the voxel in the output image

This is what the actual command call looks like:


.. code::

    labelconvert $FREESURFER_HOME/subjects/bert/mri/aparc+aseg.mgz $FREESURFER_HOME/FreeSurferColorLUT.txt ~/mrtrix3/src/connectome/config/fs_default.txt bert_parcels.mif

And this is what the resulting image looks like:

.. figure:: https://cloud.githubusercontent.com/assets/5637955/3505537/dd15fe80-0660-11e4-92d6-cd9cc94d1acd.png
   :alt: labelconvert\_after

The integer labels of the underlying grey matter parcels have been
*converted* from the input lookup table to the output lookup table (hence
the name ``labelconvert``). They now increase monotonically from 1 to the
maximum index, with no 'gaps' (i.e. ununsed integer values) in between.
Therefore, when you construct your connectome using ``tck2connectome``,
the connectome matrix will only be as big as it needs to be to store all
of the node-node connectivity information.

Design rationale
----------------

Making this step of re-indexing parcels explicit in connectome
construction has a few distinct advantages:

* You can use parcellations from any software / atlas: just provide the
structure index / name lookup table that comes with whatever
software / atlas provides the parcellation, and define an appropriate
target lookup table that defines which index you want each structure to
map to.

* ``tck2connectome`` can be 'dumb and blind': it reads the integer indices
at either end of the streamline, and that's the row/column of the connectome
matrix that needs to be incremented.

* You can have your grey matter parcels appear in any order in your
matrices: just define a new lookup table file. Doing this prior to connectome
construction is less likely to lead to heartache than re-ordering the rows
and columns in e.g. Matlab, where you may lose track of which matrices have
been re-ordered and which have not.

* You can remove structures from the connectome, or merge multiple structures
into a single parcel, just by omitting or duplicating indices appropriately in
the target lookup table file.

* Looking at your matrices and need to find out what structure corresponds to
a particular row/column? Just look at the config file!

Obviously if your parcellation image already has node indices that increase
monotonically from 1, and you're happy enough with the numerical order of the
nodes, you don't actually need to use the ``labelconvert`` step at all.

Custom design connectomes
-------------------------

Some notes for anybody that wishes to define their own configuration
files (either for re-ordering nodes, changing selection of nodes, or
using parcellations from alternative sources):

-  If you wish to omit nodes from your connectome (e.g. the cerebellar
   hemispheres), you may be better off making these nodes the largest
   indices in your connectome, but then cropping them from the connectome
   matrices retrospectively, rather than omitting them from the parcellation
   image entirely: If you were to do the latter, streamlines that would
   otherwise be assigned to your unwanted nodes may instead be
   erroneously assigned to the nearest node that is part of your
   connectome (exactly what happens here will depend on the
   streamline-node assignment mechanism used).

-  The command ``labelconvert`` is capable of reading in look-up
   tables in a number of formats. If you wish to define your own lookup
   table, you will need to conform to one of these formats in order for
   MRtrix commands to be able to import it. If you are using an atlas
   where the look-up table does not conform to any of these formats (and
   hence MRtrix refuses to import it), you can either manually manipulate
   it into a recognized format, or if it is likely that multiple users will
   be using that parcellation scheme, we may choose to add a parser to the
   MRtrix code: contact the developers directly if this is the case.

.. |labelconvert\_after| image:: https://cloud.githubusercontent.com/assets/5637955/3505537/dd15fe80-0660-11e4-92d6-cd9cc94d1acd.png
