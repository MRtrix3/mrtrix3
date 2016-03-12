Images and other data
=========

.. _data_types:

Data types
------

*MRtrix* applications can read and write data in any of the common data
types. Moreover the *MRtrix* image file format can be used to store images in any
of these formats, while other image formats may only support a subset of
these data types. When a data type is requested that isn't supported by
the image format, a hopefully suitable alternative data type will be
used instead.

Below is a list of the supported data type and their specifiers for use
on the command-line. Note that *MRtrix* is not sensitive to the case of
the specifier: ``uint16le`` will work just as well as ``UInt16LE``.

+--------------+---------------------------------------------------------------+
| Specifier    | Description                                                   |
+==============+===============================================================+
| Bit          | bitwise data                                                  |
+--------------+---------------------------------------------------------------+
| Int8         | signed 8-bit (char) integer                                   |
+--------------+---------------------------------------------------------------+
| UInt8        | unsigned 8-bit (char) integer                                 |
+--------------+---------------------------------------------------------------+
| Int16        | signed 16-bit (short) integer (native endian-ness)            |
+--------------+---------------------------------------------------------------+
| UInt16       | unsigned 16-bit (short) integer (native endian-ness)          |
+--------------+---------------------------------------------------------------+
| Int16LE      | signed 16-bit (short) integer (little-endian)                 |
+--------------+---------------------------------------------------------------+
| UInt16LE     | unsigned 16-bit (short) integer (little-endian)               |
+--------------+---------------------------------------------------------------+
| Int16BE      | signed 16-bit (short) integer (big-endian)                    |
+--------------+---------------------------------------------------------------+
| UInt16BE     | unsigned 16-bit (short) integer (big-endian)                  |
+--------------+---------------------------------------------------------------+
| Int32        | signed 32-bit int (native endian-ness)                        |
+--------------+---------------------------------------------------------------+
| UInt32       | unsigned 32-bit int (native endian-ness)                      |
+--------------+---------------------------------------------------------------+
| Int32LE      | signed 32-bit int (little-endian)                             |
+--------------+---------------------------------------------------------------+
| UInt32LE     | unsigned 32-bit int (little-endian)                           |
+--------------+---------------------------------------------------------------+
| Int32BE      | signed 32-bit int (big-endian)                                |
+--------------+---------------------------------------------------------------+
| UInt32BE     | unsigned 32-bit int (big-endian)                              |
+--------------+---------------------------------------------------------------+
| Float32      | 32-bit floating-point (native endian-ness)                    |
+--------------+---------------------------------------------------------------+
| Float32LE    | 32-bit floating-point (little-endian)                         |
+--------------+---------------------------------------------------------------+
| Float32BE    | 32-bit floating-point (big-endian)                            |
+--------------+---------------------------------------------------------------+
| Float64      | 64-bit (double) floating-point (native endian-ness)           |
+--------------+---------------------------------------------------------------+
| Float64LE    | 64-bit (double) floating-point (little-endian)                |
+--------------+---------------------------------------------------------------+
| Float64BE    | 64-bit (double) floating-point (big-endian)                   |
+--------------+---------------------------------------------------------------+
| CFloat32     | complex 32-bit floating-point (native endian-ness)            |
+--------------+---------------------------------------------------------------+
| CFloat32LE   | complex 32-bit floating-point (little-endian)                 |
+--------------+---------------------------------------------------------------+
| CFloat32BE   | complex 32-bit floating-point (big-endian)                    |
+--------------+---------------------------------------------------------------+
| CFloat64     | complex 64-bit (double) floating-point (native endian-ness)   |
+--------------+---------------------------------------------------------------+
| CFloat64LE   | complex 64-bit (double) floating-point (little-endian)        |
+--------------+---------------------------------------------------------------+
| CFloat64BE   | complex 64-bit (double) floating-point (big-endian)           |
+--------------+---------------------------------------------------------------+

.. _image_file_formats:

Working with image data
-----

*MRtrix3* provides a flexible data input/output backend in the shared
library, which is used across all applications. This means that all
applications in *MRtrix3* can read or write images in all the supported
formats - there is no need to explicitly convert the data to a given
format prior to processing.

However, some specialised applications may expect additional information
to be present in the input image. The MRtrix .mif/.mih formats are both
capable of storing such additional information data in their header, and
will hence always be supported for such applications. Most image formats
however cannot carry additional information in their header (or at
least, not easily) - this is in fact one of the main motivations for the
development of the *MRtrix* image formats. In such cases, it would be
necessary to use *MRtrix* format images. Alternatively, it may be
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

.. _image_coord_system:

Coordinate system
'''''''''

All *MRtrix* applications will consistently use the same coordinate
system, which is identical to the
`NIfTI <http://nifti.nimh.nih.gov/nifti-1>`__ standard. Note that this
frame of reference differs from the `DICOM
standard <https://www.dabsoft.ch/dicom/3/C.7.6.2.1.1/>`__ (typically the
x & y axis are reversed). The convention followed by *MRtrix* applications
is as follows:

+---------------+-----------------------------------------+
| dimensional   | description                             |
+===============+=========================================+
| 0 (x)         | increasing from left to right           |
+---------------+-----------------------------------------+
| 1 (y)         | increasing from posterior to anterior   |
+---------------+-----------------------------------------+
| 2 (z)         | increasing from inferior to superior    |
+---------------+-----------------------------------------+

All coordinates or vector components supplied to MRtrix applications
should be provided with reference to this coordinate system.

.. _multi_file_image_file_formats:

Multi-file numbered image support
'''''''''''''''''''''''''''''''''

For instance, it is possible to access a numbered series of images as a
single multi-dimensional dataset, using a syntax specific to *MRtrix*. For
example:

.. code::

    mrinfo MRI-volume-[].nii.gz

will collate all images that match the pattern
``MRI-volume-<number>.nii.gz``, sort them in ascending numerical order,
and access them as a single dataset with dimensionality one larger than
that contained in the images. In other words, assuming there are 10
``MRI-volume-0.nii.gz`` to ``MRI-volume-9.nii.gz``, and each volume is a
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

Supported native image formats
---------

.. WARNING:: 
   Need to add!


MRtrix image formats (``.mih/.mif``)
---------

These MRtrix-specific image formats are closely related. They consist of
a text header, with data stored in binary format, either within the same
file (.mif) or as one or more separate files (.mih). In both cases, the
header structure is the same, as detailed below. These file formats were
devised to address a number of limitations inherent in currently
available formats. In particular:

-  simplicity: as detailed below, the header format is deliberately kept
   very simple and human-readable, making it easy to debug and edit
   manually if needed.
-  extendability: any information can be stored in the header, and will
   simply be ignored by the application if not recognised.
-  arbitrary data organisation: voxel values can be stored in any order,
   making it simple to ensure for example that all FOD coefficients for
   a given voxel are stored contiguously on file.

Note that *MRtrix* now includes *MatLab* functions to read and write MRtrix
image files, and to load MRtrix tracks files. These are located in the
``matlab`` subfolder.

Header structure
''''''''''

The header is the first (and possibly only) data stored in the file, as
ASCII-encoded text (although other encodings such as UTF8 may work
equally well). Lines should be separated by unix-style newlines
(line-feed, '', ASCII 0x0A), although MRtrix will also accept DOS-type
newlines.

The first line should read only ``mrtrix image`` to indicate that this
is an image in MRtrix format. The last line of the header should read
only ``END`` to signal the end of the header, after which all data will
be considered as binary.

All following lines are in the format ``key: value``, with the value
entry extending up to the end of the line. All whitespace characters
before and after the value entry are ignored. Some keys are required to
read the images, others are optional, and any key not recognised by
MRtrix will simply be ignored. Recognised keys are listed below, along
with the expected format of the corresponding values.

-  **dim** [required]

the image dimensions, supplied as a comma-separated list of integers.
The number of entries specifies the dimensionality of the image. For
example: ``dim: 192,256,256`` specifies a 192×256×256 image.

-  **vox** [required]

   the voxel size along each dimension, as a comma-separated list of
   floating-point values. The number of entries should match that given
   in the dim entry. For example: ``vox: 0.9,0.898438,0.898438``.
-  **layout** [required]

   specifies the organisation of the data on file. In simplest terms, it
   provides a way of specifying the strides required to navigate the
   data file, in combination with the dim entry. It is given as a
   comma-separated list of signed integers, with the sign providing the
   direction of data traversal with respect to voxel coordinates, and
   the value providing a way of specifying the order of increasing
   stride.

   For example, assuming an image with ``dim: 192,256,256``, the entry
   ``layout: +2,-0,-1`` is interpreted as: the shortest stride is along
   the y-axis (second entry), then the z-axis (third entry), and then
   along the x-axis. Voxels are stored in the order left to right
   (positive stride) along the x-axis; anterior to posterior along the
   y-axis (negative stride); and superior to inferior (negative stride)
   along the z-axis. Given the image dimensions, the final strides are
   therefore 256×256=65536 for adjacent voxels along the x-axis, -1 for
   the y-axis, and -256 for the z-axis. This also implies that the voxel
   at coordinate [ 0 0 0 ] is located 65536 voxel values into the data
   portion of the file.

-  **datatype** [required]

   the datatype used to store individual voxel values. See the listing of 
   valid :ref:`data_types`. For example: ``datatype: UInt16LE``

-  **file** [required]

   specifies where the binary image data are stored, in the format file:
   filename offset, with the offset provided in bytes from the beginning
   of the file. For example: ``file: image.dat 0``.

   For the single-file format (.mif), the filename should consists of a
   single full-stop ('.') to indicate the current file, and the offset
   should correspond to a point in the file after the END statement of
   the header.

   For the separate header/data format (.mih), the filename should refer
   to an existing file in the same folder as the header (.mih) file.
   Multiple such entries can be supplied if the data are stored across
   several files.

-  **transform** [optional]

   used to supply the 4×4 transformation matrix specifying the
   orientation of the axes with respect to real space. This is supplied
   as a comma-separated list of floating-point values, and only the
   first 12 such values will be used to fill the first 3 rows of the
   transform matrix. Multiple such entries can be provided to fill the
   matrix; for example, MRtrix will normally produce 3 lines for the
   transform, with one row of 4 values per entry:

   ::

       transform: 0.997986,-0.0541156,-0.033109,-74.0329
       transform: 0.0540858,0.998535,-0.00179436,-100.645
       transform: 0.0331575,2.34007e-08,0.99945,-125.84

-  **comments** [optional]

   used to add generic comments to the header. Multiple such entries can
   be provided. For example: ``comment: some information``

-  **scaling** [optional]

   used to specify how intensity values should be scaled, provided as an
   offset and scale. Voxel values will be read as value\_returned =
   offset + scale \* value\_read. For example: ``scaling: -1,2``.
   Default is ``0,1`` (no modification).

MRtrix sparse image formats (``.msh/.msf``)
-------

These new image formats are designed for applications where the number
of discrete elements within a voxel may vary between voxels. The most
likely use case here is where each voxel contains some number of
discrete fibre populations ('fixels'), and some information associated
with each of these elements must be stored. Since only as many elements
are as required for any particular voxel are actually stored, rather
than having to store the maximum possible number for all voxels and
padding with empty data, the format is referred to as 'sparse'.

Much like the standard MRtrix image formats (.mif and .mih), there are
two different image file extensions available. One (.msh) separates the
image header information and raw data into separate files, while the
other (.msf) encodes all information relevant to the image into a single
file.

However unlike these established formats, sparse images contain *two*
separate raw data fields. The first of these behaves identically to
standard images: a single intensity value for every image element. The
second stores sparse image data. For any particular image element, the
intensity value within the standard image field defines a *pointer* to a
location within the sparse image field, where the sparse data relevant
for that image element can be found.

Additional image header features
''''''''

These image formats have some features within the image header that
differ from the standard MRtrix image formats:

-  The 'magic number' that appears at the start of the file must read
   'mrtrix sparse image'.
-  Key:value pair 'sparse\_data\_name' defines the *name* of the class
   used in the sparse data field. This class name is typically not
   reader-friendly; the value that appears is that provided by the C++
   call ``typeid(XYZ).name()`` for a class called XYZ. This is necessary
   to ensure that the data stored in the sparse field can be interpreted
   correctly.
-  Key:value pair 'sparse\_data\_size' defines the size (in bytes) of
   the class used to store the sparse data.
-  The 'datatype' field MUST be a 64-bit integer, with the same
   endianness as the system. A 64-bit integer type is required because
   the standard image data provides pointers to the sparse data in
   memory, while the endianness is tested to ensure that the sparse data
   can be interpreted correctly. Note that sparse images cannot be
   transferred and used between systems with different endianness.
-  In addition to the 'file' key, a second key 'sparse\_file' is also
   required, which provides the path to the beginning of the sparse
   image data. In the .msf format, this provides an offset from the
   start of the file to the start of the sparse data field; in the .msh
   format, a second associated data file with the extension .sdat is
   generated on image creation, and the path to this file is defined in
   the hedaer.

Sparse data storage
'''''''''''

Within the sparse data field, there is no delimiting information or
identifying features; the image format relies on the integers stored in
the standard image field to provide offset pointers to appropriate
locations within the sparse field.

From the data position defined by such an offset, the first 4 bytes
provide a 32-bit integer (with native endianness), which specifies the
number of discrete elements stored. This is followed by data to fill
precisely that number of instances of the sparse data class. Note that
no endianness conversion can be performed on this data; data is read and
written using a straight memory copy.


Tracks file format (``.tck``)
-------

The format for track files is similar to that for MRtrix-format images.
It consists of a text header in the same key: value format, ending with
a single 'END' statement, and followed by binary data.

The first line of the header should read ``mrtrix tracks`` to indicate
that the file contains tracks in MRtrix format. Further ``key: value``
pairs typically provide information about the parameters used to produce
the tracks, and for the most part are not required to read the data. The
only required keys are the following:

-  **file**

   a ``file: . offset`` entry is required to specify the byte offset
   from the beginning of the file to the start of the binary track data.
   At this stage, only the single-file format is supported - in other
   words the filename part must be specified as '.' (see above for
   details).

-  **datatype**

   specifies the datatype (and byte order). At this points only the
   Float32 data type is supported, either as little-endian (LE) or
   big-endian (BE).

   The binary track data themselves are stored as triplets of
   floating-point values (at this stage in 32 bit floating-point
   format), one per vertex along the track. Tracks are separated using a
   triplet of NaN values. Finally, a triplet of Inf values is used to
   indicate the end of the file.

