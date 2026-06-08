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

#include "enum.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"

#include "surface/algo/mesh2image.h"
#include "surface/mesh.h"
#include "surface/validate.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Convert a mesh surface to a partial volume estimation image";

  REFERENCES
    + "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "Anatomically-constrained tractography:"
    " Improved diffusion MRI streamlines tractography through effective use of anatomical information. "
    "NeuroImage, 2012, 62, 1924-1938";

  ARGUMENTS
  + Argument ("source",   "the mesh file;"
                          " note vertices must be defined in realspace coordinates").type_file_in()
  + Argument ("template", "the template image").type_image_in()
  + Argument ("output",   "the output image").type_image_out();

  OPTIONS
  + Option ("algorithm", "algorithm for partial volume estimation;"
                         " options are: " + MR::Enum::join<Surface::Algo::Mesh2ImageMethod>() +
                         " (default: toblerone)")
    + Argument ("name").type_choice<Surface::Algo::Mesh2ImageMethod>()

  + Option ("subvoxel", "for the \"toblerone\" algorithm,"
                        " the target sub-voxel edge length in mm (default: 0.75)")
    + Argument ("value").type_float (0.0);

  REFERENCES
  + "If utilising the default \"toblerone\" algorithm:"
    " Kirk T.F., Coalson T.S., Craig M.S., Chappell M.A."
    " Toblerone: Surface-Based Partial Volume Estimation."
    " IEEE TMI 2020:39(5);1501-1510.";

}
// clang-format on

void run() {

  // Read in the mesh data
  Surface::Mesh mesh(argument[0]);
  Surface::debug_validate(mesh);

  // Get the template image
  Header template_header = Header::open(argument[1]);

  template_header.ndim() = 3;

  // Ensure that a floating-point representation is used for the output image,
  //   as is required for representing partial volumes
  template_header.datatype() = DataType::Float32;
  template_header.datatype().set_byte_order_native();

  // Create the output image
  Image<float> output = Image<float>::create(argument[2], template_header);

  // Select the partial volume estimation algorithm
  Surface::Algo::Mesh2ImageOptions options;
  auto opt = get_options("algorithm");
  if (!opt.empty())
    options.method = MR::Enum::from_name<Surface::Algo::Mesh2ImageMethod>(opt[0][0]);
  opt = get_options("subvoxel");
  if (!opt.empty())
    options.subvoxel_mm = static_cast<default_type>(opt[0][0]);

  // Perform the partial volume estimation
  Surface::Algo::mesh2image(mesh, output, options);
}
