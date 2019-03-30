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

          Streamline (size_t size, const Eigen::Vector3f& fill) :
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




      template <typename ValueType>
      float Streamline<ValueType>::calc_length() const
      {
        switch (Streamline<ValueType>::size()) {
          case 0: return NaN;
          case 1: return 0.0;
          default: break;
        }
        default_type length = 0.0;
        for (size_t i = 1; i != Streamline<ValueType>::size(); ++i)
          length += ((*this)[i]-(*this)[i-1]).norm();
        return length;
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

