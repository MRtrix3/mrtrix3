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

#include "denoise/kernel/kernel.h"

#include "denoise/kernel/base.h"
#include "denoise/kernel/cuboid.h"
#include "denoise/kernel/sphere_radius.h"
#include "denoise/kernel/sphere_ratio.h"
#include "math/math.h"

namespace MR::Denoise::Kernel {

using namespace App;

const char *const shape_description =
    "The sliding spatial window behaves differently at the edges of the image FoV "
    "depending on the shape / size selected for that window. "
    "The default behaviour is to use a spherical kernel centred at the voxel of interest, "
    "whose size is some multiple of the number of input volumes; "
    "where some such voxels lie outside of the image FoV, "
    "the radius of the kernel will be increased until the requisite number of voxels are used. "
    "For a spherical kernel of a fixed radius, "
    "no such expansion will occur, "
    "and so for voxels near the image edge a reduced number of voxels will be present in the kernel. "
    "For a cuboid kernel, "
    "the centre of the kernel will be offset from the voxel being processed "
    "such that the entire volume of the kernel resides within the image FoV.";

const char *const default_size_description =
    "The size of the default spherical kernel is set to select a number of voxels that is "
    "1.0 / 0.85 ~ 1.18 times the number of volumes in the input series. "
    "If a cuboid kernel is requested, "
    "but the -extent option is not specified, "
    "the command will select the smallest isotropic patch size "
    "that exceeds the number of DW images in the input data; "
    "e.g., 5x5x5 for data with <= 125 DWI volumes, "
    "7x7x7 for data with <= 343 DWI volumes, etc.";

const char *const cuboid_size_description =
    "Permissible sizes for the cuboid kernel depend on the subsampling factor. "
    "If no subsampling is performed, or the subsampling factor is odd, "
    "then the extent(s) of the kernel must be odd, "
    "such that a unique voxel lies at the very centre of each kernel. "
    "If however an even subsampling factor is used, "
    "then the extent(s) of the kernel must be even, "
    "reflecting the fact that it is a voxel corner that resides at the centre of the kernel."
    "In either case, if the extent is specified manually, "
    "the user can either provide a single integer---"
    "which will determine the number of voxels in the kernel across all three spatial axes---"
    "or a comma-separated list of three integers,"
    "individually defining the number of voxels in the kernel for all three spatial axes.";

// clang-format off
const OptionGroup options = OptionGroup("Options for controlling the sliding spatial window kernel")
+ Option("shape",
         "Set the shape of the sliding spatial window. "
         "Options are: " + join(shapes, ",") + "; default: sphere")
  + Argument("choice").type_choice(shapes)
+ Option("radius_mm", "Set an absolute spherical kernel radius in mm")
  + Argument("value").type_float(0.0)
+ Option("radius_ratio",
         "Set the spherical kernel size as a ratio of number of voxels to number of input volumes "
         "(default: 1.0/0.85 ~= 1.18)")
  + Argument("value").type_float(0.0)
// TODO Command-line option that allows user to specify minimum absolute number of voxels in kernel
+ Option("extent",
         "Set the patch size of the cuboid kernel; "
         "can be either a single integer or a comma-separated triplet of integers (see Description)")
  + Argument("window").type_sequence_int();
// clang-format on

std::shared_ptr<Base>
make_kernel(const Header &H, const std::array<ssize_t, 3> &subsample_factors, const default_type size_multiplier) {
  auto opt = App::get_options("shape");
  const Kernel::shape_type shape = opt.empty() ? Kernel::shape_type::SPHERE : Kernel::shape_type((int)(opt[0][0]));
  std::shared_ptr<Kernel::Base> kernel;

  switch (shape) {
  case Kernel::shape_type::SPHERE: {
    // TODO Could infer that user wants a cuboid kernel if -extent is used, even if -shape is not
    if (!get_options("extent").empty())
      throw Exception("-extent option does not apply to spherical kernel");
    opt = get_options("radius_mm");
    if (opt.empty())
      return std::make_shared<SphereRatio>(
          H, subsample_factors, get_option_value("radius_ratio", sphere_multiplier_default * size_multiplier));
    return std::make_shared<SphereFixedRadius>(H, subsample_factors, default_type(opt[0][0]) * size_multiplier);
  }
  case Kernel::shape_type::CUBOID: {
    if (!get_options("radius_mm").empty() || !get_options("radius_ratio").empty())
      throw Exception("-radius_* options are inapplicable if cuboid kernel shape is selected");
    opt = get_options("extent");
    std::array<ssize_t, 3> extent;
    if (!opt.empty()) {
      auto userinput = parse_ints<uint32_t>(opt[0][0]);
      if (userinput.size() == 1)
        extent = {userinput[0], userinput[0], userinput[0]};
      else if (userinput.size() == 3)
        extent = {userinput[0], userinput[1], userinput[2]};
      else
        throw Exception("-extent must be either a scalar or a list of length 3");
      for (ssize_t axis = 0; axis < 3; ++axis) {
        if (extent[axis] > H.size(axis))
          throw Exception("-extent must not exceed the image dimensions");
        if ((subsample_factors[axis] & 1) != (extent[axis] & 1))
          throw Exception("-extent must match subsampling factors "
                          "(odd for no subsampling or subsampling by an odd factor; "
                          "even for subsampling by an even factor)");
      }
      // If the size multiplier is large enough to trigger an increment in cuboid extent,
      //   make the requisite change here
      const ssize_t user_patch_size = extent[0] * extent[1] * extent[2];
      do {
        const ssize_t new_size = (extent[0] + 2) * (extent[1] + 2) * (extent[2] + 2);
        if (new_size > size_multiplier * user_patch_size)
          break;
        extent = {extent[0] + 2, extent[1] + 2, extent[2] + 2};
      } while (true);
    } else {
      extent = {subsample_factors[0] & 1 ? 3 : 2, subsample_factors[1] & 1 ? 3 : 2, subsample_factors[2] & 1 ? 3 : 2};
      ssize_t prev_num_voxels = 0; // Exit loop below if maximum achievable extent is reached
      while (extent[0] * extent[1] * extent[2] < size_multiplier * std::max(H.size(3), prev_num_voxels)) {
        prev_num_voxels = extent[0] * extent[1] * extent[2];
        // If multiple axes are tied for spatial extent in mm, increment all of them
        const default_type min_length =
            std::min({extent[0] * H.spacing(0), extent[1] * H.spacing(1), extent[2] * H.spacing(2)});
        for (ssize_t axis = 0; axis != 3; ++axis) {
          if (extent[axis] * H.spacing(axis) == min_length && extent[axis] + 2 <= H.size(axis))
            extent[axis] += 2;
        }
      }
    }
    INFO("selected cuboid patch size: " + str(extent[0]) + " x " + str(extent[1]) + " x " + str(extent[2]));

    if (std::min(H.size(3), extent[0] * extent[1] * extent[2]) < 15) {
      WARN("The number of volumes or the patch size is small; "
           "this may lead to discretisation effects in the noise level "
           "and cause inconsistent denoising between adjacent voxels");
    }

    return std::make_shared<Cuboid>(H, subsample_factors, extent);
  } break;
  default:
    assert(false);
  }
  return nullptr;
}

} // namespace MR::Denoise::Kernel
