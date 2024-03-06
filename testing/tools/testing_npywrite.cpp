/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <string>

#include "command.h"
#include "datatype.h"
#include "file/matrix.h"
#include "file/npy.h"
#include "file/path.h"
#include "file/utils.h"
#include "half.h"
#include "types.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Test the writing of NPY files";

  ARGUMENTS
  + Argument("out_dir", "Path to output directory in which test data will be generated").type_directory_out();
}
// clang-format on

const Eigen::Array<default_type, 3, 1> reference_1d{0.0, 1.0, 2.0};
const Eigen::Array<bool, 3, 1> reference_1d_bool{false, true, true};
const Eigen::Array<default_type, 3, 2> reference_2d{{0.0, 1.0}, {10.0, 11.0}, {20.0, 21.0}};
const Eigen::Array<bool, 3, 2> reference_2d_bool{{false, true}, {true, true}, {true, true}};

template <typename T> void save_1d(const std::string &path) {
  File::Matrix::save_vector(reference_1d.cast<T>(), Path::join(argument[0], path));
}

template <typename T> void save_2d(const std::string &path) {
  File::Matrix::save_matrix(reference_2d.cast<T>(), Path::join(argument[0], path));
}

void run() {
  File::mkdir(argument[0]);

  File::Matrix::save_vector(reference_1d_bool, Path::join(argument[0], "1D3_BOOL.npy"));
  save_1d<int8_t>("1D3_i1.npy");
  save_1d<uint8_t>("1D3_u1.npy");
  save_1d<int16_t>("1D3_i2.npy");
  save_1d<uint16_t>("1D3_u2.npy");
  save_1d<half_float::half>("1D3_f2.npy");
  save_1d<int32_t>("1D3_i4.npy");
  save_1d<uint32_t>("1D3_u4.npy");
  save_1d<float>("1D3_f4.npy");
  save_1d<int64_t>("1D3_i8.npy");
  save_1d<uint64_t>("1D3_u8.npy");
  save_1d<double>("1D3_f8.npy");

  File::Matrix::save_matrix(reference_2d_bool, Path::join(argument[0], "2D3x2_BOOL.npy"));
  save_2d<int8_t>("2D3x2_i1.npy");
  save_2d<uint8_t>("2D3x2_u1.npy");
  save_2d<int16_t>("2D3x2_i2.npy");
  save_2d<uint16_t>("2D3x2_u2.npy");
  save_2d<half_float::half>("2D3x2_f2.npy");
  save_2d<int32_t>("2D3x2_i4.npy");
  save_2d<uint32_t>("2D3x2_u4.npy");
  save_2d<float>("2D3x2_f4.npy");
  save_2d<int64_t>("2D3x2_i8.npy");
  save_2d<uint64_t>("2D3x2_u8.npy");
  save_2d<double>("2D3x2_f8.npy");
}
