Statistical analysis using CFE is performed separately for each metric (FD, log(FC), and FDC) as follows::

     fixelcfestats fd files.txt design_matrix.txt contrast_matrix.txt tracks_2_million_sift.tck stats_fd
     fixelcfestats log_fc files.txt design_matrix.txt contrast_matrix.txt tracks_2_million_sift.tck stats_log_fc
     fixelcfestats fdc files.txt design_matrix.txt contrast_matrix.txt tracks_2_million_sift.tck stats_fdc

The input :code:`files.txt` is a text file containing the filename of each file (i.e. *not* the full path) to be analysed inside the input fixel directory, each filename on a separate line. The line ordering should correspond to the lines in the file :code:`design_matrix.txt`.

.. NOTE:: For correlation analysis, a column of 1's will *not be automatically included* (as per FSL randomise).

.. NOTE:: :code:`fixelcfestats` currently only accepts a single contrast. However if the opposite (negative) contrast is also required (i.e. a two-tailed test), then use the :code:`-neg` option. Several output files will generated all starting with the supplied prefix.

.. WARNING:: Running :code:`fixelcfestats` requires *a lot* of memory (for about 500,000 fixels in the template analysis fixel mask, 128GB of RAM is required; you can check the number of fixels in your template analysis fixel mask by :code:`mrinfo -size ./fixel_template/directions.mif`, and looking at the size of the image along the first dimension). Should hardware related limitations force you to reduce the number of fixels, it is *not* advised to change the threshold to derive the white matter template analysis fixel mask, as it may remove crucial fixels deep in the white matter (e.g. in crossing areas). Rather, consider slightly increasing the template voxel size, or spatially removing regions that are not of interest from the template mask (which was earlier obtained in this pipeline as the intersections of all subject masks in template space). Refer to this section for more information: :ref:`crash_RAM`.

