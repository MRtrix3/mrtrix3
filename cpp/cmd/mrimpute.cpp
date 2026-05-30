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

#include <cmath>

#include "command.h"
#include "datatype.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"

#include "algo/impute.h"
#include "algo/loop.h"
#include "enum.h"
#include "misc/voxel2vector.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Impute predicted intensities into invalid image voxels";

  DESCRIPTION
  + "This command fills \"invalid\" voxels in an image with intensities predicted"
    " from the surrounding valid data."
    " By default,"
    " voxels with non-finite values (NaN or Inf) are imputed;"
    " the -mask option flags additional voxels to be imputed."
    " For images with more than three axes,"
    " each 3D volume is processed independently."

  + "The set of voxels to be imputed is serialised into the unknowns of a linear"
    " system that is solved per 3D volume."
    " The available algorithms are derived from the 2D MatLab module Inpaint_nans"
    " (generalised here to 3D),"
    " together with two \"isotropic\" methods that solve the same partial"
    " differential equations over a spherically-symmetric finite-difference"
    " stencil:"

  + "laplacian: solve the Laplace (del-squared) equation as an overdetermined"
    " least-squares system (Inpaint_nans method 0)."

  + "laplaciansq: solve the same Laplace equation as a square system,"
    " with exactly one equation per imputed voxel (Inpaint_nans method 2)."

  + "biharmonic: solve the biharmonic (del-to-the-fourth) equation"
    " (Inpaint_nans method 3)."

  + "hessian: minimise the discrete Hessian (Frobenius) energy as an"
    " overdetermined least-squares system."
    " Like biharmonic it extrapolates a linear trend beyond a one-sided boundary"
    " rather than flattening,"
    " but its natural (free) boundary conditions introduce no boundary bias."

  + "spring: constrain each imputed voxel toward equality with its neighbours"
    " (Inpaint_nans method 4); yields constant extrapolation."

  + "isotropic2 / isotropic4: as for laplacian / biharmonic respectively,"
    " but assembled from a 13-direction spherical-harmonic-weighted stencil"
    " for improved rotational invariance."

  + "Inpaint_nans methods 1 (redundant with method 0)"
    " and 5 (an author-discouraged neighbour average) are intentionally omitted."
    " The linear solver is selected automatically per method"
    " (dense QR for the least-squares methods; dense LU for the square method)."

  + "The -detrend option fits a low-order polynomial trend to the known"
    " data bordering the region to be imputed,"
    " subtracts it before the solve,"
    " and re-adds it afterwards (a \"universal kriging\" decomposition):"
    " affine fits a first-order trend, quadratic a second-order trend."
    " This carries any global gradient (and, for quadratic, curvature)"
    " into the imputed region in closed form,"
    " leaving the solver to resolve only the bounded residual;"
    " it is recommended when extrapolating beyond a one-sided data boundary,"
    " where a purely harmonic solve would otherwise flatten to a constant."

  + "The imputation system is dense and scoped to each 3D volume;"
    " this is efficient for typical hole counts,"
    " but very large contiguous regions to be imputed will produce a large"
    " dense system.";

  ARGUMENTS
  + Argument ("input", "the input image").type_image_in()
  + Argument ("output", "the output image").type_image_out();

  OPTIONS
  + Option ("mask", "a bitwise mask image flagging additional voxels to impute"
                    " (beyond the non-finite voxels imputed by default)")
    + Argument ("image").type_image_in()

  + Option ("method", "the imputation algorithm to use"
                      " (default: laplacian);"
                      " one of: " + MR::Enum::join<Impute::Method>())
    + Argument ("name").type_choice<Impute::Method>()

  + Option ("detrend", "remove a parametric trend before imputation and re-add it afterwards"
                       " (default: none);"
                       " one of: " + MR::Enum::join<Impute::Detrend>())
    + Argument ("name").type_choice<Impute::Detrend>();

}
// clang-format on

using value_type = float;

namespace {

// Fill the output 3D slab at the current outer position from the input,
//   imputing those voxels flagged in the impute set.
void process_volume(Image<value_type> &image_in,
                    Image<value_type> &image_out,
                    Image<bool> &mask,
                    const Header &slab_header,
                    const Impute::Method method,
                    const Impute::Detrend detrend) {
  // Pass 1: determine the complete set of voxels to be imputed.
  Image<bool> impute_mask(Image<bool>::scratch(slab_header, "imputation region"));
  for (auto l = Loop(0, 3)(image_in, impute_mask); l; ++l) {
    bool invalid = !std::isfinite(image_in.value());
    if (mask.valid() && !invalid) {
      assign_pos_of(image_in, 0, 3).to(mask);
      invalid = mask.value();
    }
    impute_mask.value() = invalid;
  }

  const Voxel2Vector v2v(impute_mask, slab_header);
  if (v2v.empty()) {
    for (auto l = Loop(0, 3)(image_in, image_out); l; ++l)
      image_out.value() = image_in.value();
    return;
  }

  // Independent reader so that index manipulation during the solve does not
  //   disturb the iterators of the surrounding loops.
  Image<value_type> reader(image_in);
  auto value_at = [reader](const Impute::Position &p) mutable -> double {
    reader.index(0) = p[0];
    reader.index(1) = p[1];
    reader.index(2) = p[2];
    return static_cast<double>(reader.value());
  };
  auto in_fov = [&slab_header](const Impute::Position &p) -> bool { return !is_out_of_bounds(slab_header, p, 0, 3); };

  const Impute::Vec solution = Impute::make_imputer(method, v2v, value_at, in_fov, detrend)->solve();

  // Pass 2: write the output, substituting imputed values where flagged.
  for (auto l = Loop(0, 3)(image_in, image_out, impute_mask); l; ++l) {
    if (impute_mask.value()) {
      const Impute::Position p(image_in.index(0), image_in.index(1), image_in.index(2));
      image_out.value() = static_cast<value_type>(solution[v2v(p)]);
    } else {
      image_out.value() = image_in.value();
    }
  }
}

} // namespace

void run() {
  Header H_in(Header::open(argument[0]));
  if (H_in.datatype().is_complex())
    throw Exception("Command does not operate on complex image data");

  const Impute::Method method = get_option_choice<Impute::Method>("method", Impute::Method::laplacian);
  const Impute::Detrend detrend = get_option_choice<Impute::Detrend>("detrend", Impute::Detrend::none);

  // The square method requires a complete axis-aligned stencil at every voxel;
  //   reject data that does not represent a 3D volume.
  if (method == Impute::Method::laplaciansq)
    check_3D_nonunity(H_in);

  Image<value_type> image_in(H_in.get_image<value_type>());

  Image<bool> mask;
  auto opt = get_options("mask");
  if (!opt.empty()) {
    Header H_mask(Header::open(opt[0][0]));
    check_dimensions(H_in, H_mask, 0, 3);
    mask = H_mask.get_image<bool>();
  }

  Header H_out(H_in);
  H_out.datatype() = DataType::Float32;
  H_out.datatype().set_byte_order_native();
  Image<value_type> image_out(Image<value_type>::create(argument[1], H_out));

  Header slab_header(H_in);
  slab_header.ndim() = 3;

  if (image_in.ndim() > 3) {
    for (auto outer = Loop("Imputing image", image_in, 3)(image_in, image_out); outer; ++outer)
      process_volume(image_in, image_out, mask, slab_header, method, detrend);
  } else {
    process_volume(image_in, image_out, mask, slab_header, method, detrend);
  }
}
