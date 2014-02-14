/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

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



#include "command.h"
#include "file/path.h"
#include "file/utils.h"
#include "image/buffer.h"
#include "image/loop.h"
#include "image/transform.h"
#include "image/utils.h"
#include "image/voxel.h"
#include "image/interp/nearest.h"

#include "dwi/tractography/connectomics/config.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/lut.h"


#include <fstream>
#include <map>
#include <string>
#include <vector>


#define SPINE_NODE_NAME "Spinal_column"




using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography::Connectomics;
using MR::DWI::Tractography::Connectomics::node_t;



void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

	DESCRIPTION
	+ "prepare a parcellated image for connectome construction by modifying the image values; "
	  "typically this involves making the parcellation intensities increment from 1 to coincide with rows and columns of a matrix. "
	  "The configuration file passed as the second argument specifies the indices that should be assigned to different structures; "
	  "examples of such configuration files are provided in src//dwi//tractography//connectomics//example_configs//";


	ARGUMENTS
	+ Argument ("path_in",   "the input image").type_text()
	+ Argument ("config_in", "the MRtrix connectome configuration file specifying desired nodes & indices").type_file()
	+ Argument ("image_out", "the output image").type_image_out();


	OPTIONS
	+ LookupTableOption

	+ Option ("spine", "provide a manually-defined segmentation of the base of the spine where the streamlines terminate, so that "
	                   "this can become a node in the connection matrix.")
	  + Argument ("image").type_image_in();

}



void run ()
{

  // Load the lookup table - need this info to match config file structure names to indices in the input image
  Node_map in_nodes;
  load_lookup_table (in_nodes);
  if (in_nodes.empty())
    throw Exception ("Must provide the lookup table corresponding to the input image parcellation");

  // Import the configuration file
  ConfigInvLookup config;
  load_config (argument[1], config);

  // Create the look-up table (just a vector) to go from input index to output index
  // First, need the largest index expected
  const node_t max_in_index = in_nodes.rbegin()->first;
  std::vector<node_t> lookup (max_in_index + 1, 0);
  for (Node_map::const_iterator i = in_nodes.begin(); i != in_nodes.end(); ++i) {
    const node_t in_index = i->first;
    const std::string& name = i->second.get_name();
    const ConfigInvLookup::const_iterator out_index = config.find (name);
    if (out_index != config.end())
      lookup[in_index] = out_index->second;
  }

  // Open the input file
  Image::Buffer<node_t> in_data (argument[0]);
  Image::Buffer<node_t>::voxel_type in (in_data);

  // Create a new header for the output file
  Image::Header H (in_data);
  H.comments().push_back ("Created by mrprep4connectome");
  H.comments().push_back ("Basis image: " + Path::basename (argument[0]));
  H.comments().push_back ("Configuration file: " + Path::basename (argument[1]));

  // Create the output file
  Image::Buffer<node_t> out_data (argument[2], H);
  Image::Buffer<node_t>::voxel_type out (out_data);

  // Fill the output image with data
  Image::Loop loop;
  for (loop.start (in, out); loop.ok(); loop.next (in, out))
    out.value() = lookup[in.value()];

  // If the spine segment option has been provided, add this retrospectively
  Options opt = get_options ("spine");
  if (opt.size()) {

    const ConfigInvLookup::const_iterator find_spine_node_index = config.find (str (SPINE_NODE_NAME));
    if (find_spine_node_index != config.end()) {
      const node_t spine_node_index = find_spine_node_index->second;

      Image::Buffer<bool> in_spine_data (opt[0][0]);
      Image::Buffer<bool>::voxel_type in_spine (in_spine_data);

      if (dimensions_match (in_spine, out)) {

        for (loop.start (in_spine, out); loop.ok(); loop.next (in_spine, out)) {
          if (in_spine.value())
            out.value() = spine_node_index;
        }

      } else {

        WARN ("Spine node is being created from the mask image provided using -spine option using nearest-neighbour interpolation;");
        WARN ("recommend using the parcellation image as the basis for this mask so that interpolation is not required");

        Image::Transform transform (out);
        Image::Interp::Nearest< Image::Buffer<bool>::voxel_type > nearest (in_spine);
        for (loop.start (out); loop.ok(); loop.next (out)) {
          const Point<float> p (transform.voxel2scanner (out));
          if (!nearest.scanner (p) && nearest.value())
            out.value() = spine_node_index;
        }

      }

    } else {
      WARN ("Could not add spine node; need to specify \"" + str(SPINE_NODE_NAME) + "\" node in config file");
    }

  } else if (config.find (SPINE_NODE_NAME) != config.end()) {
    WARN ("Config file includes \"" + str (SPINE_NODE_NAME) + "\" node, but user has not provided the segmentation using -spine option");
  }


}

