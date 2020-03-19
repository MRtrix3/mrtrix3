.. _batch_processing:

Batch processing with ``for_each``
==================================

Image processing often involves executing the same command on many different subjects or time points within a study. *MRtrix3* includes a Python script called ``for_each`` to simplify this process. The main benefit of using ``for_each`` compared to a bash ``for`` loop is a simpler and less verbose syntax. However other benefits include multi-threaded job execution (to exploit modern multi-core CPUs when the command being run is not already multi-threaded), and automatic identification of path basenames and prefixes. To view the full help page run ``for_each`` on the command line with no arguments.


Example 1 - using IN
--------------------
Many people like to organise their imaging datasets with one directory per subject. For example::

  study/001_patient/dwi.mif
  study/002_patient/dwi.mif
  study/003_patient/dwi.mif
  study/004_control/dwi.mif
  study/005_control/dwi.mif
  study/006_control/dwi.mif

The for_each script can be used to run the same command on each subject, for example::

.. code-block:: console

  $ for_each study/* : dwidenoise IN/dwi.mif IN/dwi_denoised.mif

The first part of the command above is the ``for_each`` script name, followed by the pattern matching string (``study/*``) to identify all the files (which in this case are directories) to be looped over. The colon is used to separate the invocation of ``for_each``, along with its inputs and any command-line options, from the command to be executed. In this example the ``dwidenoise`` command will be run multiple times, by substituting the keyword ``IN`` with each of the directories that match the pattern (``study/001_patient``, ``study/002_patient``, etc.).

Example 2 - using NAME
----------------------
Other people may pefer to organise their imaging datasets with one folder per image type and have all subjects inside. For example:

.. code-block:: text

  study/dwi/001_patient.mif
  study/dwi/002_patient.mif
  study/dwi/003_patient.mif
  study/dwi/004_control.mif
  study/dwi/005_control.mif
  study/dwi/006_control.mif

The ``NAME`` keyword can be used in this situation to obtain the basename of the file path. For example:

.. code-block:: console

  $ mkdir study/dwi_denoised
  $ for_each study/dwi/* : dwidenoise IN study/dwi_denoised/NAME

Here, the IN keyword will be substituted with the full string from the matching pattern (``study/dwi/001_patient.mif``, ``study/dwi/002_patient.mif``, etc), however the NAME keyword will be replaced with the *basename* of the matching pattern (``001_patient.mif``, ``002_patient.mif``, etc).

Alternatively, the same result can be achieved by running ``for_each`` from inside the ``study/dwi`` directory. In this case NAME would not be required. For example:

.. code-block:: console

  $ mkdir study/dwi_denoised
  $ cd study/dwi
  $ for_each * : dwidenoise IN ../dwi_denoised/IN


Example 3 - using PRE
---------------------
For this example let us assume we want to convert all dwi.mif files from example 2 to NIfTI file format (``*.nii``). This can be performed using:

.. code-block:: console

  $ for_each study/dwi/* : mrconvert IN study/dwi/PRE.nii
  $ rm *.mif

There the PRE keyword will be replaced by the file basename, without the file extension.


Example 4 - Sequential Processing
---------------------------------
As an example of a single ``for_each`` command running multiple sequential commands (e.g. with the bash ``;``, ``|``, ``&&``, ``||`` operators), let's assume in the previous example we wanted to remove the ``*.mif`` files as they were converted. We could use the ``&&`` operator, which means "run next command only if current command succeeds without error".

.. code-block:: console

  $ for_each study/dwi/* : mrconvert IN study/dwi/PRE.nii "&&" rm IN

The ``&&`` operator here must be escaped with quotes in order to prevent the shell from interpreting it. Bash operator characters can also be escaped with the "\" character; for example, to :ref:`pipe an image <unix_pipelines>` between two MRtrix commands (assuming the data set directory layout from example 1):

.. code-block:: console

  $ for_each study/* : dwiextract -bzero IN/dwi.mif - \| mrmath - mean -axis 3 IN/mean_b0.mif


Example 5 - Parallel Processing
-------------------------------
To run multiple jobs at once, use the standard *MRtrix3* command-line option ``-nthreads N``, where N is the number of concurrent jobs required. For example:

.. code-block:: console

  $ for_each study/* -nthreads 8 : dwidenoise IN/dwi.mif IN/dwi_denoised.mif

will run up to 8 of the required jobs in parallel. Note that unlike in other *MRtrix3* commands where command-line options can be placed anywhere on the command-line, in this particular context the ``-nthreads`` option must be specified *before* the colon separator. This is necessary in order for the ``for_each`` script to recognise that this command-line option applies to its own operation, as opposed to the command that ``for_each`` is responsible for invoking. To demonstrate this, consider the following usage:

.. code-block:: console

  $ for_each study/* : dwidenoise IN/dwi.mif IN/dwi_denoised.mif -nthreads 8

Here, ``for_each`` would execute the ``dwidenoise`` command entirely *sequentially*, once for each input; but each time it is run, ``dwidenoise`` would be instructed to use 8 threads.

Indeed these two usages can in theory be *combined*. Imagine that a hypothetical *MRtrix3* command, "``dwidostuff``", tends to not be capable in practise of utilising any more than four threads, regardless of how many threads are in fact available on your hardware / explicitly invoked. However you have a system with eight hardware threads, and wish to utilise them all as much as possible. In such a scenario, you could use:

.. code-block:: console

  $ for_each study/* -nthreads 2 : dwidostuff IN/dwi.mif IN/dwi_stuffdone.mif -nthreads 4

This would instruct ``for_each`` to always have *two* jobs running in parallel, each of which will be explicitly instructed to use *four* threads.

Note that most *MRtrix3* commands are multi-threaded, and will generally succeed in individually using all available CPU cores, in which case running multiple jobs in parallel using ``for_each`` is unlikely to provide a benefit in computation time (or it may in fact be detrimental). If however a particular command is known to be single-threaded (or have only limited multi-threading capability), and your system possesses enough RAM to support running multiple instances of that command at once, this usage may yield a considerable reduction in total processing time.


