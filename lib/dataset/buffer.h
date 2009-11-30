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

#define BITMASK 0x01U << 7

namespace MR {
  namespace DataSet {

    //! \cond skip
    namespace {
      template <typename X> inline X* __allocate (size_t count) { return (new X [count]); }
      template <> inline bool* __allocate<bool> (size_t count) { return ((bool*) (new uint8_t [(count+7)/8])); }

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

        Buffer (const size_t* dimensions, 
            const std::string& id = "unnamed",
            const float* voxel_sizes = NULL, 
            const Math::MatrixView<float> transform_mat = Math::MatrixView<float>()) :
          descriptor (id),
          transform_matrix (transform_mat),
          offset (0),
          start (0) {
            for (size_t n = 0; n < NDIM; ++n) {
              N[n] = dimensions[n];
              V[n] = voxel_sizes ? voxel_sizes[n] : 1.0;
              stride[n] = n ? N[n-1] * stride[n-1] : 1;
              axes_layout[n] = n;
            }
            setup();
          }

        template <class Template> Buffer (const Template& D, const std::string& id = "unnamed") :
          descriptor (id),
          transform_matrix (D.transform()) {
            assert (D.ndim() >= NDIM);
            offset = start = 0;
            for (size_t n = 0; n < NDIM; ++n) {
              N[n] = D.dim(n);
              V[n] = D.vox(n);
              stride[n] = n ? N[n-1] * stride[n-1] : 1;
              axes_layout[n] = n;
            }
            setup();
          }

        ~Buffer () { }

        const std::string& name () const { return (descriptor); }
        size_t  ndim () const { return (NDIM); }
        int     dim (size_t axis) const { return (N[axis]); }
        const size_t* layout () const { return (axes_layout); }

        float   vox (size_t axis) const { return (V[axis]); }

        const Math::Matrix<float>& transform () const { return (transform_matrix); }

        void    reset () { memset (x, 0, NDIM*sizeof(ssize_t)); offset = start; }

        ssize_t pos (size_t axis) const { return (x[axis]); }
        void    pos (size_t axis, ssize_t position) { 
          offset += stride[axis] * (position - x[axis]); x[axis] = position; }
        void    move (size_t axis, ssize_t increment) { offset += stride[axis] * increment; x[axis] += increment; }

        value_type   value () const { return (__get<value_type>(data.get(), offset)); }
        void         value (value_type val) { __set<value_type>(data.get(), offset, val); }

      private:
        typename Array<value_type>::RefPtr data;
        size_t offset, start;
        size_t N [NDIM];
        ssize_t x [NDIM], stride[N];
        float  V [NDIM];
        size_t axes_layout[NDIM];
        std::string descriptor;

        Math::Matrix<float> transform_matrix;

        void setup () {
          data = __allocate<value_type> (voxel_count (*this));
          reset();
          if (transform_matrix.rows() == 4 && transform_matrix.columns() == 4) return;
          transform_matrix.allocate(4,4);
          transform_matrix.identity();
          transform_matrix(0,3) = N[0]/2.0;
          transform_matrix(1,3) = N[1]/2.0;
          transform_matrix(2,3) = N[2]/2.0;
        }
    };

    //! @}
  }
}

#endif

