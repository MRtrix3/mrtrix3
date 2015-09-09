/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2015.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "args.h"
#include "command.h"
#include "progressbar.h"
#include "thread_queue.h"

#include "mesh/mesh.h"



#define SMOOTH_SPATIAL_DEFAULT 10.0f
#define SMOOTH_INFLUENCE_DEFAULT 10.0f



using namespace MR;
using namespace App;
using namespace MR::Mesh;


const char* filters[] = { "smooth", NULL };


const OptionGroup smooth_option = OptionGroup ("Options for mesh smoothing filter")

    + Option ("smooth_spatial", "spatial extent of smoothing")
      + Argument ("value").type_float (0.0f, SMOOTH_SPATIAL_DEFAULT, std::numeric_limits<float>::infinity())

    + Option ("smooth_influence", "influence factor for smoothing")
      + Argument ("value").type_float (0.0f, SMOOTH_INFLUENCE_DEFAULT, std::numeric_limits<float>::infinity());


void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "apply filter operations to meshes.";

  ARGUMENTS
  + Argument ("input",  "the input mesh file").type_file_in()
  + Argument ("filter", "the filter to apply."
                        "Options are: smooth").type_choice (filters)
  + Argument ("output", "the output mesh file").type_file_out();

  OPTIONS
  + smooth_option;

};



void run ()
{

  MeshMulti meshes;

  // Read in the mesh data
  try {
    MR::Mesh::Mesh mesh (argument[0]);
    meshes.push_back (mesh);
  } catch (...) {
    meshes.load (argument[0]);
  }

  // Apply the relevant filter
  int filter = argument[1];
  if (filter == 0) {

    Options opt = get_options ("smooth_spatial");
    const float spatial = opt.size() ? opt[0][0] : SMOOTH_SPATIAL_DEFAULT;
    opt = get_options ("smooth_influence");
    const float influence = opt.size() ? opt[0][0] : SMOOTH_INFLUENCE_DEFAULT;

    if (meshes.size() == 1) {
      meshes.front().smooth (spatial, influence);
    } else {
      std::mutex mutex;
      ProgressBar progress ("Applying smoothing filter to multiple meshes... ", meshes.size());
      auto loader = [&] (size_t& out) { static size_t i = 0; out = i++; return (out != meshes.size()); };
      auto worker = [&] (const size_t& in) { meshes[in].smooth (spatial, influence); std::lock_guard<std::mutex> lock (mutex); ++progress; return true; };
      Thread::run_queue (loader, size_t(), Thread::multi (worker));
    }

  } else {
    assert (0);
  }

  // Create the output file
  if (meshes.size() == 1)
    meshes.front().save (argument[2]);
  else
    meshes.save (argument[2]);

}
