/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#include "command.h"
#include "connectome/enhance.h"

using namespace MR;
using namespace MR::Connectome;
using namespace MR::Math;
using namespace App;


const char* operations[] = {
  "to_symmetric",
  "upper_triangular",
  "lower_triangular",
  "transpose",
  "zero_diagonal",
  NULL
};


void usage ()
{
  AUTHOR = "Matteo Frigo (matteo.frigo@inria.fr)";

  SYNOPSIS = "Perform basic operations on a connectome";

  ARGUMENTS
  + Argument ("input", "the input connectome.").type_text ()

  + Argument ("operation", "the operation to apply, one of: " + join(operations, ", ") + ".").type_choice (operations)

  + Argument ("output", "the output connectome.").type_text();
}


MR::Connectome::matrix_type to_symmetric (MR::Connectome::matrix_type c) {
  MR::Connectome::to_symmetric (c);
  return c;
}


MR::Connectome::matrix_type upper_triangular (MR::Connectome::matrix_type c) {
  MR::Connectome::to_upper(c);
  return c;
}


MR::Connectome::matrix_type lower_triangular (MR::Connectome::matrix_type c) {
  MR::Connectome::matrix_type t = c.transpose();
  MR::Connectome::to_upper(t);
  c = t.transpose();
  return c;
}


MR::Connectome::matrix_type transpose (MR::Connectome::matrix_type c) {
  return c.transpose();
}


MR::Connectome::matrix_type zero_diagonal (MR::Connectome::matrix_type c) {
  MR::Connectome::value_type zero = 0;
  for (node_t row = 0; row != c.rows(); ++row) {
    c (row, row) = zero;
  }
  return c;
}


void run ()
{
  const int op = argument[1];

  const std::string& output_path = argument[2];

  MR::Connectome::matrix_type connectome = load_matrix (argument[0]);
  
  MR::Connectome::check(connectome);
 
  INFO("Applying \'" + str(operations[op]) + "\' transformation to the input connectome.");
  
  switch (op) {
      case 0: connectome = to_symmetric (connectome); break;
      case 1: connectome = upper_triangular (connectome); break;
      case 2: connectome = lower_triangular (connectome); break;
      case 3: connectome = transpose (connectome); break;
      case 4: connectome = zero_diagonal (connectome); break;
      default: assert (0);
    }

  MR::save_matrix(connectome, output_path);
}