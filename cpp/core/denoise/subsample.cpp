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

#include "denoise/subsample.h"

namespace MR::Denoise {

const App::Option subsample_option =
    App::Option("subsample",
                "reduce the number of PCA kernels relative to the number of image voxels; "
                "can provide either an integer subsampling factor, "
                "or a comma-separated list of three factors; "
                "default: 2") +
    App::Argument("factor").type_integer(1);

Subsample::Subsample(const Header &in, const std::array<ssize_t, 3> &factors)
    : H_in(make_input_header(in)),
      factors(factors),
      size({(in.size(0) + factors[0] - 1) / factors[0],
            (in.size(1) + factors[1] - 1) / factors[1],
            (in.size(2) + factors[2] - 1) / factors[2]}),
      origin({(in.size(0) - factors[0] * (size[0] - 1) - 1) / 2,
              (in.size(1) - factors[1] * (size[1] - 1) - 1) / 2,
              (in.size(2) - factors[2] * (size[2] - 1) - 1) / 2}),
      H_ss(make_subsample_header()) {}

bool Subsample::process(const Kernel::Voxel::index_type &pos) const {
  for (ssize_t axis = 0; axis != 3; ++axis) {
    if (pos[axis] % factors[axis] != origin[axis])
      return false;
  }
  return true;
}

std::array<ssize_t, 3> Subsample::in2ss(const Kernel::Voxel::index_type &pos) const {
  // Do not attempt to map an unprocessed voxel to a voxel index in subsampled space
  assert(process(pos));
  assert(!is_out_of_bounds(H_in, pos, 0, 3));
  return std::array<ssize_t, 3>({(pos[0] - origin[0]) / factors[0],   //
                                 (pos[1] - origin[1]) / factors[1],   //
                                 (pos[2] - origin[2]) / factors[2]}); //
}

std::array<ssize_t, 3> Subsample::ss2in(const Kernel::Voxel::index_type &pos) const {
  assert(!is_out_of_bounds(H_ss, pos));
  return std::array<ssize_t, 3>({pos[0] * factors[0] + origin[0],   //
                                 pos[1] * factors[1] + origin[1],   //
                                 pos[2] * factors[2] + origin[2]}); //
}

std::shared_ptr<Subsample> Subsample::make(const Header &in, const ssize_t default_ratio) {
  auto opt = App::get_options("subsample");
  std::array<ssize_t, 3> factors({default_ratio, default_ratio, default_ratio});
  if (!opt.empty()) {
    const std::vector<ssize_t> userinput = parse_ints<ssize_t>(opt[0][0]);
    if (userinput.size() == 1)
      factors = {userinput[0], userinput[0], userinput[0]};
    else if (userinput.size() == 3)
      factors = {userinput[0], userinput[1], userinput[2]};
    else
      throw Exception("Subsampling factor must be either a single positive integer, "
                      "or a comma-separated list of three positive integers");
  }
  return std::make_shared<Subsample>(in, factors);
}

Header Subsample::make_input_header(const Header &H_in) const {
  Header H(H_in);
  H.ndim() = 3;
  H.reset_intensity_scaling();
  H.datatype() = DataType::Float32;
  H.datatype().set_byte_order_native();
  return H;
}

Header Subsample::make_subsample_header() const {
  Header H(H_in);
  H.ndim() = 3;
  H.reset_intensity_scaling();
  H.datatype() = DataType::Float32;
  H.datatype().set_byte_order_native();
  std::array<double, 3> halfvoxel_offsets;
  for (ssize_t axis = 0; axis != 3; ++axis) {
    H.size(axis) = size[axis];
    H.spacing(axis) *= factors[axis];
    halfvoxel_offsets[axis] = factors[axis] & 1 ? 0.0 : 0.5;
  }
  H.transform().translation() =
      H_in.transform() * Eigen::Matrix<default_type, 3, 1>({(origin[0] + halfvoxel_offsets[0]) * H_in.spacing(0),   //
                                                            (origin[1] + halfvoxel_offsets[1]) * H_in.spacing(1),   //
                                                            (origin[2] + halfvoxel_offsets[2]) * H_in.spacing(2)}); //
  return H;
}

} // namespace MR::Denoise
