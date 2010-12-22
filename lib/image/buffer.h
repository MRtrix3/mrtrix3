/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/05/09.

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

#ifndef __image_buffer_h__
#define __image_buffer_h__

#include "get_set.h"
#include "image/header.h"
#include "math/complex.h"
#include "dataset/buffer.h"
#include "dataset/value.h"
#include "dataset/position.h"

namespace MR
{
  namespace Image
  {

    //! \addtogroup Image
    // @{

    //! This class provides fast access to the voxel intensities of an image.
    /*! This class provides fast access to the image intensities of an image.
     * It differs from the Image::Voxel class in that the data are guaranteed
     * to be resident in memory in native format, rather than being converted
     * to the relevant format on-the-fly. This provides fast access to the
     * underlying data by removing some of the read/write overhead, but may
     * incur a penalty if the data need to be loaded into RAM (see below).
     * It also implements all the features of the DataSet abstract class.
     *
     * \section voxeldirectpreload Data access
     * The Image::Buffer class ensures the data are accessible directly in
     * native format, as specified by the template argument. If the data are
     * already stored in the requested format, and are contained within a
     * single file, then the file is simply memory-mapped, and the data are
     * immediately usable. In all other cases, an appropriate memory buffer is
     * allocated, and the data are explicitly loaded into the buffer.
     *
     * \section voxeldirectadvantages When to use Image::Buffer
     * The Image::Buffer class is advantageous when the data in the image
     * are expected to be accessed often and repeatedly, and especially if the
     * access pattern cannot be anticipated. The reasons for this are that
     * there is no function call overhead to read/write voxel values, and the
     * indexing operations required to locate the voxel data are simpler and
     * faster than for the Image::Voxel class. However, if the processing can
     * be performed in one or two passes through the data, the Image::Voxel
     * class will probably be just as fast.
     *
     * \section voxeldirectthreading Multi-threading
     * The copy constructor of the Image::Buffer class creates an instance
     * that references the same data as the original, but can be used
     * independently. It is therefore perfectly suited to use in multi-threaded
     * applications. Note that there are no checks for concurrent read/write
     * access to the same voxel data; it is up to the programmer to ensure
     * such situations are avoided, or at least handled properly.
     */
    template <typename T> class Buffer
    {
      private:
        class Shared
        {
          public:
            Shared (size_t NDIM) : data (NULL), stride (NDIM), start (0) { }
            T* data;
            DataSet::Stride::List stride;
            size_t start;
            Ptr<T,true> block;
        };

      public:
        typedef T value_type;

        //! construct an Image::Buffer object to access the data in the Image::Header \p header
        Buffer (Image::Header& header, size_t NDIM = std::numeric_limits<size_t>::max()) :
          H (header),
          ptr (new Shared (NDIM == std::numeric_limits<size_t>::max() ? header.ndim() : NDIM)),
          x (ndim()),
          stride_instance (ptr) {
          for (size_t i = 0; i < ndim(); ++i)
            ptr->stride[i] = header.stride (i);

          Handler::Base* handler = header.get_handler();
          handler->prepare();
          if (handler->nsegments() == 1 &&
              header.datatype() == DataType::from<value_type>()) {
            info ("data in \"" + header.name() + "\" already in required format - mapping as-is");
            ptr->data = reinterpret_cast<value_type*> (handler->segment (0));
          }

          DataSet::Stride::actualise (ptr->stride, H);
          ptr->start = DataSet::Stride::offset (ptr->stride, H);
          reset();

          if (!ptr->data) {
            ptr->data = DataSet::__allocate<value_type> (DataSet::voxel_count (*this));
            ptr->block = ptr->data;
            info ("data in \"" + header.name() + "\" not in required format - loading into memory...");
            Image::Voxel<value_type> vox (header);
            DataSet::copy_with_progress_message ("loading data for image \"" + header.name() + "\"...", *this, vox);
          }
        }


        //! Construct from an Image::Header object with guaranteed strides
        /*! the resulting instance is guaranteed to have the strides specified.
         * Any zero strides will be ignored. */
        Buffer (Image::Header& header, const std::vector<ssize_t>& desired_strides, size_t NDIM = std::numeric_limits<size_t>::max()) :
          H (header),
          ptr (new Shared (NDIM == std::numeric_limits<size_t>::max() ? header.ndim() : NDIM)),
          x (ndim()),
          stride_instance (ptr) {
          bool strides_match = true;
          for (size_t i = 0; i < std::min (desired_strides.size(), ndim()); ++i) {
            if (desired_strides[i]) {
              if (Math::abs (desired_strides[i]) != Math::abs (header.stride (i))) {
                strides_match = false;
                break;
              }
            }
          }

          if (strides_match) {
            for (size_t i = 0; i < ndim(); ++i)
              ptr->stride[i] = header.stride (i);
          }
          else {
            for (size_t i = 0; i < ndim(); ++i)
              ptr->stride[i] = 0;
            for (size_t i = 0; i < std::min (desired_strides.size(), ndim()); ++i)
              ptr->stride[i] = desired_strides[i];
            DataSet::Stride::sanitise (ptr->stride);
          }

          Handler::Base* handler = header.get_handler();
          handler->prepare();
          if (handler->nsegments() == 1 &&
              header.datatype() == DataType::from<value_type>()
              && strides_match) {
            info ("data in \"" + header.name() + "\" already in native format - mapping as-is");
            ptr->data = reinterpret_cast<value_type*> (handler->segment (0));
          }

          DataSet::Stride::actualise (ptr->stride, H);
          ptr->start = DataSet::Stride::offset (ptr->stride, H);
          reset();

          if (!ptr->data) {
            ptr->data = DataSet::__allocate<value_type> (DataSet::voxel_count (*this));
            ptr->block = ptr->data;
            info ("data in \"" + header.name() + "\" not in native format - loading into memory...");
            Image::Voxel<value_type> vox (header);
            DataSet::copy_with_progress_message ("loading data for image \"" + header.name() + "\"...", *this, vox);
          }
        }


        //! Copy constructor
        /*! \note the new instance will refer to the data from the original
         * Buffer, but will not try to delete the data when the destructor is
         * called. */
        Buffer (const Buffer& buf) :
          H (buf.H),
          ptr (buf.ptr),
          offset (buf.offset),
          x (buf.x) { }

        const Header& header () const {
          return (H);
        }
        const Math::Matrix<float>& transform () const {
          return (H.transform());
        }


        ssize_t stride (size_t axis) const {
          return (ptr->stride[axis]);
        }
        size_t  ndim () const {
          return (ptr->stride.size());
        }
        ssize_t dim (size_t axis) const {
          return (H.dim (axis));
        }
        float   vox (size_t axis) const {
          return (H.vox (axis));
        }
        const std::string& name () const {
          return (H.name());
        }

        //! reset all coordinates to zero.
        void    reset () {
          std::fill (x.begin(), x.end(), 0);
          offset = ptr->start;
        }

        DataSet::Position<Buffer<T> > operator[] (size_t axis) {
          return (DataSet::Position<Buffer<T> > (*this, axis));
        }
        DataSet::Value<Buffer<T> > value () {
          return (DataSet::Value<Buffer<T> > (*this));
        }

        friend std::ostream& operator<< (std::ostream& stream, const Buffer& V) {
          stream << "position for image \"" << V.name() << "\" = [ ";
          for (size_t n = 0; n < V.ndim(); ++n) stream << const_cast<Buffer&> (V) [n] << " ";
          stream << "]\n  current offset = " << V.offset;
          return (stream);
        }

      private:
        const Header&   H; //!< reference to the corresponding Image::Header
        Shared* ptr;
        size_t   offset; //!< the offset in memory to the current voxel
        std::vector<ssize_t> x;

        Ptr<Shared> stride_instance;

        template <class A> size_t get_offset (const A& pos, size_t n = 0) const {
          return (n < ndim() ? stride (n) * pos[n] + get_offset (pos, n+1) : ptr->start);
        }

        ssize_t get_pos (size_t axis) const {
          return (x[axis]);
        }
        void    set_pos (size_t axis, ssize_t position) {
          offset += stride (axis) * (position - x[axis]);
          x[axis] = position;
        }
        void    move_pos (size_t axis, ssize_t increment) {
          offset += stride (axis) * increment;
          x[axis] += increment;
        }

        value_type   get_value () const {
          return (DataSet::__get<value_type> (ptr->data, offset));
        }
        void         set_value (value_type val) {
          DataSet::__set<value_type> (ptr->data, offset, val);
        }

        friend class DataSet::Position<Buffer<T> >;
        friend class DataSet::Value<Buffer<T> >;
    };


    //! @}

  }
}

#endif


