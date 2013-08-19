/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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



#include "dwi/tractography/connectomics/lut.h"





namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectomics {



using namespace App;



const OptionGroup LookupTableOption = OptionGroup ("Options for importing information from parcellation lookup tables")

  + Option ("lut_basic", "get information from a basic lookup table consisting of index / name pairs")
    + Argument("path").type_file()

  + Option ("lut_freesurfer", "get information from a FreeSurfer lookup table (typically \"FreeSurferColorLUT.txt\")")
    + Argument("path").type_file()

  + Option ("lut_aal", "get information from the AAL lookup table (typically \"ROI_MNI_V4.txt\")")
    + Argument("path").type_file()

  + Option ("lut_itksnap", "get information from an ITK-SNAP lookup table (this includes the IIT atlas file \"LUT_GM.txt\")")
    + Argument("path").type_file();

  // TODO Add more e.g. FSL
  // FSL's HarvardOxford atlas has identical labels across left and right hemispheres...







void load_lookup_table (Node_map& nodes)
{


  const node_t max_node_index = std::numeric_limits<node_t>::max();


  Options opt = get_options ("lut_basic");
  if (opt.size()) {

    if (nodes.size())
      throw Exception ("Cannot import lookup table information from multiple sources");

    std::ifstream in_lut (opt[0][0].c_str(), std::ios_base::in);
    if (!in_lut)
      throw Exception ("Unable to open lookup table file");

    std::string line;
    char name [80];
    while (std::getline (in_lut, line)) {
      if (line[0] != '#' && line.size() > 1) {
        node_t index = max_node_index;
        sscanf (line.c_str(), "%u %s", &index, name);
        if (index != max_node_index) {
          if (nodes.find (index) != nodes.end())
            throw Exception ("Lookup table " + opt[0][0] + " contains redundant entries");
          const std::string strname (name);
          nodes.insert (std::make_pair (index, Node_info (strname)));
        }
      }
      line.clear();
    }

  }


  Options opt = get_options ("lut_freesurfer");
  if (opt.size()) {

    if (nodes.size())
      throw Exception ("Cannot import lookup table information from multiple sources");

    std::ifstream in_lut (opt[0][0].c_str(), std::ios_base::in);
    if (!in_lut)
      throw Exception ("Unable to open FreeSurfer lookup table file");

    std::string line;
    char name [80];
    while (std::getline (in_lut, line)) {
      if (line[0] != '#' && line.size() > 1) {
        node_t index = max_node_index;
        node_t r = 256, g = 256, b = 256, a = 255;
        sscanf (line.c_str(), "%u %s %u %u %u %u", &index, name, &r, &g, &b, &a);
        if (index != max_node_index) {
          if (maxvalue (r, g, b) > 255)
            throw Exception ("Lookup table " + opt[0][0] + " is malformed");
          if (nodes.find (index) != nodes.end())
            throw Exception ("Lookup table " + opt[0][0] + " contains redundant entries");
          const std::string strname (name);
          nodes.insert (std::make_pair (index, Node_info (strname, r, g, b, a)));
        }
      }
      line.clear();
    }

  }


  opt = get_options ("lut_aal");
  if (opt.size()) {

    if (nodes.size())
      throw Exception ("Cannot import lookup table information from multiple sources");

    std::ifstream in_lut (opt[0][0].c_str(), std::ios_base::in);
    if (!in_lut)
      throw Exception ("Unable to open AAL lookup table file");

    std::string line;
    char short_name[20], name [80];
    while (std::getline (in_lut, line)) {
      if (line[0] != '#' && line.size() > 1) {
        node_t index = max_node_index;
        sscanf (line.c_str(), "%s %s %u", short_name, name, &index);
        if (index != max_node_index) {
          if (nodes.find (index) != nodes.end())
            throw Exception ("Lookup table " + opt[0][0] + " contains redundant entries");
          const std::string strname (name);
          nodes.insert (std::make_pair (index, Node_info (strname)));
        }
      }
      line.clear();
    }

  }


  opt = get_options ("lut_itksnap");
  if (opt.size()) {

    if (nodes.size())
      throw Exception ("Cannot import lookup table information from multiple sources");

    std::ifstream in_lut (opt[0][0].c_str(), std::ios_base::in);
    if (!in_lut)
      throw Exception ("Unable to open ITK-SNAP lookup table file");

    std::string line;
    char name [80];
    while (std::getline (in_lut, line)) {
      if (line[0] != '#' && line.size() > 1) {
        node_t index = max_node_index;
        node_t r = 256, g = 256, b = 256;
        float a = 1.0;
        unsigned int label_vis = 0, mesh_vis = 0;
        sscanf (line.c_str(), "%u %u %u %u %f %u %u %s", &index, &r, &g, &b, &a, &label_vis, &mesh_vis, name);
        if (index != max_node_index) {
          if (nodes.find (index) != nodes.end())
            throw Exception ("Lookup table " + opt[0][0] + " contains redundant entries");
          std::string strname (name);
          size_t first = strname.find_first_not_of ('\"');
          if (first == std::string::npos)
            first = 0;
          size_t last = strname.find_last_not_of ('\"');
          if (last == std::string::npos)
            last = strname.size() - 1;
          strname = strname.substr (first, last - first + 1);
          VAR (index);
          VAR (strname);
          nodes.insert (std::make_pair (index, Node_info (strname, r, g, b, uint8_t(a*255.0))));
        }
      }
      line.clear();
    }

  }

}








}
}
}
}



