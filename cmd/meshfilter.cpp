/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
#include "progressbar.h"
#include "thread_queue.h"

#include "surface/mesh.h"
#include "surface/mesh_multi.h"
#include "surface/filter/base.h"
#include "surface/filter/smooth.h"



using namespace MR;
using namespace App;
using namespace MR::Surface;


const char* filters[] = { "smooth", NULL };


const OptionGroup smooth_option = OptionGroup ("Options for mesh smoothing filter")

    + Option ("smooth_spatial", "spatial extent of smoothing (default: " + str(Filter::default_smoothing_spatial_factor, 2) + "mm)")
      + Argument ("value").type_float (0.0)

    + Option ("smooth_influence", "influence factor for smoothing (default: " + str(Filter::default_smoothing_influence_factor, 2) + ")")
      + Argument ("value").type_float (0.0);


void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Apply filter operations to meshes";

  DESCRIPTION
  + "While this command has only one filter operation currently available, it "
    "nevertheless presents with a comparable interface to the MRtrix3 commands "
    "maskfilter and mrfilter commands.";

  EXAMPLES
  + Example ("Apply a mesh smoothing filter (currently the only filter available",
             "meshfilter input.vtk smooth output.vtk",
             "The usage of this command may cause confusion due to the generic interface "
             "despite only one filtering operation being currently available. This simple "
             "example usage is therefore provided for clarity.");

  ARGUMENTS
  + Argument ("input",  "the input mesh file").type_file_in()
  + Argument ("filter", "the filter to apply."
                        "Options are: smooth").type_choice (filters)
  + Argument ("output", "the output mesh file").type_file_out();

  OPTIONS
  + smooth_option;

}



void run ()
{

  MeshMulti in;

  // Read in the mesh data
  try {
    Mesh mesh (argument[0]);
    in.push_back (mesh);
  } catch (...) {
    in.load (argument[0]);
  }

  MeshMulti out;

  // Apply the relevant filter
  std::unique_ptr<Filter::Base> filter;
  int filter_index = argument[1];
  if (filter_index == 0) {
    const default_type spatial   = get_option_value ("smooth_spatial",   Filter::default_smoothing_spatial_factor);
    const default_type influence = get_option_value ("smooth_influence", Filter::default_smoothing_influence_factor);
    const std::string msg = in.size() > 1 ? "Applying smoothing filter to multiple meshes" : "";
    filter.reset (new Filter::Smooth (msg, spatial, influence));
  } else {
    assert (0);
  }

  out.assign (in.size(), Mesh());
  (*filter) (in, out);

  // Create the output file
  if (out.size() == 1)
    out.front().save (argument[2]);
  else
    out.save (argument[2]);

}
