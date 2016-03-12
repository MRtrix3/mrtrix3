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



#include "connectome/connectome.h"

#include "exception.h"
#include "mrtrix.h"


namespace MR {
namespace Connectome {


void verify_matrix (matrix_type& in, const node_t num_nodes)
{
  if (in.rows() != in.cols())
    throw Exception ("Connectome matrix is not square (" + str(in.rows()) + " x " + str(in.cols()) + ")");
  if (in.rows() != num_nodes)
    throw Exception ("Connectome matrix contains " + str(in.rows()) + " nodes; expected " + str(num_nodes));

  for (node_t row = 0; row != num_nodes; ++row) {
    for (node_t column = row+1; column != num_nodes; ++column) {

      const float lower_value = in (column, row);
      const float upper_value = in (row, column);

      if (upper_value && lower_value && (upper_value != lower_value))
        throw Exception ("Connectome matrix is not symmetrical");

      if (!upper_value && lower_value)
        in (row, column) = lower_value;

      in (column, row) = 0.0f;

  } }
}


}
}


