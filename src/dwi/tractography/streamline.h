/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_streamline_h__
#define __dwi_tractography_streamline_h__


#include <limits>

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

          Streamline (size_t size, const point_type& fill) :
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

          size_t index;
          float weight;
      };



      template <typename PointType>
      typename PointType::Scalar length (const vector<PointType>& tck)
      {
        if (tck.empty())
          return std::numeric_limits<typename PointType::Scalar>::quiet_NaN();
        typename PointType::Scalar value = typename PointType::Scalar(0);
        for (size_t i = 1; i != tck.size(); ++i)
          value += (tck[i] - tck[i-1]).norm();
        return value;
      }



    }
  }
}


#endif

