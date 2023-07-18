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

#ifndef __math_sphere_set_adjacency_h__
#define __math_sphere_set_adjacency_h__

#include "types.h"
#include "math/sphere/set/set.h"
#include "misc/bitset.h"

namespace MR {
  namespace Math {
    namespace Sphere {
      namespace Set {



      class Adjacency : public vector<vector<index_type>> {

        public:

          template <class MatrixType>
          Adjacency (const Eigen::MatrixBase<MatrixType>& dirs)
          {
            initialise (to_cartesian (dirs));
          }

          Adjacency (const Adjacency& that) = default;

          Adjacency() { }

          // Are the directions corresponding to these two indices adjacent to one another?
          bool operator() (const index_type one, const index_type two) const
          {
            assert (one < size());
            assert (two < size());
            return (std::find ((*this)[one].begin(), (*this)[one].end(), two) != (*this)[one].end());
          }
          // Is this direction adjacent to a mask?
          bool operator() (const BitSet& mask, const index_type index) const
          {
            assert (mask.size() == size());
            assert (index < size());
            for (const auto i : (*this)[index]) {
              if (mask[i])
                return true;
            }
            return false;
          }

          index_type distance (const index_type one, const index_type two) const;

        private:
          void initialise (const cartesian_type& dirs); // Expects prior conversion to cartesian

      };





      class CartesianWithAdjacency : public cartesian_type
      {
        public:
          using BaseType = Eigen::Matrix<default_type, Eigen::Dynamic, 3>;
          template <class MatrixType>
          CartesianWithAdjacency (const Eigen::MatrixBase<MatrixType>& dirs) :
              BaseType (to_cartesian (dirs)),
              adj (*this) {}
          CartesianWithAdjacency() { }
          size_t size() const { return rows(); }
          BaseType::ConstRowXpr operator[] (const index_type i) const { assert (i < rows()); return row(i); }
          const vector<index_type>& adjacency (const index_type i) const { assert (i < rows()); return adj[i]; }
          bool adjacent (const BitSet& mask, const index_type index) const { return adj (mask, index); }
        private:
          Adjacency adj;
      };



      }
    }
  }
}

#endif

