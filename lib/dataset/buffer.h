/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/10/09.

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

#ifndef __dataset_buffer_h__
#define __dataset_buffer_h__

#include "ptr.h"
#include "dataset/value.h"
#include "dataset/position.h"
#include "dataset/stride.h"

#define BITMASK 0x01U << 7

namespace MR {
  namespace DataSet {

    //! \cond skip
    namespace {
      template <typename X> inline X* __allocate (size_t count) { return (new X [count]); }
      template <> inline bool* __allocate<bool> (size_t count) { return ((bool*) (new uint8_t [(count+7)/8])); }

      template <typename X> inline size_t __footprint (size_t count) { return (count*sizeof(X)); }
      template <> inline size_t __footprint<bool> (size_t count) { return ((count+7)/8); }

      template <typename X> inline X __get (const X* const data, size_t offset) { return (data[offset]); }
      template <typename X> inline void __set (X* data, size_t offset, X val) { data[offset] = val; }

      template <> inline bool __get<bool> (const bool* const data, size_t offset) 
      { return ((((uint8_t*) data)[offset/8]) & (BITMASK >> offset%8)); } 
      template <> inline void __set<bool> (bool* data, size_t offset, bool val) { 
        if (val) ((uint8_t*) data)[offset/8] |= (BITMASK >> offset%8); 
        else ((uint8_t*) data)[offset/8] &= ~(BITMASK >> offset%8); 
      }
    }
    //! \endcond

    //! \addtogroup DataSet
    // @{


    //! A class to hold data in memory with fast data access
    /*! The Buffer class is a thin wrapper around a memory region, and provides
     * a DataSet interface to image data in that memory buffer. 
     *
     * \section simpleuse Use as a image buffer
     * The simplest way to use the image buffer is to provide an existing
     * DataSet object to the constructor, which will create a Buffer with the
     * same dimensions, voxel size, etc. For more fine-grained usage, create a
     * Buffer::Prototype, set the desired properties, and pass that to the
     * constructor for the Buffer.
     *
     * \section threadsafety Thread-safety
     * The Buffer class copy constructor creates a copy that references the
     * data from the original Buffer. Multiple copies can therefore be used
     * concurrently to access the data from multiple threads. It is also
     * compatible with the Thread::Array API.
     *
     * Note that concurrent access to the same voxel is \b not thread-safe, and
     * applications will need to coordinate any read/write access to the voxel
     * data.
     *
     */
    template <typename T = float, size_t NDIM = 3> 
      class Buffer 
      {
        public:

          typedef T value_type;

          class Prototype 
          {
            public:
              Prototype () : 
                data (NULL) { 
                  for (size_t n = 0; n < NDIM; ++n) {
                    dim_[n] = 0;
                    vox_[n] = 1.0;
                    stride_[n] = n+1;
                  }
                }

              value_type* data;

              size_t ndim () const { return (NDIM); }

              size_t dim (size_t axis) const { return (dim_[axis]); }
              size_t& dim (size_t axis) { return (dim_[axis]); }

              float vox (size_t axis) const { return (vox_[axis]); }
              float& vox (size_t axis) { return (vox_[axis]); }

              ssize_t stride (size_t axis) const { return (stride_[axis]); }
              ssize_t& stride (size_t axis) { return (stride_[axis]); }

              const Math::Matrix<float>& transform () const { return (transform_); }
              Math::Matrix<float>& transform () { return (transform_); }

              const std::string& name () const { return (name_); }
              std::string& name () { return (name_); }

            private:
              size_t start_;
              ssize_t stride_ [NDIM];
              size_t dim_ [NDIM];
              float  vox_ [NDIM];
              Math::Matrix<float> transform_;
              typename Array<value_type>::RefPtr block_;
              std::string name_;

              Prototype (const Prototype& prot) :
                data (prot.data),
                transform_ (prot.transform_),
                name_ (prot.name_) {
                  memcpy (stride_, prot.stride_, sizeof (stride_));
                  memcpy (dim_, prot.dim_, sizeof (dim_));
                  memcpy (vox_, prot.vox_, sizeof (vox_));
                  init();
                }

              template <class Set> Prototype (const Set& D, const std::string& id) : 
                data (NULL),
                transform_ (D.transform()),
                name_ (id) {
                  assert (D.ndim() >= NDIM);
                  for (size_t n = 0; n < NDIM; ++n) {
                    dim_[n] = D.dim(n);
                    vox_[n] = D.vox(n);
                    stride_[n] = D.stride(n);
                  }
                  init();
                }

              void init () 
              {
                Stride::actualise (*this);
                start_ = Stride::offset (*this);

                if (transform_.rows() != 4 || transform_.columns() != 4)
                  Transform::set_default (transform_, *this);

                if (!data) {
                  block_ = __allocate<value_type> (voxel_count (*this));
                  data = block_.get();
                }
              }

              friend class Buffer<T,NDIM>;
          };



          //! Construct by using \a D as prototype
          template <class Set> Buffer (const Set& D, const std::string& id = "unnamed") :
            instance (new Prototype (D, id)) {
              ptr = instance.get();
              reset();
            }

          //! Construct by supplying a fully-formed prototype
          /*! \note if the \a data member of the prototype is set to NULL (the
           * default), a new data block will be allocated to house the data.
           * Otherwise the memory region pointed to by \a data will be used as
           * the data storage. */
          Buffer (const Prototype& prot) :
            instance (new Prototype (prot)) {
              ptr = instance.get();
              reset();
            }

          //! Copy constructor
          /*! \note the new instance will refer to the data from the original
           * Buffer, but will not try to delete the data when the destructor is
           * called. */
          Buffer (const Buffer& buf) :
            ptr (buf.ptr), 
            offset (buf.offset) {
              memcpy (x, buf.x, sizeof (x));
            }

          const std::string& name () const { return (ptr->name()); }
          size_t  ndim () const { return (NDIM); }
          int     dim (size_t axis) const { return (ptr->dim(axis)); }
          float   vox (size_t axis) const { return (ptr->vox(axis)); }
          ssize_t stride (size_t axis) const { return (ptr->stride(axis)); }

          const Math::Matrix<float>& transform () const { return (ptr->transform()); }

          Position<Buffer<T,NDIM> > operator[] (size_t axis) { return (Position<Buffer<T,NDIM> > (*this, axis)); }
          Value<Buffer<T,NDIM> > value () { return (Value<Buffer<T,NDIM> > (*this)); }

          void    reset () { memset (x, 0, NDIM*sizeof(ssize_t)); offset = ptr->start_; }

          //! set all voxel values to zero
          void    clear () { memset (ptr->data, 0, __footprint<value_type> (voxel_count (*this))); }

          //! return the value at \a pos in a thread-safe manner
          template <class A> value_type value_at (const A& pos) const 
          { 
            return (__get<value_type> (ptr->data, get_offset (pos))); 
          }


        private:
          Prototype* ptr;
          size_t offset;
          ssize_t x [NDIM];

          Ptr<Prototype> instance;

          template <class A> size_t get_offset (const A& pos, size_t n = 0) const 
          { return (n < NDIM ? stride(n) * pos[n] + get_offset (pos, n+1) : ptr->start_); }

          ssize_t get_pos (size_t axis) const { return (x[axis]); }
          void    set_pos (size_t axis, ssize_t position) { 
            offset += stride(axis) * (position - x[axis]); x[axis] = position; }
          void    move_pos (size_t axis, ssize_t increment) { offset += stride(axis) * increment; x[axis] += increment; }

          value_type   get_value () const { return (__get<value_type>(ptr->data, offset)); }
          void         set_value (value_type val) { __set<value_type>(ptr->data, offset, val); }

          friend class Position<Buffer<T,NDIM> >;
          friend class Value<Buffer<T,NDIM> >;
      };

    //! @}
  }
}

#endif

