Smoothing of fixel data is performed based on the sparse fixel-fixel
connectivity matrix::

    fixelfilter smooth fd smooth fd_smooth -matrix ../template/matrix
    fixelfilter smooth log_fc smooth log_fc_smooth -matrix ../template/matrix
    fixelfilter smooth fdc smooth fdc_smooth -matrix ../template/matrix
 
By calling the :ref:`fixelfilter` command on each fixel directory in turn,
the smoothing filter will be appplied to all fixel data files present in
each of those directories; it is not necessary to call the :ref:`fixelfilter`
command separately for each individual fixel data file.

