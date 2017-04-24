/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "header.h"
#include "surface/mesh.h"
#include "surface/mesh_multi.h"
#include "surface/filter/vertex_transform.h"



using namespace MR;
using namespace App;
using namespace MR::Surface;



const char* transform_choices[] = { "first2real", "real2first", "voxel2real", "real2voxel", nullptr };



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Convert meshes between different formats, and apply transformations";

  ARGUMENTS
  + Argument ("input",  "the input mesh file").type_file_in()
  + Argument ("output", "the output mesh file").type_file_out();

  OPTIONS
  + Option ("binary", "write the output mesh file in binary format (if supported)")

  + Option ("transform", "transform vertices from one coordinate space to another, based on a template image; "
                         "options are: " + join(transform_choices, ", "))
    + Argument ("mode").type_choice (transform_choices)
    + Argument ("image").type_image_in();

}



void run ()
{

  // Read in the mesh data
  MeshMulti meshes;
  try {
    MR::Surface::Mesh mesh (argument[0]);
    meshes.push_back (mesh);
  } catch (Exception& e) {
    try {
      meshes.load (argument[0]);
    } catch (...) {
      throw e;
    }
  }

  auto opt = get_options ("transform");
  if (opt.size()) {
    auto H = Header::open (opt[0][1]);
    auto transform = make_unique<Surface::Filter::VertexTransform> (H);
    switch (int(opt[0][0])) {
      case 0: transform->set_first2real(); break;
      case 1: transform->set_real2first(); break;
      case 2: transform->set_voxel2real(); break;
      case 3: transform->set_real2voxel(); break;
      default: throw Exception ("Unexpected mode for spatial transformation of vertices");
    }
    MeshMulti temp;
    (*transform) (meshes, temp);
    std::swap (meshes, temp);
  }

  // Create the output file
  if (meshes.size() == 1)
    meshes[0].save (argument[1], get_options ("binary").size());
  else
    meshes.save (argument[1]);

}
