Statistical analysis using CFE is performed separately for each metric
(FD, log(FC), and FDC) as follows::

     fixelcfestats fd_smooth/ files.txt design_matrix.txt contrast_matrix.txt matrix/ stats_fd/
     fixelcfestats log_fc_smooth/ files.txt design_matrix.txt contrast_matrix.txt matrix/ stats_log_fc/
     fixelcfestats fdc_smooth/ files.txt design_matrix.txt contrast_matrix.txt matrix/ stats_fdc/

The input :code:`files.txt` is a text file containing the filename of each
file (i.e. *not* the full path) to be analysed inside the input fixel
directory, each filename on a separate line. The line ordering should
correspond to the lines in the file :code:`design_matrix.txt`.

.. NOTE:: While previous versions of :ref:`fixelcfestats` performed smoothing
   of the input fixel data as those data were imported, this is *no longer the case*.
   It is expected that the fixel data provided as input to :ref:`fixelcfestats`
   will already have been appropriately smoothed; e.g. using :ref:`fixelfilter`.

.. NOTE:: Unlike some other software packages providing a GLM (e.g. FSL
   :code:`randomise`), a column of 1's (corresponding to the "global intercept":
   the mean image value when all other design matrix factors are zero) will
   *not be automatically included*. If the model for your experiment requires
   such a factor, it is necessary to include it explicitly in the design matrix.

