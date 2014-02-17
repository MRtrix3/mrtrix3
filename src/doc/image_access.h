/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
    to access its data. MRtrix also places no restrictions on the
    dimensionality, memory layout or data type of an image - it supports an
    arbitrary number of directions, any reasonable set of strides to navigate
    the data array, and almost all data types available, from bitwise to double
    precision complex.

    MRtrix provides many convenience template functions to operate on the
    relevant classes, assuming they all conform to the same general interface.
    For instance, there are simple functions to compute the number of voxels in
    an image, to ensure the dimensions of two images match, to loop over one or
    more images, to copy one image into the other, and more advanced functions
    to smooth images or compute gradients.

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

    In the MRtrix code and documentation, objects that provide an equivalent
    interface to Image::Info are often referred to as InfoType objects. This
    terminology is used particularly in naming template arguments. 

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
    otherwise, preserving the original data type. Individual voxel values are
    converted to and from the requested data type (\c value_type) on-the-fly,
    using dedicated functions. It is usually the most efficient way to access
    data for single-pass applications, but the overhead of converting between
    data types makes it undesirable for multi-pass applications.

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

    In the MRtrix code and documentation, objects that provide an equivalent
    interface to the Image::Buffer classes are often to referred as BufferType
    objects. This terminology is used particularly in naming template
    arguments. 

    \section image_voxel_class Image::Voxel

    This class provides access to voxel intensities at particular locations,
    indexed by their coordinates. This differs from the Image::Buffer classes,
    where the order of the voxels is dependent on the strides, and the data are
    provided as a linear array. It is a template class, and takes the
    corresponding buffer class as its template argument. For convenience, you
    can use the relevant Image::Buffer \c voxel_type member, which is a \c
    typedef to the correct type.

    The location of the voxel is set using its operator[] methods, and the
    voxel value is accessed using its value() methods. For example:
    \code
    // create an Image::Voxel pointing to buffer:
    Image::Buffer<float>::voxel_type vox (buffer);

    // set position to voxel at [ 12 3 55 ]:
    vox[0] = 12;
    vox[1] = 3;
    vox[2] = 55;

    // fetch value at that position:
    float value = vox.value();

    // perform computation ...

    // write back to same location:
    vox.value() = value;
    \endcode

    In contrast to the Image::Buffer classes, the Image::Voxel class is
    designed to be copy-constructable, making it suitable for use in
    multi-threading applications. It will keep a reference to its Image::Buffer
    class, so that all copies will be accessing the same data buffer. They can
    be used in a thread-safe manner - as long as a mechanism exists to prevent
    concurrent writes to the same voxel location.


    In the MRtrix code and documentation, objects that provide an equivalent
    interface to Image::Voxel are often to referred as VoxelType objects. This
    terminology is used particularly in naming template arguments. 

    \section image_adapter_class Image::Adapter

    Image::Adapter classes extend the Image::Voxel concept by processing the
    original voxel value from the Image::Buffer on-the-fly and returning the
    result of the computation, rather than the original voxel value itself.
    They have a similar interface to the Image::Voxel class (i.e. they are
    VoxelType objects), and can therefore be used directly in most relevant
    functions. 

    \section image_filter_class Image::Filter

    Image::Filter classes are used to implement algorithms that operate on a
    whole image, and return a different whole image. Typical usage involves
    creating an instance of the filter class based on the input image, followed
    by creation of the output image based on the filter's info() method, which
    provides a template Image::Info structure with the correct settings for the
    output image. Processing is then invoked using the filter's operator()
    method. 


    \section image_loop Image::Loop & Image::ThreadedLoop
    
    MRtrix provides a set of flexible looping functions and classes that
    support looping over an arbitrary numbers of dimensions, in any order
    desired, using the Image::Loop and Image::LoopInOrder classes. This can
    also be done in a multi-threaded context using the Image::ThreadedLoop
    class, at the cost of a slight (but worthwhile) increase in code
    complexity. These enable applications to be written that make no
    assumptions about the dimensionality of the input data - they will work
    whether the data are 3D or 5D. 

    \section image_example An example application
    
    This is a simple example illustrating the use of the methods above, in this
    case to perform multi-threaded 3x3x3 median filtering of the
    input image into the output image (similar to what the
    Image::Filter::Median3D filter does), converted to Float32 data type: 
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
      
      // create a Median3D adapter for the input voxel:
      std::vector<int> extent (1,3);
      Adapter::Median3D<InputVoxelType> median_adapter (vox_in, extent);

      // perform the processing using a simple multi-threaded copy:
      Image::threaded_copy_with_progress_message ("median filtering...", median_adapter, vox_out);

      // done!
    }
    \endcode



 */


 //! An abstract concept used to refer to image info objects 
 /*! InfoType is an abstract concept used to refer to objects that provide an
   interface equivalent to that of the Image::Info class, and can hence be
   used in the same template functions. */
 class InfoType { };

    //! An abstract concept used to refer to image buffer objects 
    /*! BufferType is an abstract concept used to refer to objects that provide an
      interface equivalent to that of the Image::Buffer classes, and can hence be
      used in the same template functions. */
    class BufferType { };

    //! An abstract concept used to refer to voxel accessor objects 
    /*! VoxelType is an abstract concept used to refer to objects that provide an
      interface equivalent to that of the Image::Voxel class, and can hence be
      used in the same template functions. */
    class VoxelType { };

}

