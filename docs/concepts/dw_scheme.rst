Diffusion gradient scheme handling
==================================

An essential piece of information for DWI processing is the diffusion-weighted
(DW) gradient scheme, also known as the "*DW gradient table*", the "*DW encoding*",
the "*b-vectors*", the "*bvecs*", and other variations on the theme. This table
provides information about the diffusion sensitisation gradients applied during
acquisition of each imaging volume in a DWI dataset, usually in the form of the
*b*-value and the (unit) vector for the DW gradient direction. In this page we
will describe the details of how this information is typically stored / 
represented, and how *MRtrix3* handles / manipulates this data.


Gradient table storage
----------------------

*MRtrix3* allows the DW gradient table to be read directly from, or written to,
the image headers for specific image formats; notably :ref:`dicom_format`
(read-only) and the :ref:`mrtrix_image_formats` (read/write).  *MRtrix3*
applications will automatically make use of this information when it is available
for the input dataset through storage of the table within the `image header`_,
without requiring explicit intervention from the user. In addition, *MRtrix3*
commands can also import or export this information from/to two different
external file formats: typically referred to as the `MRtrix format`_ and the
`FSL format`_.  These differ in a number of respects, as outlined below.

MRtrix format
.............

This format consists of a single ASCII text file, with no restrictions on the
filename. It consists of one row per entry (i.e. per DWI volume), with each row
consisting of 4 space-separated floating-point values; these correspond to
``[ x y z b ]``, where ``[ x y z ]`` are the components of the gradient vector,
and ``b`` is the *b*-value in units of s/mm². A typical *MRtrix* format DW
gradient table file might look like this:

.. code-block:: text
  :caption: **grad.b:**

           0           0           0           0
           0           0           0           0
  -0.0509541   0.0617551    -0.99679        3000
    0.011907    0.955047    0.296216        3000
   -0.525115    0.839985    0.136671        3000
   -0.785445     -0.6111  -0.0981447        3000
    0.060862   -0.456701    0.887536        3000
    0.398325    0.667699      0.6289        3000
   -0.680604    0.689645   -0.247324        3000
    0.237399    0.969995   0.0524565        3000
    0.697302    0.541873   -0.469195        3000
   -0.868811    0.407442     0.28135        3000

  ...
 
It is important to note that in this format, the direction vectors are assumed
to be provided with respect to *real* or *scanner* coordinates. This is the same
convention as is used in the DICOM format. Also note that the file does not
need to have the file type extension ``.b`` (or any other particular suffix);
this is simply a historical convention.

.. _embedded_dw_scheme:

Image header
............

When using the :ref:`mrtrix_image_formats`, *MRtrix3* has the capability of
*embedding* the diffusion gradient table *within the header of the image file*.
This provides significant advantages when performing image processing:

-  The table accompanies the image data at all times, which means that the user
   is not responsible for tracking which diffusion gradient table corresponds to
   which image file, or whether or not a particular gradient table file reflects
   some manipulation that has been applied to an image.

-  In *MRtrix3* commands that require a diffusion gradient table, and/or make
   modifications to the image data that require corresponding modifications to
   the diffusion gradient table, these data will be utilised (and/or modified)
   *automatically*, without requiring explicit intervention from the user.

For these reasons, the general recommendation of the *MRtrix3* team is to make
use of the :ref:`mrtrix_image_formats` whenever possible.

This embedding is achieved by writing an entry into the Image
:ref:`header_keyvalue_pairs`, using the key ``dw_scheme``. The value of this
entry is the complete diffusion gradient table, stored in the `MRtrix format`_.
However, this entry should generally *not be accessed or manipulated directly*
by users; instead, users should rely on the internal handling of these data as
performed by *MRtrix3* commands, or where relevant, use the command-line
options provided as part of specific *MRtrix3 commands*, as detailed later.

FSL format
..........
        
This format consists of a pair of ASCII text files, typically named ``bvecs`` & ``bvals``
(or variations thereof). The ``bvals`` file consists of a single row of
space-separated floating-point values, all in one row, with one value per
volume in the DWI dataset. The ``bvecs`` file consists of 3 rows of space-separated
floating-point values, with the first row corresponding to the *x*-component 
of the DW gradient vectors, one value per volume in the dataset; the second
row corresponding to the *y*-component, and the third row to the *z*-component.
A typical pair of FSL format DW gradient files might look like:
 
.. code-block:: text
  :caption: **bvecs:**

  0 0 -4.30812931665e-05 -0.00028279245503 -0.528846962834659 -0.781281266220383 0.014299684287952  0.36785999072309 -0.66507232482745  0.237350171404029  0.721877079467007 -0.880754419294581 0 -0.870185851757858 ...
  0 0 -0.002606397951389 -0.97091525561761 -0.846605326714759  0.615840299891175 0.403330065122241 -0.70377676751476 -0.67378508548543 -0.971399047063277 -0.513131073140676 -0.423391107245363 0 -0.416501756655988 ...
  0 0 -0.999996760803023  0.23942421337746  0.059831733802001 -0.101684552642539 0.914942902775223  0.60776414747636 -0.32201498900359  0.007004078617919 -0.464317089148873  0.212157919445896 0 -0.263255013300656 ...

.. code-block:: text
  :caption: **bvals:**

  0 0 3000 3000 3000 3000 3000 3000 3000 3000 3000 3000 ...

It is important to note that in this format, the gradient vectors are provided
*with respect to the image axes*, **not** in real or scanner coordinates
(actually, it's a little bit more complicated than that, refer to the `FSL wiki
<https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/FDT/FAQ#What_conventions_do_the_bvecs_use.3F>`_
for details). This is a rich source of confusion, since seemingly innocuous
changes to the image can introduce inconsistencies in the *b*-vectors. For
example, simply reformatting the image from sagittal to axial will effectively
rotate the *b*-vectors, since this operation changes the image axes. It is
also important to remember that a particular ``bvals/bvecs`` pair is only valid
for the particular image that it corresponds to. 


Using the DW gradient table in *MRtrix3* applications
-----------------------------------------------------

Querying the DW gradient table
..............................

As mentioned above, *MRtrix3* will use the DW gradient table from the image
headers when it is available. Currently, only the :ref:`dicom_format` and
:ref:`mrtrix_image_formats` support this. The DW gradient table can be queried
for any particular image using the ``mrinfo`` command in combination with the
``-dwgrad`` option. For example:

.. code-block:: console
  
  $ mrinfo DICOM/ -dwgrad
  mrinfo: [done] scanning DICOM folder "DICOM/"
  mrinfo: [100%] reading DICOM series "BRI 64 directions ep2d_diff_3scan_trace_p2"
            0           0           0           0
    -0.999994  0.00167109  0.00300897        3000
           -0    0.999996  0.00299996        3000
    0.0261389     0.65148   -0.758215        3000
    -0.590138   -0.767763   -0.249553        3000
     0.236087   -0.527069   -0.816371        3000
     0.893005   -0.261931    -0.36597        3000
    -0.797405    0.126351   -0.590068        3000
    -0.233751    0.930868   -0.280794        3000
    -0.936406    0.141569   -0.321095        3000
    -0.505355   -0.845584     0.17206        3000
    -0.346203   -0.848909     0.39937        3000
    -0.457204   -0.633042    0.624678        3000
      0.48716   -0.391994   -0.780395        3000
     0.617871    0.674589   -0.403938        3000
     0.577709   -0.102522    0.809779        3000
     0.825818   -0.523076   -0.210752        3000
  
  ...


Exporting the DW gradient table
...............................

This information can also be exported from the image headers using the
``-export_grad_mrtrix`` option (for the `MRtrix format`_) or
``-export_grad_fsl`` option (for the `FSL format`_) in commands that support
it. For example:

.. code-block:: console

  $ mrinfo dwi.mif -export_grad_mrtrix grad.b

results in a ``grad.b`` file in `MRtrix format`_, while:

.. code-block:: console

  $ mrconvert DICOM/ dwi.nii.gz -export_grad_fsl bvecs bvals 
  mrconvert: [done] scanning DICOM folder "DICOM/"
  mrconvert: [100%] reading DICOM series "BRI 64 directions ep2d_diff_3scan_trace_p2"
  mrconvert: [100%] reformatting DICOM mosaic images
  mrconvert: [100%] copying from "DICOM data...ns ep2d_diff_3scan_trace_p2" to "dwi.nii.gz"
  mrconvert: [100%] compressing image "dwi.nii.gz"

converts the DWI data in the ``DICOM/`` folder to
:ref:`compressed_nifti_format`, and exports the DW gradient table to `FSL
format`_ if found in the DICOM headers, resulting in a pair of ``bvecs`` &
``bvals`` files. 


Importing the DW gradient table
...............................

If the `image header`_ already contain the DW information, then no further action
is required - the *MRtrix3* application will be able to find it and use it
directly. If this is not the case (e.g. the image format does not support
including it in the header), or the information contained is not correct,
*MRtrix3* applications also allow the DW gradient table to be imported using
the ``-grad`` option (for the `MRtrix format`_) or the ``-fslgrad`` option (for
the `FSL format`_). Note that this will override the information found in the
image headers if it was there. This can be used during conversion using
``mrconvert``, or at the point of use. For example:

.. code-block:: console

  $ mrconvert dwi.nii -fslgrad dwi_bvecs dwi_bvals dwi.mif

will convert the ``dwi.nii`` from :ref:`nifti_format` to
:ref:`mrtrix_image_formats`, embedding the DW gradient table information found
in the ``dwi_bvecs`` & ``dwi_bvals`` files (in `FSL format`_) directly into the
output image header. As another example:

.. code-block:: console

  $ dwi2tensor DICOM/ -grad encoding.b tensor.nii

will process the DWI dataset found in the ``DICOM/`` folder (in
:ref:`dicom_format` format), but *override* any DW gradient information
in the DICOM data with the table stored in the `MRtrix format`_ file ``encoding.b``. 


Operations performed by *MRtrix3* when handling DW gradient tables
------------------------------------------------------------------

*MRtrix3* applications will perform a number of sanity checks and modifications
to the information in the DW gradient table, depending on the nature of the
operation, and its original format.


When using the FSL format
.........................

In this format, the gradient vectors are provided relative to the image axes
(as detailed in the `FSL wiki
<https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/FDT/FAQ#What_conventions_do_the_bvecs_use.3F>`_).
To convert them to the internal representation used in *MRtrix3* (and in the
`MRtrix format`_ gradient table), these vectors need to be transformed into the
real / scanner coordinate system. To do this requires knowledge of the DWI
dataset these vectors correspond to, in particular the image transform. In
essence, this consists of rotating the gradient vectors according to the
rotation part of the transform (i.e. the top-left 3×3 part of the matrix). This
will introduce differences between the components of the gradient vectors when
stored in `MRtrix format`_ compared to the `FSL format`_, particularly for images
not acquired in a pure axial orientation (i.e. images where the rotation part of
the image transform is identity). Indeed, as mentioned earlier, there is an
additional confound related to the handed-ness of the coordinate system; see the
`FSL wiki
<https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/FDT/FAQ#What_conventions_do_the_bvecs_use.3F>`_
for details.

.. warning:: **Never** perform a manual conversion between MRtrix and FSL
   gradient table formats using a text editor or basic shell script. This
   poses a risk of introducing an unwanted rotation / reflection of the
   gradient directions, with concomitant errors in later processing.

Note that in this operation, what matters is the transform as stored in the
NIfTI headers (i.e. the ``sform`` / ``qform``); the transform as reported by
``mrinfo`` can differ substantially from this (while still being consistent
with the data), as the *MRtrix3* image loading backend will try to provide the
image transform in a near-axial orientation (by inverting / exchanging columns
of the transform, and adjusting the :ref:`strides` to match - see
:ref:`transform` for details). To find out the actual transform that
was stored in the NIfTI header, use ``mrinfo`` with the ``-norealign`` option.


When copying or converting
..........................

Applications like ``mrconvert`` that don't actually need to interpret the DW
gradient table will simply pass the information through to the output
unmodified. If the DW gradient table was found in the input image header, it
will be written to the output image header if the image format supports it
(i.e. if the output is in :ref:`mrtrix_image_formats` - DICOM is not supported
for writing). If the DW gradient table is imported via the ``-grad`` or
``-fslgrad`` option, it will also be passed through as-is (although including
the modifications mentioned above in the `When using the FSL format`_ section).
If the output image format does not allow storing the DW gradient table in the
image header, the ``-export_grad_mrtrix`` or ``-export_grad_fsl`` options can
be used to write it out to separate files, ready for use with third-party
applications, or directly within *MRtrix3* if users prefer to keep their data
organised in this way. 


When using the information for processing
.........................................

Applications that actually need to make use of the DW gradient information
(e.g. ``dwi2tensor``, ``dwi2fod``, ``dwiextract``, ...) will perform additional
sanity checks and modifications of these data, beyond those described above.
These include:

- verifying that the number of volumes in the DWI dataset matches the number of
  entries in the DW gradient table;
- where relevant, verifying that the DW gradient tables contains the data in a
  shell structure, by clustering similar *b*-values together (see ``mrinfo``'s
  ``-shell`` and ``-shellcount`` options);
- normalising the gradient vectors to unit amplitude;
- scaling the *b*-values by the square of the gradient vector amplitude - see
  `b-value scaling`_ for details. 

Note that ``mrinfo`` will also perform most of these checks. While there is no
technical reason for it to interpret the DW gradient information, in practice
it is generally helpful to view the information as it would be interpreted by
other *MRtrix3* applications. If this is not desired, you can add the
``-raw_dwgrad`` option to ``mrinfo`` to disable these modifications when
querying the DW gradient table. 

b-value scaling
---------------

On MRI scanners that do not explicitly allow for multi-shell datasets, a
common workaround is to set the scanning protocol according to the largest
desired *b*-value, but use gradient vector directions that have *less than unit
norm*. This results in diffusion sensitisation gradients with reduced strength,
and hence images with lower *b*-values.

For example, if this was the desired gradient table:

.. code-block:: text

  0    0    0    0
  1    0    0  700
  1    0    0 2800

This could be achieved on some systems by supplying this custom diffusion
vectors file, now nominally containing only *b* = 0 and *b* = 2800 s/mm²:

.. code-block:: text

  0    0    0    0
  0.5  0    0 2800
  1    0    0 2800

By default, *MRtrix3* applications will **automatically** scale the *b*-values
by the squared amplitude of the gradient vectors (so that the stored gradient
table is equivalent to the first example), in order to more sensibly reflect
the nature of the image data.

While this scaling allows such datasets to be processed seamlessly, it will
introduce minor variations in the *b*-values for other datasets, due to minor
rounding errors in the components of the direction vectors. These are benign,
and have no consequence on the correct operation of *MRtrix3* applications,
since the deviations are typically very small, and the strategy used to group
*b*-values into shells is robust to such variations. If however this becomes a
problem (e.g. for third-party applications), this feature can be disabled
using the ``-bvalue_scaling`` option for those applications that support it.
