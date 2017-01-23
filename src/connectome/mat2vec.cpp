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


#include "connectome/mat2vec.h"


namespace MR {
namespace Connectome {



Mat2Vec::Mat2Vec (const node_t i) :
    size (i)
{
  lookup.assign (size, vector<size_t> (size, 0));
  inv_lookup.reserve (size * (size+1) / 2);
  size_t index = 0;
  for (node_t row = 0; row != size; ++row) {
    for (node_t column = row; column != size; ++column) {
      lookup[row][column] = lookup[column][row] = index++;
      inv_lookup.push_back (std::make_pair (row, column));
    }
  }
}



Mat2Vec& Mat2Vec::operator= (Mat2Vec&& that)
{
  size = that.size; that.size = 0;
  lookup = std::move (that.lookup);
  inv_lookup = std::move (that.inv_lookup);
  return *this;
}






}
}



