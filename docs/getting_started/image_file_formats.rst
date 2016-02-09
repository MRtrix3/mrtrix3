.. _image_file_formats:

Supported image file formats
=====

MRtrix3 provides a flexible data input/output backend in the shared
library, which is used across all applications. This means that all
applications in MRtrix3 can read or write images in all the supported
formats - there is no need to explicitly convert the data to a given
format prior to processing.

However, some specialised applications may expect additional information
to be present in the input image. The MRtrix .mif/.mih formats are both
capable of storing such additional information data in their header, and
will hence always be supported for such applications. Most image formats
however cannot carry additional information in their header (or at
least, not easily) - this is in fact one of the main motivations for the
development of the MRtrix image formats. In such cases, it would be
necessary to use MRtrix format images. Alternatively, it may be
necessary to provide the additional information using command-line
arguments (this is the case particularly for the DW gradient table, when
providing DWI data in NIfTI format for instance).

Image file formats are recognised by their file extension. One exception
to this is DICOM: if the filename corresponds to a folder, it is assumed
to contain DICOM data, and the entire folder will be scanned recursively
for DICOM images.

It is also important to note that the name given as an argument will not
necessarily correspond to an actual file name on disk: in many cases,
images may be split over several files. What matters is that the text
string provided as the *image specifier* is sufficient to unambiguously
identify the full image.

.. _multi_file_image_file_formats:

Multi-file numbered image support
'''''''''''''''''''''''''''''''''

For instance, it is possible to access a numbered series of images as a
single multi-dimensional dataset, using a syntax specific to MRtrix. For
example:

.. code::

    mrinfo MRI-volume-[].nii.gz

will collate all images that match the pattern
``MRI-volume-<number>.nii.gz``, sort them in ascending numerical order,
and access them as a single dataset with dimensionality one larger than
that contained in the images. In other words, assuming there are 10
``MRI-volume-0.nii.gz`` ⇒ ``MRI-volume-9.nii.gz``, and each volume is a
3D image, the result will be a 4D dataset with 10 volumes.

Note that this isn't limited to one level of numbering:

.. code::

    mrconvert data-[]-[].nii combined.mif

will collate all images that match the ``data-number-number.nii``
pattern and generate a single dataset with dimensionality two larger
than its constituents.

Finally, it is also possible to explicitly request specific numbers,
using a :ref:`number_sequences`
within the square brackets:

.. code:: 

    mrconvert data-[10:20].nii combined.mif

DICOM (folder or .dcm)
----------------------

NIfTI-1 (.nii/.nii.gz)
----------------------

MRtrix (.mif/.mif.gz/.mih)
--------------------------

MGH (.mgh/.mgz)
---------------

Analyse / multi-file NIfTI-1 (.img pair)
----------------------------------------

Legacy MRtrix format (.mri)
---------------------------

XDS format (.bfloat/.bshort)
----------------------------

