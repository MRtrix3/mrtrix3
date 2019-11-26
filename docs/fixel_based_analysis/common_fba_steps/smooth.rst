Smoothing of fixel data is performed based on the sparse fixel-fixel
connectivity matrix::

    fixelfilter fd smooth fd_smooth -matrix matrix/
    fixelfilter log_fc smooth log_fc_smooth -matrix matrix/
    fixelfilter fdc smooth fdc_smooth -matrix matrix/
 
By calling the :ref:`fixelfilter` command on each fixel directory in turn,
the smoothing filter will be appplied to all fixel data files present in
each of those directories; it is not necessary to call the :ref:`fixelfilter`
command separately for each individual fixel data file.

