Accessing image data       {#image_access}
====================

Access to data stored in image files is done via the dedicated classes and
functions outlined here. Most classes and algorithms included in MRtrix to
handle image data have been written using C++ templates to maximise code
re-use. They have also been written explicitly with multi-threading in mind,
and this is reflected in some of the design decisions.  MRtrix also places no
restrictions on the dimensionality, memory layout or data type of an image - it
supports an arbitrary number of dimensions, any reasonable set of strides to
navigate the data array, and almost all data types available, from bitwise to
double precision complex.

MRtrix provides many convenience template functions to operate on the relevant
classes, assuming they all conform to the same general interface.  For
instance, there are simple functions to compute the number of voxels in an
image, to ensure the dimensions of two images match, to loop over one or more
images, to copy one image into the other, and more advanced functions to smooth
images or compute gradients.

The basic architecture used in MRtrix is outlined below, along with an example
application to illustrate how the different pieces fit together.  The various
classes and functions are then described in more detail.



Overview       {#image_access_overview}
========

[Header]: @ref MR::Header
[Image]: @ref MR::Image
[Adapter]: @ref MR::Adapter
[Filter]: @ref MR::Filter
[Iterator]: @ref MR::Iterator
[Loop]: @ref MR::Loop
[ThreadedLoop]: @ref MR::ThreadedLoop

MRtrix3 defines a [Header](@ref header_class) class representing the on-disk
attributes of an image. Accessing voxel data is done via the [Image](@ref image_class) 
class, or via an [Adapter](@ref adapter_class) class providing a modified view of an
[Image](@ref image_class). 

Many functions in MRtrix3 are template calls that can operate on any object
that presents the expected interface (i.e. specific methods and attributes).
Some functions only require access to the _attributes_ of an image, not the
voxel intensities. For these functions, the template argument is typically
labelled `HeaderType`, and will accept [Header](@ref header_class), 
[Image](@ref image_class) or [Adapter](@ref adapter_class) classes. 
Other functions will also access image intensities; in such cases, the template
argument is labelled `ImageType`, and the function will accept [Image] and
[Adapter](@ref adapter_class) classes (i.e. not the [Header](@ref header_class)
class). 

The [looping functions](@ref image_loop) available in MRtrix3 fall into this
category, and illustrate the concept. These can be instantiated from a
`HeaderType` (since the image attributes are sufficient to construct the loop
object), but will operate on `ImageType` objects (since they will access the
voxel values). 

[Filter](@ref filter_class) classes are also available for common operations.


An example application      {#image_example}
======================

This simple example illustrates the use of this functionality, in this
case to perform multi-threaded 3x3x3 median filtering of the input image into
the output image (similar to what the `MR::Filter::Median` filter does),
converted to `Float32` data type: 

~~~{.cpp}
void run () 
{
  // access the input image:
  auto in = Image<float>::create (argument[0]);

  // obtain and modify the image header:
  Header header (in);
  header.datatype() = DataType::Float32;

  // create the output image based on that header:
  auto out = Image::create (argument[1], header);

  // create a Median adapter for the input voxel:
  std::vector<int> extent (1,3);
  auto median_adapter = Adapter::make<Adapter::Median> (in, extent);

  // perform the processing using a simple multi-threaded copy:
  threaded_copy_with_progress_message ("median filtering", median_adapter, out);

  // done!
}
~~~


Header       {#header_class}
======

The [Header] class contains modifiable information about an Image as stored
on disk - whether this image already exists or is about to be created.  This
includes:

- image dimensions
- image spacing (i.e. voxel sizes)
- image strides
- data type
- the format of the image
- files and byte offsets where the data are stored
- the DW gradient scheme, if found.
- any other image header information, such as comments or generic fields

The [Header] is designed to be copy-constructible (from another [Header] or any
[Image] or similar class) so that all copies are completely independent. 
It is used as-is to retrieve or specify all the relevant information for
input and output images, and is designed to be instantiated from existing images,
modified to suit, and used as a template for the output image. Instantiating a 
Header does _not_ load the image data - only when an [Image] is instantiated 
(whether directly or from a previously opened [Header]) is the data actually made 
available.

For example:
~~~{.cpp}
// open an existing image, as specified by the first argument:
auto header = Header::open (argument[0]);

// modify as necessary:
header.set_ndim (3);
header.datatype = DataType::Float32;

// create an output Header, according to the second argument.
// this header is then ready to be accessed by an Image:
auto header_out = Header::create (argument[1], header);
~~~



Image      {#image_class}
=====

The [Image] class provides access to the image data, and most of the information 
provided by the [Header]. This includes specifically:

- image dimensions
- image indices (current position)
- image spacing (i.e. voxel sizes)
- image strides
- the DW gradient scheme, if found.
- generic fields

The [Image] class is designed to be lightweight and copy-constructable, so
that all copies access the same image data. In essence, all copies of the
[Image] share a common [Header] via a shared pointer. This is essential for
multi-threading, by allowing multiple threads to each have their own
instance of an [Image], so that they can all concurrently access the image
data without affecting each other (although threads do need to ensure they
don't write to the same voxel locations concurrently - see 
@ref multithreading for details).

The [Image] class can be used to access on-disk data (existing or newly
created images), or on-RAM temporary (scratch) data. The developer can
request _direct IO_ where this can benefit performance, which means
the data will be accessed in RAM using the same type as requested in the
`ValueType` template argument (this will involve preloading if the on-disk
datatype does not match). Also, the in-RAM layout (_strides_) of the data
can be specified by the developer, to ensure contiguity of the data in RAM
in situations where this can affect performance. In general, the [Image]
class will try to access the data via memory-mapping where possible,
reverting to a preload strategy otherwise (for example, the data are stored
across too many files, or the developer has requested a data layout that
doesn't match the image on disk).

For example:
~~~{.cpp}
// open an existing header and create instantiate an [[Image]] to access the
// data as float:
auto header = Header::open (argument[0]);
auto in = header.get_image<float> (header);

// open another existing image directly, without opening the Header first
// here, the data are accessed as bool:
auto mask = Image<bool>::open (argument[1]);

// create output image based on input image's on-disk header:
auto out = Image::create (argument[2], header);

// create an output Header first, then the image.
// this time, the output image uses the in-RAM header from the mask image:
auto header_mask = Header::create (arguemnt[3], mask);
auto mask_out = header_out.get_image<bool> (header_mask);

// open another image with fast (direct) IO, with data contiguous along
// axis 3 (the volume axis):
auto image = Image<uint32_t>::open (opt[0][0]).with_direct_IO (3);
~~~



Adapter      {#adapter_class}
=========

[Adapter] classes provide a modified view into the data held by another
[Image]. This may involve on-the-fly computation of derived values (e.g. the
neighbourhood median, a smoothed version of the data, etc.), or access
to selected portions of the data (e.g. a ROI, etc.). They are written to
have a semantically identical interface to the [Image] class, so that they
can be used in place of an [Image] in many of the algorithms (e.g. the
looping functions).

For example:
~~~{.cpp}
// open an existing image:
auto in = Image<float>::open (argument[0]);

// create an Adapter::Subset to access an ROI:
std::vector<int> from = { 10, 10, 20 };
std::vector<int> dim = { 5, 5, 5 };
auto roi = Adapter::make<Adapter::Subset> (in, from, dim);

// use the adapter like any other Image:
threaded_copy (roi, output_image);
~~~





Filter      {#filter_class}
========

[Filter] classes are used to implement algorithms that operate on a whole
image, and return a different whole image. Typical usage involves creating an
instance of the filter class based on the input image, followed by creation of
the output image using the filter itself as the template [Header] 
(it derives from [Header]). Processing is then invoked using the filter's
`operator()` method. 



Loop & ThreadedLoop     {#image_loop}
=======================

MRtrix provides a set of flexible looping functions and classes that
support looping over an arbitrary numbers of dimensions, in any order
desired, using the [Loop] and [ThreadedLoop] functions and associated classes.
These enable applications to be written that make no assumptions about the
dimensionality of the input data - they will work whether the data are 3D or
5D. 


Iterator      {#iterator_class}
==========

The [Iterator] class is a simple structure containing basic information related
to an image. This includes: 

- image dimensions
- image indices (current position)

It is used as a placholder for the looping functions, in cases where the loop 
shouldn't operate on an [Image] directly.




