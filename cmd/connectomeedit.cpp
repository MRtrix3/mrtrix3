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



void run ()
{
  MR::Connectome::matrix_type connectome = load_matrix (argument[0]);
  MR::Connectome::check(connectome);
  const int op = argument[1];
  const std::string& output_path = argument[2];

  INFO("Applying \'" + str(operations[op]) + "\' transformation to the input connectome.");

  switch (op) {
      case 0: MR::Connectome::to_symmetric (connectome); break;
      case 1: MR::Connectome::to_upper (connectome); break;
      case 2: MR::Connectome::to_upper (connectome); connectome.transposeInPlace(); break;
      case 3: connectome.transposeInPlace(); break;
      case 4: connectome.matrix().diagonal().setZero(); break;
      default: assert (0);
    }

  MR::save_matrix(connectome, output_path);
}