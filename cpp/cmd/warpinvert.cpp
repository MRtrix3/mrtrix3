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
#include "datatype.h"
#include "header.h"
#include "image.h"
#include "registration/warp/helpers.h"
#include "registration/warp/invert.h"
#include "registration/warp/validate.h"

using namespace MR;
using namespace App;

// Choice strings for the -extrapolate option; index maps to ExtrapolateDegree.
const std::vector<std::string> extrapolate_choices{"adaptive", "affine"};

// Choice strings for the -validity option; index maps to Interp::ValidityPolicy.
const std::vector<std::string> validity_choices{"interpolated", "nearest"};

// clang-format off
void usage() {
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)"
           " and David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Invert a non-linear warp field";

  DESCRIPTION
  + "By default, this command assumes that the input warp field is a deformation field,"
    " i.e. each voxel stores the corresponding position in the other image"
    " (in scanner space),"
    " and the calculated output warp image will also be a deformation field."
    " If the input warp field is instead a displacment field,"
    " i.e. where each voxel stores an offset from which to sample the other image"
    " (but still in scanner space),"
    " then the -displacement option should be used;"
    " the output warp field will additionally be calculated as a displacement field in this case.";

  ARGUMENTS
  + Argument ("in", "the input warp image.").type_image_in()
  + Argument ("out", "the output warp image.").type_image_out();

  OPTIONS
  + Option ("template", "define a template image grid for the output warp")
  + Argument ("image").type_image_in ()

  + Option ("displacement", "indicates that the input warp field is a displacement field;"
                            " the output will also be a displacement field")

  + Option ("extrapolate", "polynomial degree used to extrapolate the warp field across"
                           " the halo around the valid region prior to cubic interpolation"
                           " (default: adaptive); a diagnostic control for assessing the"
                           " sensitivity of the inverted valid region to halo extrapolation")
  + Argument ("mode").type_choice(extrapolate_choices)

  + Option ("validity", "how a sampled position is judged to reside in valid input data:"
                        " \"interpolated\" (default) thresholds the trilinearly-interpolated"
                        " validity field at one half, placing the accept boundary at the true"
                        " sub-voxel region edge; \"nearest\" uses the validity of the enclosing"
                        " voxel (nearest-voxel rounding), retained for assessing its effect")
  + Argument ("mode").type_choice(validity_choices);

}
// clang-format on

void run() {
  const bool displacement = !get_options("displacement").empty();
  Header header_in(Header::open(argument[0]));
  auto format = Registration::Warp::validate_header(header_in);
  if (format != Registration::Warp::WarpFormat::Simple)
    throw Exception("Command requires as input a 4D deformation or displacement field,"
                    " not a 5D \"full\" warp field series"
                    " (see MRtrix command \"warpconvert\")");
  Header header_out(header_in);
  auto opt = get_options("template");
  if (!opt.empty()) {
    header_out = Header::open(opt[0][0]);
    header_out.ndim() = 4;
    header_out.size(3) = 3;
    header_out.datatype() = DataType::Float32;
    header_out.datatype().set_byte_order_native();
  }

  auto degree = Registration::Warp::ExtrapolateDegree::Adaptive;
  auto extrap_opt = get_options("extrapolate");
  if (!extrap_opt.empty()) {
    degree = (static_cast<int>(extrap_opt[0][0]) == 1) ? Registration::Warp::ExtrapolateDegree::Affine
                                                       : Registration::Warp::ExtrapolateDegree::Adaptive;
  }

  auto validity_policy = Interp::ValidityPolicy::Interpolated;
  auto validity_opt = get_options("validity");
  if (!validity_opt.empty()) {
    validity_policy = (static_cast<int>(validity_opt[0][0]) == 1) ? Interp::ValidityPolicy::Nearest
                                                                  : Interp::ValidityPolicy::Interpolated;
  }

  Image<default_type> image_in(header_in.get_image<default_type>());
  Registration::Warp::debug_validate_image(image_in);
  Image<default_type> image_out(Image<default_type>::create(argument[1], header_out));

  if (displacement) {
    Registration::Warp::invert_displacement_warp(image_in, image_out, false, 50, 0.0001, degree, validity_policy);
  } else {
    Registration::Warp::invert_deformation(image_in, image_out, false, 50, 0.0001, degree, validity_policy);
  }
}
