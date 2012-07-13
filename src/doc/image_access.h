/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 16/08/09.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#error - this file is for documentation purposes only!
#error - It should NOT be included in other code files.

namespace MR
{

  /*! \page image_access Accessing image data

    Access to data stored in image files is done via the classes and functions
    defined in the Image namespace. The corresponding headers are stored in the
    \c lib/image/ directory.  Most classes and algorithms included in MRtrix to
    handle image data have been written using C++ templates to maximise code
    re-use. They have also been written explicitly with multi-threading in
    mind, and this is reflected some of the design decisions - in particular
    the separation between an Image::Buffer and the Image::Voxel classes used
    to access its data. 

    The basic architecture used in MRtrix is outlined below. An example is also
    given at the end to illustrate the use of these classes.


    \section image_info_class Image::Info

    The Image::Info is a simple structure containing basic information related to an image. This includes: 
    - image dimensions
    - voxel sizes
    - data strides
    - data type
    - transform

    It is used as a base class (or at least template) for all other Image
    classes, and provides the blueprint for the interface expected by most
    template functions and algorithms that operate on images.

    \section image_header_class Image::Header

    The Image::Header class extends the Image::Info class with information
    relevant for actual files as stored on disk. Aside from the information in
    the Image::Info class, this includes:
    - the format of the image
    - files and byte offsets where the data are stored
    - the DW gradient scheme, if found.
    - any other image header information, such as comments or generic fields

    It is used as-is to retrieve or specify all the relevant information for
    input and output images, and is used as the base class for some of the
    Image::Buffer classes.

    \section image_buffer_class Image::Buffer

    The Image::Buffer classes provide an array-like view into the image data.
    There are 3 flavours of Image::Buffer, each templated based on the data
    type required by the application. The basic types are:

    \par Image::Buffer&lt;value_type&gt;
    This is the standard way to access image data. It will attempt to access
    the data using memory-mapping where possible, and load the data into RAM
    otherwise, preserving the original data type. Voxel values are converted to
    and from the requested data type (\c value_type) on-the-fly, using
    dedicated functions. It is usually the most efficient way to access data
    for single-pass applications, but the overhead of converting between data
    types makes it undesirable for multi-pass applications.

    \par Image::BufferPreload&lt;value_type&gt;
    This class provides much faster, direct access to the data, and is much
    more suitable for multi-pass algorithms, or applications that rely on a
    specific memory layout of the data (strides). It will access the data via
    memory-mapping only if the data type on file matches that requested (\c
    value_type), and the strides match those optionally specified. Otherwise,
    the data are loaded into RAM and converted to the requested layout and data
    type.

    \par Image::BufferScratch&lt;value_type&gt;
    This class provides a RAM-based scratch buffer, with no associated file on
    disk.

    \section image_voxel_class Image::Voxel

    This class provides access to voxel intensities at particular locations,
    indexed by their coordinates. This differs from the Image::Buffer classes,
    where the order of the voxels is dependent on the strides, and the data are
    provided as a linear array. It is a template class, and takes the
    corresponding buffer class as an argument. For convenience, you can use the
    relevant Image::Buffer \c voxel_type member, which is a \c typedef to the
    correct type.

    The location of the voxel is set using its operator[] methods, and the
      voxel value is accessed using its value() methods.

    In contrast to the Image::Buffer classes, the Image::Voxel class is
    designed to be copy-constructable, making it suitable for use in
    multi-threading applications. It will keep a reference to its Image::Buffer
    class, so that all copies will be accessing the same data buffer. They can
    be used in a thread-safe manner - as long as a mechanism exists to prevent
    concurrent write access to the same voxel location.

    \section image_adapter_class Image::Adapter

    TODO

    \section image_filter_class Image::Filter


    TODO

    \section image_example An example application
    
    This is a simple example illustrating the use of the methods above, in this
    case to perform a multi-threaded copy of the input image into the output
    image, converted to Float32 data type: 
    \code
    void run () 
    {
      // access the input image:
      Image::Buffer<float> buffer_in (argument[0]);

      // obtain and modify the image header:
      Image::Header header (buffer_in);
      header.datatype() = DataType::Float32;

      // create the output image based on that header:
      Image::Buffer<float> buffer_out (argument[1], header);

      // create Image::Voxel classes to access the data:
      Image::Buffer<float>::voxel_type vox_in (buffer_in);
      Image::Buffer<float>::voxel_type vox_out (buffer_out);
      
      // perform the multi-threaded copy:
      Image::threaded_copy (vox_in, vox_out);

      // done!
    }
    \endcode



 */
 
}

