/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */



#include "command.h"
#include "progressbar.h"
#include "thread_queue.h"

#include "mesh/mesh.h"



#define DEFAULT_SMOOTHING_SPATIAL 10.0
#define DEFAULT_SMOOTHING_INFLUENCE 10.0



using namespace MR;
using namespace App;
using namespace MR::Mesh;


const char* filters[] = { "smooth", NULL };


const OptionGroup smooth_option = OptionGroup ("Options for mesh smoothing filter")

    + Option ("smooth_spatial", "spatial extent of smoothing (default: " + str(DEFAULT_SMOOTHING_INFLUENCE, 2) + "mm)")
      + Argument ("value").type_float (0.0)

    + Option ("smooth_influence", "influence factor for smoothing (default: " + str(DEFAULT_SMOOTHING_INFLUENCE, 2) + ")")
      + Argument ("value").type_float (0.0);


void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "apply filter operations to meshes.";

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

    const float spatial   = get_option_value ("smooth_spatial",  DEFAULT_SMOOTHING_SPATIAL);
    const float influence = get_option_value ("smooth_inluence", DEFAULT_SMOOTHING_INFLUENCE);

    if (meshes.size() == 1) {
      meshes.front().smooth (spatial, influence);
    } else {
      std::mutex mutex;
      ProgressBar progress ("Applying smoothing filter to multiple meshes", meshes.size());
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
