Hanging or Crashing
===================


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


.. _crash_ram:

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
and otherwise proceed with FIxel-Based Analysis (FBA) using this down-sampled
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

