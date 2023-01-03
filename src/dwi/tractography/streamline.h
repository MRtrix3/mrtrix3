/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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



      // Base class for storing an index alongside either streamline vertex or track scalar data
      //
      class DataIndex
      { NOMEMALIGN
        public:
          static constexpr size_t invalid = std::numeric_limits<size_t>::max();
          DataIndex () : index (invalid) { }
          DataIndex (const size_t i) : index (i) { }
          DataIndex (const DataIndex& i) : index (i.index) { }
          DataIndex (DataIndex&& i) : index (i.index) { i.index = invalid; }
          DataIndex& operator= (const DataIndex& i) { index = i.index; return *this; }
          DataIndex& operator= (DataIndex&& i) { index = i.index; i.index = invalid; return *this; }
          void set_index (const size_t i) { index = i; }
          size_t get_index() const { return index; }
          void clear() { index = invalid; }
          bool operator< (const DataIndex& i) const { return index < i.index; }
        private:
          size_t index;
      };




      // A class for track scalars
      template <typename ValueType = float>
      class TrackScalar : public vector<ValueType>, public DataIndex
      { MEMALIGN(TrackScalar)
        public:
          using value_type = ValueType;
          using vector<ValueType>::vector;
          TrackScalar () = default;
          TrackScalar (const TrackScalar&) = default;
          TrackScalar (TrackScalar&& that) :
              vector<value_type> (std::move (that)),
              DataIndex (std::move (that)) { }
          TrackScalar& operator= (const TrackScalar& that) = default;
          void clear() { vector<ValueType>::clear(); DataIndex::clear(); }
      };




      template <typename ValueType = float>
        class Streamline : public vector<Eigen::Matrix<ValueType,3,1>>, public DataIndex
      { MEMALIGN(Streamline<ValueType>)
        public:
          using point_type = Eigen::Matrix<ValueType,3,1>;
          using value_type = ValueType;

          Streamline () : weight (1.0f) { }

          Streamline (size_t size) :
            vector<point_type> (size),
            weight (value_type (1.0)) { }

          Streamline (size_t size, const point_type& fill) :
            vector<point_type> (size, fill),
            weight (value_type (1.0)) { }

          Streamline (const Streamline&) = default;
          Streamline& operator= (const Streamline& that) = default;

          Streamline (Streamline&& that) :
            vector<point_type> (std::move (that)),
            DataIndex (std::move (that)),
            weight (that.weight) {
              that.weight = 1.0f;
            }

          Streamline (const vector<point_type>& tck) :
            vector<point_type> (tck),
            DataIndex (),
            weight (1.0) { }

          Streamline& operator= (Streamline&& that)
          {
            vector<point_type>::operator= (std::move (that));
            DataIndex::operator= (std::move (that));
            weight = that.weight; that.weight = 0.0f;
            return *this;
          }


          void clear()
          {
            vector<point_type>::clear();
            DataIndex::clear();
            weight = 1.0;
          }

          float calc_length() const;
          float calc_length (const float step_size) const;

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

