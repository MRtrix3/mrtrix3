/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 22/04/2015.

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



#include "connectome/connectome.h"


namespace MR {
namespace Connectome {


void verify_matrix (Math::Matrix<float>& in, const node_t num_nodes)
{
  if (in.rows() != in.columns())
    throw Exception ("Connectome matrix is not square (" + str(in.rows()) + " x " + str(in.columns()) + ")");
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


