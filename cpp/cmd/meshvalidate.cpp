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
#include "mrtrix.h"

#include "surface/mesh.h"
#include "surface/validate.h"

using namespace MR;
using namespace App;
using namespace MR::Surface;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate a mesh surface file";

  DESCRIPTION
  + "This command checks that a mesh surface file conforms to the"
    " requirements of a valid closed orientable surface."
    " The following properties are verified:"

  + "1. No disconnected vertices:"
    " every vertex in the file must be referenced by at least one polygon."

  + "2. No duplicate vertices:"
    " no two vertices may occupy the same 3D position."

  + "3. No duplicate polygons:"
    " no two polygons may reference the same set of vertex indices."

  + "4. Closed surface:"
    " every edge must be shared by exactly two polygons."
    " An edge belonging to only one polygon indicates a boundary (open surface);"
    " an edge shared by three or more polygons indicates a non-manifold mesh."

  + "5. Single connected component:"
    " all polygons must belong to a single connected piece."
    " A surface with multiple disconnected pieces is not valid."

  + "6. Consistent normal orientation:"
    " for every shared edge, the two adjacent polygons must traverse it"
    " in opposite directions."
    " If both polygons traverse the edge in the same direction"
    " their winding orders — and therefore their surface normals — are inconsistent.";

  ARGUMENTS
  + Argument ("mesh", "the input mesh file").type_file_in();
}
// clang-format on

void run() {
  const Mesh mesh(argument[0]);

  // validate_mesh() throws an Exception with a descriptive message
  // if any requirement is violated.
  Surface::validate(mesh);

  CONSOLE("Mesh \"" + std::string(argument[0]) + "\" is valid:" + //
          " " + str(mesh.num_vertices()) + " vertices," +         //
          " " + str(mesh.num_polygons()) + " polygon(s)" +        //
          " (" + str(mesh.num_triangles()) + " triangle(s)," +    //
          " " + str(mesh.num_quads()) + " quad(s))");             //
}
