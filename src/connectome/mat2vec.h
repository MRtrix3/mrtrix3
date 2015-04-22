/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 22/12/2014.

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



#ifndef __connectome_mat2vec_h__
#define __connectome_mat2vec_h__


#include <vector>

#include "math/matrix.h"
#include "math/vector.h"

#include "connectome/connectome.h"


namespace MR {
namespace Connectome {




class Mat2Vec
{

  public:
    Mat2Vec (const index_t);
    Mat2Vec& operator= (Mat2Vec&&);

    index_t operator() (const index_t i, const index_t j) const
    {
      assert (i < dim);
      assert (j < dim);
      return lookup[i][j];
    }
    std::pair<index_t, index_t> operator() (const index_t i) const
    {
      assert (i < inv_lookup.size());
      return inv_lookup[i];
    }
    index_t size() const { return dim; }
    index_t vec_size() const { return inv_lookup.size(); }

    // Complete Matrix->Vector and Vector->Matrix conversion
    template <typename T> Math::Vector<T>& operator() (const Math::Matrix<T>&, Math::Vector<T>&) const;
    template <typename T> std::vector<T>&  operator() (const Math::Matrix<T>&, std::vector<T>&) const;
    template <class Cont, typename T> Math::Matrix<T>& operator() (const Cont&, Math::Matrix<T>&) const;

  protected:
    index_t dim;

  private:
    // Lookup tables
    std::vector< std::vector<index_t> > lookup;
    std::vector< std::pair<index_t, index_t> > inv_lookup;

};




template <typename T>
Math::Vector<T>& Mat2Vec::operator() (const Math::Matrix<T>& in, Math::Vector<T>& out) const
{
  assert (in.rows() == in.columns());
  assert (in.rows() == dim);
  out.resize (vec_size());
  for (index_t index = 0; index != out.size(); ++index) {
    const std::pair<index_t, index_t> row_column = (*this) (index);
    out[index] = in (row_column.first, row_column.second);
  }
  return out;
}


template <typename T>
std::vector<T>& Mat2Vec::operator() (const Math::Matrix<T>& in, std::vector<T>& out) const
{
  assert (in.rows() == in.columns());
  assert (in.rows() == dim);
  out.resize (vec_size());
  for (index_t index = 0; index != out.size(); ++index) {
    const std::pair<index_t, index_t> row_column = (*this) (index);
    out[index] = in (row_column.first, row_column.second);
  }
  return out;
}


template <class Cont, typename T>
Math::Matrix<T>& Mat2Vec::operator() (const Cont& in, Math::Matrix<T>& out) const
{
  assert (in.size() == vec_size());
  out.resize (dim, dim);
  for (index_t row = 0; row != dim; ++row) {
    for (index_t column = 0; column != dim; ++column)
      out (row, column) = in[(*this) (row, column)];
  }
  return out;
}




}
}


#endif

