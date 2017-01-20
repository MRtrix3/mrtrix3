/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */



#ifndef __connectome_mat2vec_h__
#define __connectome_mat2vec_h__


#include <stdint.h>
#include <vector>

#include <Eigen/Dense>

#include "types.h"

#include "connectome/connectome.h"



namespace MR {
  namespace Connectome {




    class Mat2Vec
    { MEMALIGN (Mat2Vec)

      public:
        Mat2Vec (const node_t);

        Mat2Vec& operator= (Mat2Vec&&);

        size_t operator() (const node_t i, const node_t j) const
        {
          assert (i < dim);
          assert (j < dim);
          return lookup[i][j];
        }
        std::pair<node_t, node_t> operator() (const size_t i) const
        {
          assert (i < inv_lookup.size());
          return inv_lookup[i];
        }
        node_t mat_size() const { return dim; }
        size_t vec_size() const { return inv_lookup.size(); }

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
        node_t dim;
        // Lookup tables
        vector< vector<size_t> > lookup;
        vector< std::pair<node_t, node_t> > inv_lookup;

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

