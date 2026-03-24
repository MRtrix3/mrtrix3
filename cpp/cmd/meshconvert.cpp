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
#include "surface/filter/vertex_transform.h"
#include "surface/mesh.h"
#include "surface/mesh_multi.h"

using namespace MR;
using namespace App;
using namespace MR::Surface;

enum class TransformChoice { First2Real, Real2First, Voxel2Real, Real2Voxel, Fs2Real };

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Convert meshes between different formats, and apply transformations";

  ARGUMENTS
  + Argument ("input",  "the input mesh file").type_file_in()
  + Argument ("output", "the output mesh file").type_file_out();

  OPTIONS
  + Option ("binary", "write the output mesh file in binary format (if supported)")

  + Option ("transform", "transform vertices from one coordinate space to another,"
                         " based on a template image;"
                         " options are: " + MR::Enum::join<TransformChoice>() + ".")
    + Argument ("mode").type_choice<TransformChoice>()
    + Argument ("image").type_image_in();

}
// clang-format on

void run() {

  // Read in the mesh data
  MeshMulti meshes;
  try {
    MR::Surface::Mesh mesh(argument[0]);
    meshes.push_back(mesh);
  } catch (Exception &e) {
    try {
      meshes.load(argument[0]);
    } catch (...) {
      throw e;
    }
  }

  auto opt = get_options("transform");
  if (!opt.empty()) {
    auto H = Header::open(opt[0][1]);
    auto transform = std::make_unique<Surface::Filter::VertexTransform>(H);
    switch (MR::Enum::from_name<TransformChoice>(opt[0][0])) {
    case TransformChoice::First2Real:
      transform->set_first2real();
      break;
    case TransformChoice::Real2First:
      transform->set_real2first();
      break;
    case TransformChoice::Voxel2Real:
      transform->set_voxel2real();
      break;
    case TransformChoice::Real2Voxel:
      transform->set_real2voxel();
      break;
    case TransformChoice::Fs2Real:
      transform->set_fs2real();
      break;
    default:
      throw Exception("Unexpected mode for spatial transformation of vertices");
    }
    MeshMulti temp;
    (*transform)(meshes, temp);
    std::swap(meshes, temp);
  }

  // Create the output file
  if (meshes.size() == 1)
    meshes[0].save(argument[1], !get_options("binary").empty());
  else
    meshes.save(argument[1]);
}
