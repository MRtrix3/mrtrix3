.. command-line-interface:

Command-line usage
==================

*MRtrix3* generally follows a relatively standard Unix syntax, namely:

.. code::

    $ command [options] argument1 argument2 ...

If you need to become familiar with using the command-line, there are
plenty of tutorials online to get you started. There are however a few notable 
features specific to *MRtrix3*, which are outlined below.

Using short option names
-------------------------

Options do not need to be provided in full, as long as the initial part
of the option provided is sufficient to unambiguously identify it. For
example:

.. code::

    $ mrconvert -debug in.mif out.nii.gz

is the same as:

.. code::

    $ mrconvert -de in.mif out.nii.gz

but will conflict with the ``-datatype`` option if shortened any
further:

.. code::

    $ mrconvert -d in.mif out.nii.gz 
    mrconvert: [ERROR] several matches possible for option "-d": "-datatype, "-debug"

Ordering of options on the command-line
---------------------------------------

Options can typically occur anywhere on the command-line, in any order -
they do not usually need to precede the arguments. However, there are a
few commands where the order does matter, particularly ``mrcalc``.

.. _number_sequences:

Number sequences and floating-point lists
-----------------------------------------

Options often expect arguments in the form of *number sequences* or
*floating-point lists of numbers*. The former consists or a series of
integers separated by commas or colons (no spaces), with colons
indicating a range, optionally with an increment (if different from 1).
For example:

-  ``1,4,8`` becomes ``[ 1 4 8 ]``
-  ``3,6:12,2`` becomes ``[ 3 6 7 8 9 10 11 12 2 ]``
-  ``1:3:10,8:2:0`` becomes ``[ 1 4 7 10 8 6 4 2 0 ]``

Note that the sign of the increment does not matter, it will always run
in the direction required.

Likewise, floating-point lists consist of a comma-separated list of
numbers, for example:

-  ``2.47,-8.2223,1.45e-3``


.. _unix_pipelines:

Unix Pipelines
--------------

The output of one program can be fed straight through to the input of
another program via `Unix
pipes <http://en.wikipedia.org/wiki/Pipeline_%28Unix%29>`__ in a single
command. The appropriate syntax is illustrated in this example:

.. code:: 

    $ dwi2tensor /data/DICOM_folder/ - | tensor2metric - -vector ev.mif
    dwi2tensor: [done] scanning DICOM folder "/data/DICOM_folder/"
    dwi2tensor: [100%] reading DICOM series "ep2d_diff"...
    dwi2tensor: [100%] reformatting DICOM mosaic images...
    dwi2tensor: [100%] loading data for image "ACME (hm) [MR] ep2d_diff"...
    dwi2tensor: [100%] estimating tensor components...
    tensor2metric: [100%] computing tensor metrics...

This command will execute the following actions:

1. ``dwi2tensor`` will load the input diffusion-weighted data in DICOM
   format from the folder ``/data/DICOM_folder/`` and compute the
   corresponding tensor components. The resulting data set is then fed
   into the pipe.

2. ``tensor2metric`` will access the data set from the pipe, generate an
   eigenvector map and store the resulting data set as ``ev.mif``.

The two stages of the pipeline are separated by the ``|`` symbol, which
indicates to the system that the output of the first command is to be
used as input for the next command. The image that is to be fed to or
from the pipeline is specified for each program using a single dash
``-`` where the image would normally be specified as an argument.

For this to work properly, it is important to know which arguments each
program will interpret as input images, and which as output images. For
example, this command will fail:

.. code:: 

    dwi2tensor - /data/DICOM_folder/ | tensor2metric - ev.mif

In this example, ``dwi2tensor`` will hang waiting for input data (its
first argument should be the input DWI data set). This will also cause
``tensor2metric`` to hang while it waits for ``dwi2tensor`` to provide some
input.

Advanced pipeline usage
'''''''''''''''''''''''

Such pipelines are not limited to two programs. Complex operations can
be performed in one line using this technique. Here is a longer example:

.. code:: 

    $ dwi2tensor /data/DICOM_folder/ - | tensor2metric - -vector - | mrcalc -
    mask.nii -mult - | mrview -
    dwi2tensor: [done] scanning DICOM folder "/data/DICOM_folder/"
    dwi2tensor: [100%] reading DICOM series "ep2d_diff"...
    dwi2tensor: [100%] reformatting DICOM mosaic images...
    dwi2tensor: [100%] loading data for image "ACME (hm) [MR] ep2d_diff"...
    dwi2tensor: [100%] estimating tensor components...
    tensor2metric: [100%] computing tensor metrics...
    mrcalc: [100%] computing: (/tmp/mrtrix-tmp-VihKrg.mif * mask.nii) ...

This command will execute the following actions:

1. ``dwi2tensor`` will load the input diffusion-weighted data in DICOM
   format from the folder /data/DICOM\_folder/ and compute the
   corresponding tensor components. The resulting data set is then fed
   into the pipe.

2. ``tensor2metric`` will access the tensor data set from the pipe,
   generate an eigenvector map and feed the resulting data into the next
   stage of the pipeline.

3. ``mrcalc`` will access the eigenvector data set from the pipe,
   multiply it by the image mask.nii, and feed the resulting data into
   the next stage of the pipeline.

4. ``mrview`` will access the masked eigenvector data set from the pipe
   and display the resulting image.

How is it implemented?
'''''''''''''''''''''''''''''''''''''''''

The procedure used in *MRtrix3* to feed data sets down a pipeline is
somewhat different from the more traditional use of pipes. Given the
large amounts of data typically contained in a data set, the 'standard'
practice of feeding the entire data set through the pipe would be
prohibitively inefficient. *MRtrix3* applications access the data via
memory-mapping (when this is possible), and do not need to explicitly
copy the data into their own memory space. When using pipes, *MRtrix3*
applications will simply generate a temporary file and feed its filename
through to the next stage once their processing is done. The next
program in the pipeline will then simply read this filename and access
the corresponding file. The latter program is then responsible for
deleting the temporary file once its processing is done.

This implies that any errors during processing may result in undeleted
temporary files. By default, these will be created within the ``/tmp``
folder (on Unix, or the current folder on Windows) with a filename of
the form ``mrtrix-tmp-XXXXXX.xyz`` (note this can be changed by
specifying a custom ``TmpFileDir`` and ``TmpFilePrefix`` in the :ref:`mrtrix_config`). 
If a piped command has failed, and no other *MRtrix* programs are currently running, these can be safely deleted.

*Really* advanced pipeline usage
'''''''''''''''''''''''''''''''''''''''''

As implemented, *MRtrix3* commands treat image file names that start with
the ``TmpFilePrefix`` (default is ``mrtrix-tmp-``) as temporary. When
reading the image name from the previous stage in the pipeline, the
image file name will trivially match this. But this also means that it
is possible to provide such a file as a normal *argument*, and it will
be treated as a temporary *piped* image. For example:

.. code:: 

    $ mrconvert /data/DICOM/ -datatype float32 -
    mrconvert: [done] scanning DICOM folder "/data/DICOM/"
    mrconvert: [100%] reading DICOM series "ep2d_diff"...
    mrconvert: [100%] reformatting DICOM mosaic images...
    mrconvert: [100%] copying from "ACME (hm) [MR] ep2d_diff" to "/tmp/mrtrix-tmp-zcD1nr.mif"...
    /tmp/mrtrix-tmp-zcD1nr.mif

Notice that the name of the temporary file is now printed on the
terminal, since the command's stdout has not be piped into another
command, and we specified ``-`` as the second argument. You'll also see
this file is now present in the ``/tmp`` folder. You can use this file
by copy/pasting it as an *argument* to another *MRtrix* command (be
careful though, it will be deleted once this command exits):

.. code:: 

    $ mrstats /tmp/mrtrix-tmp-zcD1nr.mif
            channel         mean       median    std. dev.          min          max       count
             [ 0 ]       1053.47           96      1324.71            0         3827       506880
             [ 1 ]       173.526           84      140.645            0          549       506880
    ...

This allows for a non-linear arrangement of pipelines, whereby multiple
pipelines can feed into a single command. This is achieved by using the
shell's output capture feature to insert the temporary file name of one
pipeline as an argument into a second pipeline. In BASH, output capture
is achieved using the ``$(commands)`` syntax, or equivalently using
backticks: ```commands```. For example:

.. code:: 

    $ dwi2tensor /data/DICOM/ - | tensor2metric - -mask $(dwi2mask /data/DICOM/ - | maskfilter - erode -npass 3 - ) -vec ev.mif -fa - | mrthreshold - -top 300 highFA.mif
    dwi2mask: [done] scanning DICOM folder "/data/DICOM/"
    dwi2tensor: [done] scanning DICOM folder "/data/DICOM/"
    dwi2mask: [100%] reading DICOM series "ep2d_diff"...
    dwi2tensor: [100%] reading DICOM series "ep2d_diff"...
    dwi2mask: [100%] reformatting DICOM mosaic images...
    dwi2tensor: [100%] reformatting DICOM mosaic images...
    dwi2mask: [100%] loading data for image "ACME (hm) [MR] ep2d_diff"...
    dwi2tensor: [100%] loading data for image "ACME (hm) [MR] ep2d_diff"...
    dwi2mask: [100%] finding min/max of "mean b=0 image"...
    dwi2mask: [done] optimising threshold...
    dwi2mask: [100%] thresholding...
    dwi2tensor: [100%] estimating tensor components...
    dwi2mask: [100%] finding min/max of "mean b=1000 image"...
    dwi2mask: [done] optimising threshold...
    dwi2mask: [100%] thresholding...
    dwi2mask: [done] computing dwi brain mask... 
    maskfilter: [100%] applying erode filter to image -... 
    tensor2metric: [100%] computing tensor metrics...
    mrthreshold: [100%] thresholding "/tmp/mrtrix-tmp-UHvhc2.mif" at 300th top voxel...

In this one command, we asked the system to perform this non-linear
pipeline:

::

                  dwi2tensor \  
                              |--> tensor2metric  ---> mrthreshold
    dwi2mask ---> maskfilter /

More specifically:

1. ``dwi2tensor`` will load the input diffusion-weighted data in DICOM
   format from the folder /data/DICOM/ and compute the corresponding
   tensor components. The resulting data set is then fed into the pipe.

   1. meanwhile, ``dwi2mask`` will generate a brain mask from the DWI
      data, and feed the result into a second pipeline.

   2. ``maskfilter`` will access the mask from this second pipeline,
      erode the mask by 3 voxels, and output the name of the temporary
      file for use as an *argument* by the next stage.

2. ``tensor2metric`` will access the tensor data set from the first
   pipe, generate eigenvector and FA maps within the mask provided as an
   *argument* by the second pipeline, store the eigenvector map in
   ``ev.mif`` and feed the FA map into the next stage of the pipeline.

3. ``mrthreshold`` will access the FA image from the pipe, identify the
   300 highest-valued voxels, and produce a mask of these voxels, stored
   in ``highFA.mif``.

