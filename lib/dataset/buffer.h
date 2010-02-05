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

      template <typename X> inline X __get (const X* const data, ssize_t offset) { return (data[offset]); }
      template <typename X> inline void __set (X* data, ssize_t offset, X val) { data[offset] = val; }

      template <> inline bool __get<bool> (const bool* const data, ssize_t offset) 
      { return ((((uint8_t*) data)[offset/8]) & (BITMASK >> offset%8)); } 
      template <> inline void __set<bool> (bool* data, ssize_t offset, bool val) { 
        if (val) ((uint8_t*) data)[offset/8] |= (BITMASK >> offset%8); 
        else ((uint8_t*) data)[offset/8] &= ~(BITMASK >> offset%8); 
      }
    }
    //! \endcond

    //! \addtogroup DataSet
    // @{

    template <typename T = float, size_t NDIM = 3> class Buffer {
      public:
        typedef T value_type;

        template <class Set> Buffer (const Set& D, const std::string& id = "unnamed") :
          offset (0),
          start (0),
          descriptor (id),
          transform_matrix (D.transform()) {
            assert (D.ndim() >= NDIM);
            for (size_t n = 0; n < NDIM; ++n) {
              N[n] = D.dim(n);
              V[n] = D.vox(n);
              skip[n] = D.stride(n);
            }
            setup();
          }

        ~Buffer () { }

        const std::string& name () const { return (descriptor); }
        size_t  ndim () const { return (NDIM); }
        int     dim (size_t axis) const { return (N[axis]); }
        float   vox (size_t axis) const { return (V[axis]); }
        ssize_t stride (size_t axis) const { return (skip[axis]); }

        const Math::Matrix<float>& transform () const { return (transform_matrix); }

        Position<Buffer<T,NDIM> > operator[] (size_t axis) { return (Position<Buffer<T,NDIM> > (*this, axis)); }
        Value<Buffer<T,NDIM> > value () { return (Value<Buffer<T,NDIM> > (*this)); }

        void    reset () { memset (x, 0, NDIM*sizeof(ssize_t)); offset = start; }
        void    clear () { memset (data, 0, __footprint<value_type> (voxel_count (*this))); }

      private:
        typename Array<value_type>::RefPtr data;
        size_t offset, start;
        size_t N [NDIM];
        ssize_t x [NDIM], skip[NDIM];
        float  V [NDIM];
        size_t axes_layout[NDIM];
        std::string descriptor;

        Math::Matrix<float> transform_matrix;

        ssize_t& stride (size_t axis) { return (skip[axis]); }

        void setup () 
        {
          data = __allocate<value_type> (voxel_count (*this));
          Stride::actualise (*this);
          start = Stride::offset (*this);
          reset();
          if (transform_matrix.rows() == 4 && transform_matrix.columns() == 4) return;
          transform_matrix.allocate(4,4);
          transform_matrix.identity();
          transform_matrix(0,3) = N[0]/2.0;
          transform_matrix(1,3) = N[1]/2.0;
          transform_matrix(2,3) = N[2]/2.0;
        }

        ssize_t get_pos (size_t axis) const { return (x[axis]); }
        void    set_pos (size_t axis, ssize_t position) { 
          offset += skip[axis] * (position - x[axis]); x[axis] = position; }
        void    move_pos (size_t axis, ssize_t increment) { offset += skip[axis] * increment; x[axis] += increment; }

        value_type   get_value () const { return (__get<value_type>(data.get(), offset)); }
        void         set_value (value_type val) { __set<value_type>(data.get(), offset, val); }

        friend void Stride::actualise<Buffer<T,NDIM> > (Buffer<T,NDIM>& set);
        friend class Position<Buffer<T,NDIM> >;
        friend class Value<Buffer<T,NDIM> >;
    };

    //! @}
  }
}

#endif

