Generation of the fixel-fixel connectivity matrix based on the whole-brain
streamlines tractogram is performed as follows::

    fixelconnectivity fixel_mask/ tracks_2_million_sift.tck matrix/

The output directory should contain three images: `index.mif`, `fixels.mif`
and `values.mif`; these are used to encode the fixel-fixel connectivity
that is by its nature sparse.

.. WARNING:: Running :code:`fixelconnectivity` requires quite a lot of memory
   (for about 500,000 fixels in the template analysis fixel mask and a typical
   tractogram defining the pairwise connectivity between fixels, 32GB of RAM is a
   typical memory requirement; you can check the number of fixels in your template
   analysis fixel mask by :code:`mrinfo -size ./fixel_template/directions.mif`,
   and looking at the size of the image along the first dimension; also check for
   (and avoid) gross false positive connections in the tractogram, e.g.
   streamlines connecting both hemispheres in large areas where they shouldn't).
   Should hardware related limitations force you to reduce the number of fixels,
   it is *not* advised to change the threshold to derive the white matter template
   analysis fixel mask, as it may remove crucial fixels deep in the white matter
   (e.g. in crossing areas). Rather, consider slightly increasing the template
   voxel size, or spatially removing regions that are not of interest from the
   template mask (which was earlier obtained in this pipeline as the intersections
   of all subject masks in template space).
 
