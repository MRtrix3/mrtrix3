/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

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

#ifndef __dwi_tractography_streamline_h__
#define __dwi_tractography_streamline_h__


#include <vector>

#include "types.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {


      template <typename ValueType = float>
        class Streamline : public std::vector<Eigen::Matrix<ValueType,3,1>>
      {
        public:
          typedef Eigen::Matrix<ValueType,3,1> point_type;
          typedef ValueType value_type;

          Streamline () : index (-1), weight (1.0f) { }

          Streamline (size_t size) : 
            std::vector<point_type> (size), 
            index (-1),
            weight (value_type (1.0)) { }

          Streamline (size_t size, const Eigen::Vector3f& fill) :
            std::vector<point_type> (size, fill),
            index (-1),
            weight (value_type (1.0)) { }

          Streamline (const Streamline&) = default;
          Streamline& operator= (const Streamline& that) = default;

          Streamline (Streamline&& that) :
            std::vector<point_type> (std::move (that)),
            index (that.index),
            weight (that.weight) {
              that.index = -1;
              that.weight = 1.0f;
            }

          Streamline (const std::vector<point_type>& tck) :
            std::vector<point_type> (tck),
            index (-1),
            weight (1.0) { }

          Streamline& operator= (Streamline&& that)
          {
            std::vector<point_type>::operator= (std::move (that));
            index = that.index; that.index = -1;
            weight = that.weight; that.weight = 1.0f;
            return *this;
          }


          void clear()
          {
            std::vector<point_type>::clear();
            index = -1;
            weight = 1.0;
          }

          float calc_length() const;
          float calc_length (const float step_size) const;

          size_t index;
          float weight;
      };




      template <typename ValueType>
      float Streamline<ValueType>::calc_length() const
      {
        switch (Streamline<ValueType>::size()) {
          case 0: return NaN;
          case 1: return 0.0;
          case 2: return ((*this)[1]-(*this)[0]).norm();
          case 3: return ((*this)[1]-(*this)[0]).norm() + ((*this)[2]-(*this)[1]).norm();
          default: break;
        }
        const size_t midpoint = Streamline<ValueType>::size() / 2;
        const float step_size = ((*this)[midpoint-1]-(*this)[midpoint]).norm();
        return calc_length (step_size);
      }

      template <typename ValueType>
      float Streamline<ValueType>::calc_length (const float step_size) const
      {
        switch (Streamline<ValueType>::size()) {
          case 0: return NaN;
          case 1: return 0.0;
          case 2: return ((*this)[1]-(*this)[0]).norm();
          case 3: return ((*this)[1]-(*this)[0]).norm() + ((*this)[2], (*this)[1]).norm();
          default: break;
        }
        const size_t size = Streamline<ValueType>::size();
        return step_size*(size-3) + ((*this)[1]-(*this)[0]).norm() + ((*this)[size-1]-(*this)[size-2]).norm();
      }



    }
  }
}


#endif

