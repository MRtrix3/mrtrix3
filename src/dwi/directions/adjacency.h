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

#ifndef __dwi_directions_adjacency_h__
#define __dwi_directions_adjacency_h__


#include "types.h"
#include "math/sphere.h"

#include "dwi/directions/directions.h"



namespace MR {
  namespace DWI {
    namespace Directions {


      class Adjacency : public vector<vector<index_type>> {

        public:

          template <class MatrixType>
          Adjacency (const Eigen::MatrixBase<MatrixType>& dirs)
          {
            initialise (Math::Sphere::to_cartesian (dirs));
          }

          // Are the directions corresponding to these two indices adjacent to one another?
          bool operator() (const index_type one, const index_type two) const {
            assert (one < size());
            assert (two < size());
            return (std::find ((*this)[one].begin(), (*this)[one].end(), two) != (*this)[one].end());
          }

          index_type distance (const index_type one, const index_type two) const;

        private:
          void initialise (const Eigen::Matrix<default_type, Eigen::Dynamic, 3>& dirs); // Expects prior conversion to cartesian

      };





      class CartesianWithAdjacency : public Eigen::Matrix<default_type, Eigen::Dynamic, 3>
      {
        public:
          using BaseType = Eigen::Matrix<default_type, Eigen::Dynamic, 3>;
          template <class MatrixType>
          CartesianWithAdjacency (const Eigen::MatrixBase<MatrixType>& dirs) :
              BaseType (Math::Sphere::to_cartesian (dirs)),
              adjacency (*this) {}
          size_t size() const { return rows(); }
          Eigen::Matrix<default_type, Eigen::Dynamic, 3>::ConstRowXpr operator[] (const size_t i) const { assert (i < rows()); return row(i); }
          const Adjacency adjacency;
      };



    }
  }
}

#endif

