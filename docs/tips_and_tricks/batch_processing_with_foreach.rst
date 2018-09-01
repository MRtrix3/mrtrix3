.. _batch_processing:

Batch processing with ``foreach``
=====================================

Image processing often involves executing the same command on many different subjects or time points within a study. *MRtrix3* includes a Python script called ``foreach`` to simplify this process. The main benefit of using ``foreach`` compared to a bash ``for`` loop is a simpler and less verbose syntax. However other benefits include multi-threaded job execution (to exploit modern multi-core CPUs when the command being run is not already multi-threaded), and automatic identification of path basenames and prefixes. To view the full help page run ``foreach`` on the command line with no arguments.


Example 1 - using IN
--------------------
Many people like to organise their imaging datasets with one directory per subject. For example::

  study/001_patient/dwi.mif
  study/002_patient/dwi.mif
  study/003_patient/dwi.mif
  study/004_control/dwi.mif
  study/005_control/dwi.mif
  study/006_control/dwi.mif

The foreach script can be used to run the same command on each subject, for example::

  $ foreach study/* "dwidenoise IN/dwi.mif IN/dwi_denoised.mif"

The first part of the command above is the ``foreach`` script name, followed by the pattern matching string (``study/*``) to identify all the files (which in this case are directories) to be looped over. The final string input is the command to be executed. In this example the ``dwidenoise`` command will be run multiple times, by substituting the keyword ``IN`` with each of the directories that match the pattern (``study/001_patient``, ``study/002_patient``, etc).

Example 2 - using NAME
-----------------------
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
  $ foreach study/dwi/* "dwidenoise IN study/dwi_denoised/NAME"

Here, the IN keyword will be substituted with the full string from the matching pattern (``study/dwi/001_patient.mif``, ``study/dwi/002_patient.mif``, etc), however the NAME keyword will be replaced with the *basename* of the matching pattern (``001_patient.mif``, ``002_patient.mif``, etc).

Alternatively, the same result can be achieved by running foreach from inside the ``study/dwi`` directory. In this case NAME would not be required. For example:

.. code-block:: console

  $ mkdir study/dwi_denoised
  $ cd study/dwi
  $ foreach * "dwidenoise IN ../dwi_denoised/IN"


Example 3 - using PRE
----------------------
For this example let us assume we want to convert all dwi.mif files from example 2 to NIfTI file format (``*.nii``). This can be performed using:

.. code-block:: console

  $ foreach study/dwi/* "mrconvert IN study/dwi/PRE.nii"
  $ rm *.mif

There the PRE keyword will be replaced by the file basename, without the file extension.


Example 4 - Sequential Processing
---------------------------------
As an example of a single ``foreach`` command running multiple sequential commands (e.g. with the bash ``;``, ``|``, ``&&``, ``||`` operators), let's assume in the previous example we wanted to remove the ``*.mif`` files as they were converted. We could use the ``&&`` operator, which means "run next command only if current command succeeds without error".

.. code-block:: console

  $ foreach study/dwi/* "mrconvert IN study/dwi/PRE.nii && rm IN"


The ``&&`` operator does not need to be escaped here since it is defined within a string input. The same applies to the piping character `|`, to :ref:`pipe an image <unix_pipelines>` between two MRtrix commands (assuming the data set directory layout from example 1):

.. code-block:: console

  $ foreach study/* "dwiextract -bzero IN/dwi.mif - | mrmath - mean -axis 3 IN/mean_b0.mif"


Example 5 - Parallel Processing
-------------------------------
To run multiple jobs at once, use the standard *MRtrix3* command-line option ``-nthreads N``, where N is the number of concurrent jobs required. For example:

.. code-block:: console

  $ foreach study/* "dwidenoise IN/dwi.mif IN/dwi_denoised.mif" -nthreads 8

will run up to 8 of the required jobs in parallel. Note that most *MRtrix3* commands are multi-threaded, and will hence *individually* use all available CPU cores; in this case, running multiple jobs in parallel using ``foreach`` is unlikely to benefit the computation time. If however a particular command is known to be single-threaded (and your system possesses enough RAM to support running multiple instances of that command at once), this usage may yield a considerable reduction in processing time.


