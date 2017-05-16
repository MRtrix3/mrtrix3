/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __connectome_mat2vec_h__
#define __connectome_mat2vec_h__


#include <stdint.h>

#include "types.h"

#include "connectome/connectome.h"




namespace MR {
  namespace Connectome {



    class Mat2Vec
    { NOMEMALIGN

      public:
        Mat2Vec (const node_t i) : dim (i) { }

        uint64_t operator() (const node_t i, const node_t j) const
        {
          assert (i < dim);
          assert (j < dim);
          const uint64_t i64 (i);
          const uint64_t j64 (j);
          if (i < j)
            return j64 + (uint64_t(dim) * i64) - ((i64 * (i64+1)) / 2);
          else
            return i64 + (uint64_t(dim) * j64) - ((j64 * (j64+1)) / 2);
        }

        std::pair<node_t, node_t> operator() (const uint64_t i) const
        {
          static const uint64_t temp = 2*dim+1;
          static const uint64_t temp_sq = temp * temp;
          const uint64_t row = std::floor ((temp - std::sqrt(temp_sq - (8*i))) / 2);
          const uint64_t col = i - (uint64_t(dim)*row) + ((row * (row+1))/2);
          assert (row < dim);
          assert (col < dim);
          return std::make_pair (node_t(row), node_t(col));
        }

        node_t mat_size() const { return dim; }
        uint64_t vec_size() const { return (uint64_t(dim) * (uint64_t(dim)+1) / 2); }

        // Complete Matrix->Vector and Vector->Matrix conversion
        template <class MatType, class VecType>
        VecType& M2V (const MatType&, VecType&) const;
        template <class VecType, class MatType>
        MatType& V2M (const VecType&, MatType&) const;

        // Convenience functions to avoid having to pre-define the output class
        template <class MatType>
        vector_type M2V (const MatType&) const;
        template <class VecType>
        matrix_type V2M (const VecType&) const;


      private:
        const node_t dim;

    };



    template <class MatType, class VecType>
    VecType& Mat2Vec::M2V (const MatType& m, VecType& v) const
    {
      assert (m.rows() == m.cols());
      assert (m.rows() == dim);
      v.resize (vec_size());
      for (size_t index = 0; index != vec_size(); ++index) {
        const std::pair<node_t, node_t> row_col = (*this) (index);
        v[index] = m (row_col.first, row_col.second);
      }
      return v;
    }

    template <class VecType, class MatType>
    MatType& Mat2Vec::V2M (const VecType& v, MatType& m) const
    {
      assert (size_t (v.size()) == vec_size());
      m.resize (dim, dim);
      for (node_t row = 0; row != dim; ++row) {
        for (node_t col = 0; col != dim; ++col)
          m (row, col) = v[(*this) (row, col)];
      }
      return m;
    }

    template <class MatType>
    vector_type Mat2Vec::M2V (const MatType& m) const
    {
      vector_type v;
      M2V (m, v);
      return v;
    }

    template <class VecType>
    matrix_type Mat2Vec::V2M (const VecType& v) const
    {
      matrix_type m;
      V2M (v, m);
      return m;
    }



  }
}


#endif

