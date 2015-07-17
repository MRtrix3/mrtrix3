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


#include "connectome/mat2vec.h"


namespace MR {
namespace Connectome {



Mat2Vec::Mat2Vec (const node_t i) :
    dim (i)
{
  lookup.assign (dim, std::vector<size_t> (dim, 0));
  inv_lookup.reserve (dim * (dim+1) / 2);
  size_t index = 0;
  for (node_t row = 0; row != dim; ++row) {
    for (node_t column = row; column != dim; ++column) {
      lookup[row][column] = lookup[column][row] = index++;
      inv_lookup.push_back (std::make_pair (row, column));
    }
  }
}



Mat2Vec& Mat2Vec::operator= (Mat2Vec&& that)
{
  dim = that.dim;
  lookup = std::move (that.lookup);
  inv_lookup = std::move (that.inv_lookup);
  return *this;
}






}
}



