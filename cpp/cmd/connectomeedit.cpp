/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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
#include "file/matrix.h"

using namespace MR;
using namespace MR::Connectome;
using namespace MR::Math;
using namespace App;

enum class Operation { TO_SYMMETRIC, UPPER_TRIANGULAR, LOWER_TRIANGULAR, TRANSPOSE, ZERO_DIAGONAL };
const std::vector<std::string> operations = lower_case_enums<Operation>();

// clang-format off
void usage() {

  AUTHOR = "Matteo Frigo (matteo.frigo@inria.fr)";

  SYNOPSIS = "Perform basic operations on a connectome";

  ARGUMENTS
  + Argument ("input", "the input connectome.").type_file_in()

  + Argument ("operation", "the operation to apply,"
                           " one of: " + join_enum<Operation>() + ".").type_choice<Operation>()

  + Argument ("output", "the output connectome.").type_file_out();

}
// clang-format on

void run() {
  MR::Connectome::matrix_type connectome = File::Matrix::load_matrix(argument[0]);
  MR::Connectome::check(connectome);
  const Operation op = enum_from_name<Operation>(argument[1]);
  const std::string_view output_path = argument[2];

  INFO("Applying \'" + lowercase_enum_name(op) + "\' transformation to the input connectome.");

  switch (op) {
  case Operation::TO_SYMMETRIC:
    MR::Connectome::to_symmetric(connectome);
    break;
  case Operation::UPPER_TRIANGULAR:
    MR::Connectome::to_upper(connectome);
    break;
  case Operation::LOWER_TRIANGULAR:
    MR::Connectome::to_upper(connectome);
    connectome.transposeInPlace();
    break;
  case Operation::TRANSPOSE:
    connectome.transposeInPlace();
    break;
  case Operation::ZERO_DIAGONAL:
    connectome.matrix().diagonal().setZero();
    break;
  default:
    assert(0);
  }

  File::Matrix::save_matrix(connectome, output_path);
}
