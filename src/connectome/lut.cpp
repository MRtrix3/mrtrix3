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



#include "connectome/lut.h"

#include <fstream>



namespace MR {
namespace Connectome {



using namespace App;



const char* lut_format_strings[] = { "No LUT", "Index / name pairs", "FreeSurfer", "AAL", "ITK-SNAP", NULL };



const OptionGroup LookupTableOption = OptionGroup ("Options for importing information from parcellation lookup tables")

  + Option ("lut_basic", "get information from a basic lookup table consisting of index / name pairs")
    + Argument("path").type_file_in()

  + Option ("lut_freesurfer", "get information from a FreeSurfer lookup table (typically \"FreeSurferColorLUT.txt\")")
    + Argument("path").type_file_in()

  + Option ("lut_aal", "get information from the AAL lookup table (typically \"ROI_MNI_V4.txt\")")
    + Argument("path").type_file_in()

  + Option ("lut_itksnap", "get information from an ITK-SNAP lookup table (this includes the IIT atlas file \"LUT_GM.txt\")")
    + Argument("path").type_file_in();

  // TODO Add more e.g. FSL
  // FSL's HarvardOxford atlas has identical labels across left and right hemispheres...







void load_lut_from_cmdline (Node_map& nodes)
{

  auto
  opt = get_options ("lut_basic");
  if (opt.size())
    nodes.load (opt[0][0], LUT_BASIC);

  opt = get_options ("lut_freesurfer");
  if (opt.size())
    nodes.load (opt[0][0], LUT_FREESURFER);

  opt = get_options ("lut_aal");
  if (opt.size())
    nodes.load (opt[0][0], LUT_AAL);

  opt = get_options ("lut_itksnap");
  if (opt.size())
    nodes.load (opt[0][0], LUT_ITKSNAP);

}



void Node_map::load (const std::string& path, const lut_format format)
{
  if (size())
    throw Exception ("Cannot import lookup table information from multiple sources");
  std::ifstream in_lut (path, std::ios_base::in);
  if (!in_lut)
    throw Exception ("Unable to open lookup table file");
  std::string line;
  while (std::getline (in_lut, line)) {
    switch (format) {
      case LUT_BASIC:      parse_line_basic      (line); break;
      case LUT_FREESURFER: parse_line_freesurfer (line); break;
      case LUT_AAL:        parse_line_aal        (line); break;
      case LUT_ITKSNAP:    parse_line_itksnap    (line); break;
      default: assert (0);
    }
    line.clear();
  }
}


void Node_map::parse_line_basic (const std::string& line)
{
  if (line.size() > 1 && line[0] != '#') {
    node_t index = std::numeric_limits<node_t>::max();
    char name [80];
    sscanf (line.c_str(), "%u %s", &index, name);
    if (index != std::numeric_limits<node_t>::max()) {
      if (find (index) != end())
        throw Exception ("Lookup table contains redundant entries (" + str(index) + ")");
      const std::string strname (name);
      insert (std::make_pair (index, Node_info (strname)));
    }
  }
}
void Node_map::parse_line_freesurfer (const std::string& line)
{
  if (line.size() > 1 && line[0] != '#') {
    node_t index = std::numeric_limits<node_t>::max();
    node_t r = 256, g = 256, b = 256, a = 255;
    char name [80];
    sscanf (line.c_str(), "%u %s %u %u %u %u", &index, name, &r, &g, &b, &a);
    if (index != std::numeric_limits<node_t>::max()) {
      if (std::max ({r, g, b}) > 255)
        throw Exception ("Lookup table is malformed");
      if (find (index) != end())
        throw Exception ("Lookup table contains redundant entries (" + str(index) + ")");
      const std::string strname (name);
      insert (std::make_pair (index, Node_info (strname, r, g, b, a)));
    }
  }
}
void Node_map::parse_line_aal (const std::string& line)
{
  if (line.size() > 1 && line[0] != '#') {
    node_t index = std::numeric_limits<node_t>::max();
    char short_name[20], name [80];
    sscanf (line.c_str(), "%s %s %u", short_name, name, &index);
    if (index != std::numeric_limits<node_t>::max()) {
      if (find (index) != end())
        throw Exception ("Lookup table contains redundant entries (" + str(index) + ")");
      const std::string strname (name);
      insert (std::make_pair (index, Node_info (strname)));
    }
  }
}
void Node_map::parse_line_itksnap (const std::string& line)
{
  if (line.size() > 1 && line[0] != '#') {
    node_t index = std::numeric_limits<node_t>::max();
    node_t r = 256, g = 256, b = 256;
    float a = 1.0;
    unsigned int label_vis = 0, mesh_vis = 0;
    char name [80];
    sscanf (line.c_str(), "%u %u %u %u %f %u %u %s", &index, &r, &g, &b, &a, &label_vis, &mesh_vis, name);
    if (index != std::numeric_limits<node_t>::max()) {
      if (find (index) != end())
        throw Exception ("Lookup table contains redundant entries (" + str(index) + ")");
      std::string strname (name);
      size_t first = strname.find_first_not_of ('\"');
      if (first == std::string::npos)
        first = 0;
      size_t last = strname.find_last_not_of ('\"');
      if (last == std::string::npos)
        last = strname.size() - 1;
      strname = strname.substr (first, last - first + 1);
      insert (std::make_pair (index, Node_info (strname, r, g, b, uint8_t(a*255.0))));
    }
  }
}





}
}



