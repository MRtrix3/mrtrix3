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



#include "app.h"
#include "file/path.h"
#include "file/utils.h"
#include "image/buffer.h"
#include "image/loop.h"
#include "image/transform.h"
#include "image/utils.h"
#include "image/voxel.h"
#include "image/interp/nearest.h"

#include "dwi/tractography/connectomics/connectomics.h"
using MR::DWI::Tractography::Connectomics::node_t;

#include <fstream>
#include <string>


#define AAL_LUT_PATH        "ROI_MNI_V4.txt"
#define FREESURFER_LUT_PATH "FreeSurferColorLUT.txt"

#define SPINE_NODE_NAME "Spinal_column"

#define MAX_NODE_INDEX (std::numeric_limits<node_t>::max())




MRTRIX_APPLICATION

using namespace MR;
using namespace App;


void usage ()
{

	AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

	DESCRIPTION
	+ "prepare the output from FreeSurfer segmentation for a connectomics analysis";

	ARGUMENTS
	+ Argument ("path_in",   "the input image").type_text()
	+ Argument ("config_in", "the MRtrix connectome configuration file specifying desired nodes & indices").type_file()
	+ Argument ("image_out", "the output image").type_image_out();


	OPTIONS
	+ Option ("freesurfer", "indicate that anatomical parcellation is from FreeSurfer, and provide the path to the "
	                        "FreeSurfer lookup table file (\"" FREESURFER_LUT_PATH "\")")
	  + Argument ("path").type_file()

	+ Option ("aal", "indicate that anatomical parcellation is from AAL, and provide the path to the "
                   "AAL lookup table file (\"" AAL_LUT_PATH "\")")
    + Argument ("path").type_file()

	+ Option ("spine", "provide a manually-defined segmentation of the base of the spine where the streamlines terminate, so that "
	                   "this can become a node in the connection matrix.")
	  + Argument ("image").type_image_in();

}



void run ()
{

  // Parse the parcellation file, getting indices and string names for nodes
  std::vector<std::string> in_nodes;

  Options opt = get_options ("freesurfer");
  if (opt.size()) {

    std::ifstream in_lut (opt[0][0].c_str(), std::ios_base::in);
    if (!in_lut)
      throw Exception ("Unable to open FreeSurfer lookup table file!");

    std::string line;
    char name [80];
    while (std::getline (in_lut, line)) {
      if (!line[0] != '#' && line.size() > 1) {
        node_t index = MAX_NODE_INDEX, r, g, b, a;
        sscanf (line.c_str(), "%u %s %u %u %u %u", &index, name, &r, &g, &b, &a);
        if (index != MAX_NODE_INDEX) {
          if (index >= in_nodes.size())
            in_nodes.resize (index + 1);
          in_nodes[index] = str(name);
        }
      }
      line.clear();
    }

  } else {

    opt = get_options ("aal");
    if (opt.size()) {

      std::ifstream in_lut (opt[0][0].c_str(), std::ios_base::in);
      if (!in_lut)
        throw Exception ("Unable to open AAL lookup table file!");

      std::string line;
      char short_name[20], name [80];
      while (std::getline (in_lut, line)) {
        if (!line[0] != '#' && line.size() > 1) {
          node_t index = MAX_NODE_INDEX;
          sscanf (line.c_str(), "%s %s %u", short_name, name, &index);
          if (index != MAX_NODE_INDEX) {
            if (index >= in_nodes.size())
              in_nodes.resize (index + 1);
            in_nodes[index] = str(name);
          }
        }
        line.clear();
      }

    } else {
      throw Exception ("Must provide either -freesurfer or -aal option (no other parcellation types currently supported)");
    }

  }

  // Import the configuration file
  if (!Path::exists (argument[1]))
    throw Exception ("Cannot find input configuration file!");
  std::ifstream in_config (argument[1].c_str(), std::ios_base::in);
  if (!in_config)
    throw Exception ("Unable to open configuration file!");

  // Configuration file should contain node index, followed by structure name
  // Name should be identical to that in relevant lookup table file e.g. FreeSurferColorLUT.txt, ROI_MNI_v4.txt
  // However, when importing, these are inverted; map structure name to desired node index
  std::map<std::string, node_t> inv_lookup;
  std::string line;
  char name [80];
  while (std::getline (in_config, line)) {
    if (!line[0] != '#' && line.size() > 1) {
      node_t index = MAX_NODE_INDEX;
      sscanf (line.c_str(), "%u %s", &index, name);
      if (index != MAX_NODE_INDEX)
        inv_lookup.insert (std::make_pair (str(name), index));
    }
  }

  // Create the look-up table (just a vector) to go from input index to output index
  std::vector<node_t> lookup (in_nodes.size(), 0);
  for (node_t in_index = 0; in_index != in_nodes.size(); ++in_index) {
    const std::string& name = in_nodes[in_index];
    if (!name.empty()) {
      const std::map<std::string, node_t>::const_iterator out_index = inv_lookup.find (name);
      if (out_index != inv_lookup.end())
        lookup[in_index] = out_index->second;
    }
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
  opt = get_options ("spine");
  if (opt.size()) {

    const std::map<std::string, node_t>::const_iterator find_spine_node_index = inv_lookup.find (str (SPINE_NODE_NAME));
    if (find_spine_node_index != inv_lookup.end()) {
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

  } else if (inv_lookup.find (SPINE_NODE_NAME) != inv_lookup.end()) {
    WARN ("Config file includes \"" + str (SPINE_NODE_NAME) + "\" node, but user has not provided the segmentation using -spine option");
  }


}

