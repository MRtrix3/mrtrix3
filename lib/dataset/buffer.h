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

namespace MR {
  namespace DataSet {

    //! \cond skip
    namespace {
      template <typename X> inline void __allocate (X*& data, size_t count) { data = new X [count]; }
      template <> inline void __allocate<bool> (bool*& data, size_t count) { data = (bool*) (new uint8_t [(count+7)/8]); }

      template <typename X> inline X __get (const X*& data, ssize_t offset) { return (data[offset]); }
      template <typename X> inline void __set (X*& data, ssize_t offset, X val) { data[offset] = val; }

      template <> inline bool __get<bool> (const bool*& data, ssize_t offset) 
      { return (((((uint8_t*) data)[offset/8] << offset%8) & BITMASK) ? true : false); } 
      template <> inline void __set<bool> (bool*& data, ssize_t offset, bool val) { 
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
          transform_matrix (transform_mat) {
            for (size_t n = 0; n < NDIM; ++n) {
              N[n] = dimensions[n];
              V[n] = voxel_sizes ? voxel_sizes[n] : 1.0;
            }
            setup();
          }

        template <class Template> Buffer (const Template& D, const std::string& id = "unnamed") :
          descriptor (id),
          transform_matrix (D.transform()) { 
            assert (D.ndim() >= NDIM);
            for (size_t n = 0; n < NDIM; ++n) {
              N[n] = D.dim(n);
              V[n] = D.vox(n);
            }
            setup();
          }

        ~Buffer () { delete [] data; }

        const std::string& name () const { return (descriptor); }
        size_t  ndim () const { return (NDIM); }
        int     dim (size_t axis) const { return (N[axis]); }

        float   vox (size_t axis) const { return (V[axis]); }

        const Math::Matrix<float>& transform () const { return (transform_matrix); }

        void    reset () { memset (x, 0, NDIM*sizeof(ssize_t)); }

        ssize_t pos (size_t axis) const { return (x[axis]); }
        void    pos (size_t axis, ssize_t position) { x[axis] = position; }
        void    move (size_t axis, ssize_t increment) { x[axis] += increment; }

        value_type   value () const { return (__get<value_type>(data, offset())); }
        void         value (value_type val) { __set<value_type>(data, offset(), val); }

      private:
        value_type* data;
        size_t N [NDIM];
        ssize_t x [NDIM];
        float  V [NDIM];
        std::string descriptor;

        Math::Matrix<float> transform_matrix;

        void setup () {
          __allocate<value_type> (data, voxel_count (*this));
          reset();
          if (transform_matrix.rows() == 4 && transform_matrix.columns() == 4) return;
          transform_matrix.allocate(4,4);
          transform_matrix.identity();
          transform_matrix(0,3) = N[0]/2.0;
          transform_matrix(1,3) = N[1]/2.0;
          transform_matrix(2,3) = N[2]/2.0;
        }

        ssize_t offset () const { return (x[0] + N[0] * offset (1)); }
        ssize_t offset (size_t a) const { return (a < NDIM-1 ? x[a] : x[a] + N[a] * offset(a+1)); }
    };

    //! @}
  }
}

#endif

