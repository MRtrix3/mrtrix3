.. _environment_variables:

##########################################
List of MRtrix3 environment variables
##########################################

.. envvar:: DICOM_ID

     when reading DICOM data, match the PatientID entry against
     the string provided

.. envvar:: DICOM_PATIENT

     when reading DICOM data, match the PatientName entry against
     the string provided

.. envvar:: DICOM_SERIES

     when reading DICOM data, match the SeriesName entry against
     the string provided

.. envvar:: DICOM_STUDY

     when reading DICOM data, match the StudyName entry against
     the string provided

.. envvar:: MRTRIX_CONFIGFILE

     This can be used to set the location of the system-wide
     configuration file. By default, this is ``/etc/mrtrix.conf``.
     This can be useful for deployments where access to the system's
     ``/etc`` folder is problematic, or to allow different versions of
     the software to have different configurations, etc.

.. envvar:: MRTRIX_LOGLEVEL

     Set the default terminal verbosity. Default terminal verbosity
     is 1. This has the same effect as the ``-quiet`` (0),
     ``-info`` (2) or ``-debug`` (3) comand-line options.

.. envvar:: MRTRIX_NOSIGNALS

     If this variable is set to any value, disable MRtrix3's custom
     signal handlers. This may sometimes be useful when debugging.
     Note however that this prevents the
     deletion of temporary files when the command terminates
     abnormally.

.. envvar:: MRTRIX_NTHREADS

     set the number of threads that MRtrix3 applications should use.
     This overrides the automatically determined number, or the
     :option:`NumberOfThreads` setting in the configuration file, but
     will be overridden by the ENVVAR ``-nthreads`` command-line option.

.. envvar:: MRTRIX_PRESERVE_PHILIPS_ISO

     Do not remove the synthetic isotropically-weighted diffusion
     image often added at the end of the series on Philips
     scanners. By default, these images are removed from the series
     to prevent errors in downstream processing. If this
     environment variable is set, these images will be preserved in
     the output.
     
     Note that it can be difficult to ascertain which volume is the
     synthetic isotropically-weighed image, since its DW encoding
     will normally have been modified from its initial value
     (e.g. [ 0 0 0 1000 ] for a b=1000 acquisition) to b=0 due to
     b-value scaling.

.. envvar:: MRTRIX_QUIET

     Do not display information messages or progress status. This has
     the same effect as the ``-quiet`` command-line option. If set,
     supersedes the MRTRIX_LOGLEVEL environment variable.

.. envvar:: MRTRIX_RNG_SEED

     Set the seed used for the random number generator.
     Ordinarily, MRtrix applications will use random seeds to ensure
     repeat runs of stochastic processes are never the same.
     However, when experimenting or debugging, it may be useful to
     explicitly set the RNG seed to ensure reproducible results across
     runs. To do this, set this variable to a fixed number prior to
     running the command(s).
     
     Note that to obtain the same results
     from a multi-threaded command, you should also disable
     multi-threading (using the option ``-nthread 0`` or by
     setting the :envvar:`MRTRIX_NTHREADS` environment variable to zero).
     Multi-threading introduces randomness in the order of execution, which
     will generally also affect the reproducibility of results.

.. envvar:: MRTRIX_TMPFILE_DIR

     This has the same effect as the :option:`TmpFileDir`
     configuration file entry, and can be used to set the location of
     temporary files (as used in Unix pipes) for a single session,
     within a single script, or for a single command without
     modifying the configuration  file.

.. envvar:: MRTRIX_TMPFILE_PREFIX

     This has the same effect as the :option:`TmpFilePrefix`
     configuration file entry, and can be used to set the prefix for
     the name  of temporary files (as used in Unix pipes) for a
     single session, within a single script, or for a single command
     without modifying the configuration file.

