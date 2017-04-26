/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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
        class Streamline : public vector<Eigen::Matrix<ValueType,3,1>>
      { MEMALIGN(Streamline<ValueType>)
        public:
          using point_type = Eigen::Matrix<ValueType,3,1>;
          using value_type = ValueType;

          Streamline () : index (-1), weight (1.0f) { }

          Streamline (size_t size) : 
            vector<point_type> (size), 
            index (-1),
            weight (value_type (1.0)) { }

          Streamline (size_t size, const Eigen::Vector3f& fill) :
            vector<point_type> (size, fill),
            index (-1),
            weight (value_type (1.0)) { }

          Streamline (const Streamline&) = default;
          Streamline& operator= (const Streamline& that) = default;

          Streamline (Streamline&& that) :
            vector<point_type> (std::move (that)),
            index (that.index),
            weight (that.weight) {
              that.index = -1;
              that.weight = 1.0f;
            }

          Streamline (const vector<point_type>& tck) :
            vector<point_type> (tck),
            index (-1),
            weight (1.0) { }

          Streamline& operator= (Streamline&& that)
          {
            vector<point_type>::operator= (std::move (that));
            index = that.index; that.index = -1;
            weight = that.weight; that.weight = 1.0f;
            return *this;
          }


          void clear()
          {
            vector<point_type>::clear();
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

