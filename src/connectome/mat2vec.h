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


#include <vector>

#include "connectome/connectome.h"


namespace MR {
namespace Connectome {




class Mat2Vec
{ MEMALIGN(Mat2Vec)

  public:
    Mat2Vec (const node_t);
    Mat2Vec& operator= (Mat2Vec&&);

    size_t operator() (const node_t i, const node_t j) const
    {
      assert (i < size);
      assert (j < size);
      return lookup[i][j];
    }
    std::pair<node_t, node_t> operator() (const size_t i) const
    {
      assert (i < inv_lookup.size());
      return inv_lookup[i];
    }
    node_t mat_size() const { return size; }
    size_t vec_size() const { return inv_lookup.size(); }

    // Complete Matrix->Vector and Vector->Matrix conversion
    // Templating allows use of either an Eigen::Vector or a std::vector
    template <class Cont> Cont&        operator() (const matrix_type&, Cont&) const;
    template <class Cont> matrix_type& operator() (const Cont&, matrix_type&) const;

  protected:
    node_t size;

  private:
    // Lookup tables
    std::vector< std::vector<size_t> > lookup;
    std::vector< std::pair<node_t, node_t> > inv_lookup;

};




template <class Cont>
Cont& Mat2Vec::operator() (const matrix_type& in, Cont& out) const
{
  assert (in.rows() == in.cols());
  assert (in.rows() == size);
  out.resize (vec_size());
  for (size_t index = 0; index != vec_size(); ++index) {
    const std::pair<node_t, node_t> row_column = (*this) (index);
    out[index] = in (row_column.first, row_column.second);
  }
  return out;
}



template <class Cont>
matrix_type& Mat2Vec::operator() (const Cont& in, matrix_type& out) const
{
  assert (in.size() == vec_size());
  out.resize (size, size);
  for (node_t row = 0; row != size; ++row) {
    for (node_t column = 0; column != size; ++column)
      out (row, column) = in[(*this) (row, column)];
  }
  return out;
}




}
}


#endif

