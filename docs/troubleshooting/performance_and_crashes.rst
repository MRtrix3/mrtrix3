Crashes, errors and performance issues
======================================

Throughout *MRtrix3* we try to provide users with meaningful error messages
if something does not work or sensible default behaviour cannot be determined.
However in some cases, feedback to the user can be unavoidably difficult to
interpret, or non-existent in cases where performance is poor but a command
does not fail outright; this page is a collection of information of such
cases.


Compiler error during build
---------------------------

If you encounter an error during the build process that resembles the following:

.. code-block:: text

    ERROR: (#/#) [CC] release/cmd/command.o

    /usr/bin/g++-4.8 -c -std=c++11 -pthread -fPIC -I/home/user/mrtrix3/eigen -Wall -O2 -DNDEBUG -Isrc -Icmd -I./lib -Icmd cmd/command.cpp -o release/cmd/command.o

    failed with output

    g++-4.8: internal compiler error: Killed (program cc1plus)
    Please submit a full bug report,
    with preprocessed source if appropriate.
    See for instructions.


This is most typically caused by the compiler running out of RAM. This
can be solved either through installing more RAM into your system, or
by restricting the number of threads to be used during compilation:

.. code-block:: console

    $ NUMBER_OF_PROCESSORS=1 ./build


.. _crash_RAM:

Commands crashing due to memory requirements
--------------------------------------------

Some commands in *MRtrix3* have substantial RAM requirements, and can
therefore fail even on a relatively modern machine:

-  ``tcksift`` and ``tcksift2`` must store, for every streamline,
   a list of every `fixel <Dixels-and-Fixels>`__ traversed, with
   an associated streamline length through each voxel.

-  ``fixelcfestats`` must store a sparse matrix of fixel-fixel connectivity
   between fixels in the template image.

In both of these cases, the memory requirements increase in proportion to
the number of streamlines (directly proportionally in the case of SIFT/SIFT2,
less so in the case of Connectivity-based Fixel Enhancement (CFE)). They also
depend on the spatial resolution: If the voxel size is halved, the number
of unique fixels traversed by each individual streamline will go up by a
factor of around 3, with a corresponding increase in RAM usage in SIFT/SIFT2;
but the total number of unique fixels increases by up to 8, and hence the total
number of possible fixel-fixel connections goes up by a factor of 64! The RAM
usage of CFE therefore increases by a substantial amount as the resolution of
the template is increased. Unfortunately in both of these cases it is
theoretically impossible to reduce the RAM requirements in software any
further than has already been done; the information stored is fundamental to
the operation of these algorithms.

In both cases, the memory usage can be reduced somewhat by reducing the number
of streamlines; this can however be detrimental to the quality of the
analysis. Possibly a better solution is to reduce the spatial resolution
of the underlying image, reducing the RAM usage without having too much
influence on the outcomes of such algorithms.

For SIFT/SIFT2, the subject FOD image can be down-sampled using e.g.:

.. code-block:: console

    $ mrresize in.mif out.mif -scale 0.5

Note that it is not necessary to use this down-sampled image for tractography,
nor for any other processing; it is simply used for SIFT/SIFT2 to reduce
memory usage. Additionally, by performing this down-sampling using *MRtrix3*
rather than some other software, it will ensure that the down-sampled image is
still properly aligned with the full-resolution image in scanner space,
regardless of the image header transformation.

For CFE, it is the resolution of the *population template image* that affects
the memory usage; however using higher-resolution images for registration
when *generating* that population template may still be beneficial. Therefore
we advocate downsampling the population template image after its generation,
and otherwise proceed with Fixel-Based Analysis (FBA) using this down-sampled
template image.


Scripts crashing due to storage requirements
--------------------------------------------

The Python scripts provided with *MRtrix* generate their own temporary
directory in which to store various data files and image manipulations
generated during their operation. In some cases - typically due to use of a
temporary RAM-based file system with limited size, and/or a failure to clean
up old temporary files - the location where this temporary directory is
created may run out of storage space, resulting in the script crashing out.

A few pointers for anybody who encounters this issue:

-  When these scripts fail to complete due to an error, they will typically
   *not* erase the temporary directory, instead allowing the user to
   investigate the contents of that directory to see what went wrong,
   potentially fixing any issues and continuing the script from that point.
   While this behaviour may be useful in this context by retaining the
   progress the script had made thus far, it also means that these very
   scripts may be contributing to filling up your storage and thus creating
   further issues! We recommend that users manually delete such directories
   as soon as they are no longer required.

-  The location where the temporary directory is created for the script will
   influence the amount of storage space available. For instance, the
   location ``/tmp//`` is frequently created as a temporary RAM-based file
   system, such that the script's temporary files are never actually written
   to disk and are therefore read & written very quickly; it is however also
   likely to have a smaller capacity than a physical hard drive.

   This location can be set manually in two different ways:

   -  In the MRtrix _Configuration_file, key "ScriptTmpDir" can be used to
      set the location where such temporary directories will be created by
      default.

   -  When executing the script, command-line option ``-tempdir`` can be
      used to set the location of the temporary directory for that particular
      script execution.

   In the absence of either of these settings, *MRtrix3* will now create this
   temporary directory in the *working directory* (i.e. the location the
   terminal was navigated to when the script was called), in the hope that it
   will reduce the prevalence of users encountering this issue. This may
   however cause issues if working across a network, or using a job scheduler.

-  The storage requirements can vary considerably between different scripts.
   For instance, ``dwibiascorrect`` only needs to generate a couple of
   temporary images per execution; whereas ``population_template`` must
   store non-linear warp fields across many subjects. This may explain why
   one script crashed when other scripts have completed successfully.


Hanging on network file system when writing images
--------------------------------------------------

When any *MRtrix3* command must read or write image data, there are two
primary mechanisms by which this is performed:

1. `Memory mapping <https://en.wikipedia.org/wiki/Memory-mapped_file>`_:
The operating system provides access to the contents of the file as
though it were simply a block of data in memory, without needing to
explicitly load all of the image data into RAM.

2. Preload / delayed write-back: When opening an existing image, the
entire image contents are loaded into a block of RAM. If an image is
modified, or a new image created, this occurs entirely within RAM, with
the image contents written to disk storage only at completion of the
command.

This design ensures that loading images for processing is as fast as
possible and does not incur unnecessary RAM requirements, and writing
files to disk is as efficient as possible as all data is written as a
single contiguous block.

Memory mapping will be used wherever possible. However one circumstance
where this should *not* be used is when *write access* is required for
the target file, and it is stored on a *network file system*: in this
case, the command typically slows to a crawl (e.g. progressbar stays at
0% indefinitely), as the memory-mapping implementation repeatedly
performs small data writes and attempts to keep the entire image data
synchronised.

*MRtrix3* will now *test* the type of file system that a target image is
stored on; and if it is a network-based system, it will *not* use
memory-mapping for images that may be written to. *However*, if you
experience the aforementioned slowdown in such a circumstance, it is
possible that the particular configuration you are using is not being
correctly detected or identified. If you are unfortunate enough to
encounter this issue, please report to the developers the hardware
configuration and file system type in use.


Linux: very slow performance when writing large images
------------------------------------------------------

This might be due to the Linux Disk Caching or the kernel's handling of `dirty
pages
<https://lonesysadmin.net/2013/12/22/better-linux-disk-caching-performance-vm-dirty_ratio/>`__.

On Ubuntu, you can get your current dirty page handling settings with ``sysctl -a | grep dirty``.
Those settings can be modified in ``/etc/sysctl.conf`` by adding the following
two lines to ``/etc/sysctl.conf``:

.. code-block:: text

    vm.dirty_background_ratio = 60
    vm.dirty_ratio = 80

``vm.dirty_background_ratio`` is a percentage fraction of your RAM and should
be larger than the image to be written.  After changing ``/etc/sysctl.conf``,
execute ``sysctl -p`` to configure the new kernel parameters at runtime.
Depending on your system, these changes might not be persistent after reboot.


``mrview`` unable to open images: "Too many open files"
-------------------------------------------------------

It is possible to encounter this error message particularly if trying to
open a large number of DICOM images. In most cases, each slice in a
DICOM series is stored in an individual file; all of these files must remain
open while the image is loaded. In addition, the maximum number of files
open at any time (imposed by the kernel, *not* *MRtrix3*) may be relatively
small (e.g. 256), such that very few subjects can be opened at once.

There are two ways to solve this issue:

-  *Reduce the number of files opened concurrently*: By converting each series
   of interest to an alternative format (e.g. :ref:`mrtrix_image_formats`)
   before opening them in ``mrview``, the total number of files open at once
   will be drastically reduced.

-  *Increase the limit on number of files opened*: If directly opening DICOM
   images without first converting them is more convenient, then it is
   possible to instead increase the kernel's upper limit on the number of
   files that can remain open at once. The specific details on how this is
   done may vary between different OS's / distributions, but here are a couple
   of suggestions to try:

   -  The current limit should be reported by:

      ::

         ulimit -n

   -  Try running the following (potentially with the use of ``sudo``):

      ::

         sysctl -w fs.file-max=100000

      If this solves the issue, the change can be made permanent by editing
      file ``/etc/sysctl.conf``, adding the following line (replacing
      ``<number>`` with your desired upper limit):

      ::

         fs.file-max = <number>

      On MacOSX, you may instead need to look at the ``kern.maxfiles`` and
      ``kern.maxfilesperproc`` parameters.

   -  Set the new upper limit using ``ulimit`` (you can try using a number
      instead of "unlimited" if you choose to):

      ::

         ulimit -n unlimited

      If this works, you will need to add that line to a file such as
      ``~/.bashrc`` in order for the change to be applied permanently.
