/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __image_h__
#define __image_h__

#include <functional>
#include <type_traits>
#include <tuple>

#include "debug.h"
#include "header.h"
#include "image_io/fetch_store.h"
#include "image_helpers.h"
#include "formats/mrtrix_utils.h"
#include "algo/copy.h"
#include "algo/threaded_copy.h"

namespace MR
{

  constexpr int SpatiallyContiguous = -1;


  template <typename ValueType>
    class Image {
      public:
        typedef ValueType value_type;
        class Buffer;

        Image ();
        FORCE_INLINE Image (const Image&) = default;
        FORCE_INLINE Image (Image&&) = default;
        FORCE_INLINE Image& operator= (const Image& image) = default;
        FORCE_INLINE Image& operator= (Image&&) = default;
        ~Image();

        //! used internally to instantiate Image objects
        Image (const std::shared_ptr<Buffer>&, const Stride::List& = Stride::List());

        FORCE_INLINE bool valid () const { return bool(buffer); }
        FORCE_INLINE bool operator! () const { return !valid(); }

        //! get generic key/value text attributes
        FORCE_INLINE const std::map<std::string, std::string>& keyval () const { return buffer->keyval(); }

        FORCE_INLINE const std::string& name() const { return buffer->name(); }
        FORCE_INLINE const transform_type& transform() const { return buffer->transform(); }

        FORCE_INLINE size_t  ndim () const { return buffer->ndim(); }
        FORCE_INLINE ssize_t size (size_t axis) const { return buffer->size (axis); }
        FORCE_INLINE default_type spacing (size_t axis) const { return buffer->spacing (axis); }
        FORCE_INLINE ssize_t stride (size_t axis) const { return strides[axis]; }

        //! offset to current voxel from start of data
        FORCE_INLINE size_t offset () const { return data_offset; }

        //! reset index to zero (origin)
        FORCE_INLINE void reset () {
          for (size_t n = 0; n < ndim(); ++n)
            index(n) = 0;
        }

        //! get position of current voxel location along \a axis
        FORCE_INLINE ssize_t index (size_t axis) const { return x[axis]; }

        //! get/set position of current voxel location along \a axis
        FORCE_INLINE auto index (size_t axis) -> decltype (Helper::index (*this, axis)) { return { *this, axis }; }
        FORCE_INLINE void move_index (size_t axis, ssize_t increment) { data_offset += stride (axis) * increment; x[axis] += increment; }

        FORCE_INLINE bool is_direct_io () const { return data_pointer; }

        //! get voxel value at current location
        FORCE_INLINE ValueType value () const {
          if (data_pointer) return Raw::fetch_native<ValueType> (data_pointer, data_offset);
          return buffer->get_value (data_offset);
        }
        //! get/set voxel value at current location
        FORCE_INLINE auto value () -> decltype (Helper::value (*this)) { return { *this }; }
        FORCE_INLINE void set_value (ValueType val) {
          if (data_pointer) Raw::store_native<ValueType> (val, data_pointer, data_offset);
          else buffer->set_value (data_offset, val);
        }

        //! get set/set a row of values at the current index position along the specified axis
        FORCE_INLINE Eigen::Map<Eigen::Matrix<value_type, Eigen::Dynamic, 1 >, Eigen::Unaligned, Eigen::InnerStride<> > row (size_t axis)
        {
          assert (is_direct_io() && "Image::row() method can only be used on Images loaded using Image::with_direct_io()");
          index (axis) = 0;
          return Eigen::Map<Eigen::Matrix<value_type, Eigen:: Dynamic, 1 >, Eigen::Unaligned, Eigen::InnerStride<> >
                   (address(), size (axis), Eigen::InnerStride<> (stride (axis)));
        }

        //! use for debugging
        friend std::ostream& operator<< (std::ostream& stream, const Image& V) {
          stream << "\"" << V.name() << "\", datatype " << DataType::from<Image::value_type>().specifier() << ", index [ ";
          for (size_t n = 0; n < V.ndim(); ++n) stream << V.index(n) << " ";
          stream << "], current offset = " << V.offset() << ", value = " << V.value();
          if (!V.data_pointer) stream << " (using indirect IO)";
          else stream << " (using direct IO, data at " << V.data_pointer << ")";
          return stream;
        }

        //! write out the contents of a direct IO image to file 
        /*! 
         * returns the name of the image - needed by display() to get the
         * name of the temporary file to supply to MRView. 
         *
         * \note this is \e not the recommended way to save an image - only use
         * this function when you absolutely need to minimise RAM usage on
         * write-out (this avoids any further buffering before write-out).
         *
         * \note this will only work for images accessed using direct IO (i.e.
         * opened as a scratch image, or using with_direct_io(), and only
         * supports output to MRtrix format images (*.mif / *.mih). There is a
         * chance that images opened in other ways may also use direct IO (e.g.
         * if the datatype & strides match, and the image is single-file), you
         * can check using the is_direct_io() method. If there is any
         * possibility that this image might use indirect IO, you should use
         * the save() function instead (and even then, it should only be used
         * for debugging purposes). */
        std::string dump_to_mrtrix_file (std::string filename, bool use_multi_threading = true) const;

        //! return a new Image using direct IO
        /*! 
         * this will preload the data into RAM if the datatype on file doesn't
         * match that on file (or if any scaling is applied to the data). The
         * optional \a with_strides argument is used to additionally enforce
         * preloading if the strides aren't compatible with those specified. 
         *
         * Example:
         * \code
         * auto image = Header::open (argument[0]).get_image().with_direct_io();
         * \endcode 
         * \note this invalidate the invoking Image - do not use the original
         * image in subsequent code.*/
        Image with_direct_io (Stride::List with_strides = Stride::List());

        //! return a new Image using direct IO
        /*! 
         * this is a convenience function, performing the same function as
         * with_direct_io(Stride::List). The difference is that the \a axis
         * argument specifies which axis should be contiguous, or if \a axis is
         * negative, that the spatial axes should be contiguous (the \c
         * SpatiallyContiguous constexpr, set to -1, is provided for clarity).
         * In other words:
         * \code 
         * auto image = Image<float>::open (filename).with_direct_io (3);
         * \endcode
         * is equivalent to:
         * \code 
         * auto header = Header::open (filename);
         * auto image = header.get_image<float>().with_direct_io (Stride::contiguous_along_axis (3, header));
         * \endcode
         * and
         * \code 
         * auto image = Image<float>::open (filename).with_direct_io (-1);
         * // or;
         * auto image = Image<float>::open (filename).with_direct_io (SpatiallyContiguous);
         * \endcode
         * is equivalent to:
         * \code 
         * auto header = Header::open (filename);
         * auto image = header.get_image<float>().with_direct_io (Stride::contiguous_along_spatial_axes (header));
         * \endcode
         */
        Image with_direct_io (int axis) {
          return with_direct_io ( axis < 0 ?
              Stride::contiguous_along_spatial_axes (*buffer) :
              Stride::contiguous_along_axis (axis, *buffer) );
        }


        //! return RAM address of current voxel
        /*! \note this will only work if image access is direct (i.e. for a
         * scratch image, with preloading, or when the data type is native and
         * without scaling. */
        ValueType* address () const { 
          assert (data_pointer != nullptr && "Image::address() can only be used when image access is via direct RAM access");
          return data_pointer ? static_cast<ValueType*>(data_pointer) + data_offset : nullptr; }

        static Image open (const std::string& image_name, bool read_write_if_existing = false) {
          return Header::open (image_name).get_image<ValueType> (read_write_if_existing);
        }
        static Image create (const std::string& image_name, const Header& template_header) {
          return Header::create (image_name, template_header).get_image<ValueType>();
        }
        static Image scratch (const Header& template_header, const std::string& label = "scratch image") {
          return Header::scratch (template_header, label).get_image<ValueType>();
        }

      protected:
        //! shared reference to header/buffer
        std::shared_ptr<Buffer> buffer;
        //! pointer to data address whether in RAM or MMap
        void* data_pointer;
        //! voxel indices
        std::vector<ssize_t> x;
        //! voxel indices
        Stride::List strides;
        //! offset to currently pointed-to voxel
        size_t data_offset;
    };







  template <typename ValueType> 
    class Image<ValueType>::Buffer : public Header
    {
      public:
        //! construct a Buffer object to access the data in the image specified
        Buffer (Header& H, bool read_write_if_existing = false);
        Buffer (Buffer&&) = default;
        Buffer& operator= (const Buffer&) = delete;
        Buffer& operator= (Buffer&&) = default;
        Buffer (const Buffer& b) : 
          Header (b), fetch_func (b.fetch_func), store_func (b.store_func) { }

      EIGEN_MAKE_ALIGNED_OPERATOR_NEW  // avoid memory alignment errors in Eigen3;

        FORCE_INLINE ValueType get_value (size_t offset) const {
          ssize_t nseg = offset / io->segment_size();
          return fetch_func (io->segment (nseg), offset - nseg*io->segment_size(), intensity_offset(), intensity_scale());
        }

        FORCE_INLINE void set_value (size_t offset, ValueType val) const {
          ssize_t nseg = offset / io->segment_size();
          store_func (val, io->segment (nseg), offset - nseg*io->segment_size(), intensity_offset(), intensity_scale());
        }

        std::unique_ptr<uint8_t[]> data_buffer;
        void* get_data_pointer ();

        FORCE_INLINE ImageIO::Base* get_io () const { return io.get(); }

      protected:
        std::function<ValueType(const void*,size_t,default_type,default_type)> fetch_func;
        std::function<void(ValueType,void*,size_t,default_type,default_type)> store_func;

        void set_fetch_store_functions () {
          __set_fetch_store_functions (fetch_func, store_func, datatype());
        }
    };













  //! \cond skip

  namespace
  {

    // lightweight struct to copy data into:
    template <typename ValueType>
      struct TmpImage {
        typedef ValueType value_type;

        const typename Image<ValueType>::Buffer& b;
        void* const data;
        std::vector<ssize_t> x;
        const Stride::List& strides;
        size_t offset;

        bool valid () const { return true; }
        const std::string name () const { return "direct IO buffer"; }
        FORCE_INLINE size_t ndim () const { return b.ndim(); }
        FORCE_INLINE ssize_t size (size_t axis) const { return b.size(axis); }
        FORCE_INLINE ssize_t stride (size_t axis) const { return strides[axis]; }

        FORCE_INLINE ssize_t index (size_t axis) const { return x[axis]; }
        FORCE_INLINE auto index (size_t axis) -> decltype (Helper::index (*this, axis)) { return { *this, axis }; }
        FORCE_INLINE void move_index (size_t axis, ssize_t increment) { offset += stride (axis) * increment; x[axis] += increment; }

        FORCE_INLINE value_type value () const { return Raw::fetch_native<ValueType> (data, offset); } 
        FORCE_INLINE auto value () -> decltype (Helper::value (*this)) { return { *this }; }
        FORCE_INLINE void set_value (ValueType val) { Raw::store_native<ValueType> (val, data, offset); }
      };

  }








  template <typename ValueType>
    Image<ValueType>::Buffer::Buffer (Header& H, bool read_write_if_existing) :
      Header (H) {
        assert (H.valid() && "IO handler must be set when creating an Image"); 
        assert ((H.is_file_backed() ? is_data_type<ValueType>::value : true) && "class types cannot be stored on file using the Image class");

        acquire_io (H);
        io->set_readwrite_if_existing (read_write_if_existing);
        io->open (*this, footprint<ValueType> (voxel_count (*this)));
        if (io->is_file_backed()) 
          set_fetch_store_functions ();
      }






  template <typename ValueType> 
    void* Image<ValueType>::Buffer::get_data_pointer () 
    {
      if (data_buffer) // already allocated via with_direct_io()
        return data_buffer.get();

      assert (io && "data pointer will only be set for valid Images");
      if (!io->is_file_backed()) // this is a scratch image
        return io->segment(0);

      // check whether we can still do direct IO
      // if so, return address where mapped
      if (io->nsegments() == 1 && datatype() == DataType::from<ValueType>() && intensity_offset() == 0.0 && intensity_scale() == 1.0)
        return io->segment(0);

      // can't do direct IO
      return nullptr;
    }



  template <typename ValueType>
    Image<ValueType> Header::get_image (bool read_write_if_existing)
    {
      if (!valid())
        throw Exception ("FIXME: don't invoke get_image() with invalid Header!");
      std::shared_ptr<typename Image<ValueType>::Buffer> buffer (new typename Image<ValueType>::Buffer (*this, read_write_if_existing));
      return { buffer };
    }






  template <typename ValueType>
    FORCE_INLINE Image<ValueType>::Image () :
      data_pointer (nullptr), 
      data_offset (0) { }

  template <typename ValueType>
    Image<ValueType>::Image (const std::shared_ptr<Image<ValueType>::Buffer>& buffer_p, const Stride::List& desired_strides) :
      buffer (buffer_p),
      data_pointer (buffer->get_data_pointer()),
      x (ndim(), 0),
      strides (desired_strides.size() ? desired_strides : Stride::get (*buffer)),
      data_offset (Stride::offset (*this))
      { 
        assert (buffer);
        assert (data_pointer || buffer->get_io());
        DEBUG ("image \"" + name() + "\" initialised with strides = " + str(strides) + ", start = " + str(data_offset) 
            + ", using " + ( is_direct_io() ? "" : "in" ) + "direct IO");
      }





  template <typename ValueType>
    Image<ValueType>::~Image () 
    {
      if (buffer.unique()) {
        // was image preloaded and read/write? If so,need to write back:
        if (buffer->get_io()) {
          if (buffer->get_io()->is_image_readwrite() && buffer->data_buffer) {
            auto data_buffer = std::move (buffer->data_buffer);
            TmpImage<ValueType> src = { *buffer, data_buffer.get(), std::vector<ssize_t> (ndim(), 0), strides, Stride::offset (*this) };
            Image<ValueType> dest (buffer);
            threaded_copy_with_progress_message ("writing back direct IO buffer for \"" + name() + "\"", src, dest); 
          }
        }
      }
    }



  template <typename ValueType>
    Image<ValueType> Image<ValueType>::with_direct_io (Stride::List with_strides)
    {
      if (buffer->data_buffer)
        throw Exception ("FIXME: don't invoke 'with_direct_io()' on images already using direct IO!");
      if (!buffer->get_io())
        throw Exception ("FIXME: don't invoke 'with_direct_io()' on non-validated images!");
      if (!buffer.unique())
        throw Exception ("FIXME: don't invoke 'with_direct_io()' on images if other copies exist!");

      bool preload = ( buffer->datatype() != DataType::from<ValueType>() ) || ( buffer->get_io()->files.size() > 1 );
      if (with_strides.size()) {
        auto new_strides = Stride::get_actual (Stride::get_nearest_match (*this, with_strides), *this);
        preload |= ( new_strides != Stride::get (*this) );
        with_strides = new_strides;
      }
      else 
        with_strides = Stride::get (*this); 

      if (!preload) 
        return std::move (*this);

      // do the preload:

      // the buffer into which to copy the data:
      const auto buffer_size = footprint<ValueType> (voxel_count (*this));
      buffer->data_buffer = std::unique_ptr<uint8_t[]> (new uint8_t [buffer_size]);

      if (buffer->get_io()->is_image_new()) {
        // no need to preload if data is zero anyway:
        memset (buffer->data_buffer.get(), 0, buffer_size);
      }
      else {
        auto src (*this);
        TmpImage<ValueType> dest = { *buffer, buffer->data_buffer.get(), std::vector<ssize_t> (ndim(), 0), with_strides, Stride::offset (with_strides, *this) };
        threaded_copy_with_progress_message ("preloading data for \"" + name() + "\"", src, dest); 
      }

      return Image (buffer, with_strides);
    }





  template <typename ValueType>
    std::string Image<ValueType>::dump_to_mrtrix_file (std::string filename, bool) const 
    {
      if (!data_pointer || ( !Path::has_suffix (filename, ".mih") && !Path::has_suffix (filename, ".mif") ))
        throw Exception ("FIXME: image not suitable for use with 'Image::dump_to_mrtrix_file()'");

      // try to dump file to mrtrix format if possible (direct IO)
      if (filename == "-")
        filename = File::create_tempfile (0, "mif");

      DEBUG ("dumping image \"" + name() + "\" to file \"" + filename + "\"...");

      File::OFStream out (filename, std::ios::out | std::ios::binary);
      out << "mrtrix image\n";
      Formats::write_mrtrix_header (*buffer, out);

      const bool single_file = Path::has_suffix (filename, ".mif");
      std::string data_filename = filename;

      int64_t offset = 0;
      out << "file: ";
      if (single_file) {
        offset = out.tellp() + int64_t(18);
        offset += ((4 - (offset % 4)) % 4);
        out << ". " << offset << "\nEND\n";
      }
      else {
        data_filename = filename.substr (0, filename.size()-4) + ".dat";
        out << Path::basename (data_filename) << "\n";
        out.close();
        out.open (data_filename, std::ios::out | std::ios::binary); 
      }

      const int64_t data_size = footprint (*buffer);
      out.seekp (offset, out.beg);
      out.write ((const char*) data_pointer, data_size);
      if (!out.good())
        throw Exception ("error writing back contents of file \"" + data_filename + "\": " + strerror(errno));
      out.close();
     
      // If data_size exceeds some threshold, ostream artificially increases the file size beyond that required at close()
      // TODO check whether this is still needed...?
      File::resize (data_filename, offset + data_size);

      return filename;
    }








  template <class ImageType>
    std::string __save_generic (ImageType& x, const std::string& filename, bool use_multi_threading) {
      auto out = Image<typename ImageType::value_type>::create (filename, x);
      if (use_multi_threading)
        threaded_copy (x, out);
      else
        copy (x, out);
      return out.name();
    }

  //! \endcond

  //! save contents of an existing image to file (for debugging only)
  template <class ImageType>
    typename std::enable_if<is_adapter_type<typename std::remove_reference<ImageType>::type>::value, std::string>::type
    save (ImageType&& x, const std::string& filename, bool use_multi_threading = true) 
    {
      return __save_generic (x, filename, use_multi_threading);
    }

  //! save contents of an existing image to file (for debugging only)
  template <class ImageType>
    typename std::enable_if<is_pure_image<typename std::remove_reference<ImageType>::type>::value, std::string>::type
    save (ImageType&& x, const std::string& filename, bool use_multi_threading = true) 
    {
      try { return x.dump_to_mrtrix_file (filename, use_multi_threading); }
      catch (...) { }
      return __save_generic (x, filename, use_multi_threading);
    }


  //! display the contents of an image in MRView (for debugging only)
  template <class ImageType>
    typename enable_if_image_type<ImageType,void>::type display (ImageType& x) {
      std::string filename = save (x, "-");
      CONSOLE ("displaying image \"" + filename + "\"");
      if (system (("bash -c \"mrview " + filename + "\"").c_str()))
        WARN (std::string("error invoking viewer: ") + strerror(errno));
    }


}

#endif


