/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

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

#ifndef __gui_mrview_tool_connectome_types_h__
#define __gui_mrview_tool_connectome_types_h__

#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/lut.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

      typedef MR::DWI::Tractography::Connectomics::node_t    node_t;
      typedef MR::DWI::Tractography::Connectomics::Node_info Node_info;
      typedef MR::DWI::Tractography::Connectomics::Node_map  Node_map;

      enum node_geometry_t   { NODE_GEOM_SPHERE, NODE_GEOM_CUBE, NODE_GEOM_OVERLAY, NODE_GEOM_MESH, NODE_GEOM_SMOOTH_MESH };
      enum node_colour_t     { NODE_COLOUR_FIXED, NODE_COLOUR_RANDOM, NODE_COLOUR_LUT, NODE_COLOUR_FILE };
      enum node_size_t       { NODE_SIZE_FIXED, NODE_SIZE_VOLUME, NODE_SIZE_FILE };
      enum node_visibility_t { NODE_VIS_ALL, NODE_VIS_NONE, NODE_VIS_FILE, NODE_VIS_DEGREE };
      enum node_alpha_t      { NODE_ALPHA_FIXED, NODE_ALPHA_LUT, NODE_ALPHA_FILE };

      enum edge_geometry_t   { EDGE_GEOM_LINE, EDGE_GEOM_CYLINDER, EDGE_GEOM_EXEMPLAR };
      enum edge_colour_t     { EDGE_COLOUR_FIXED, EDGE_COLOUR_DIR, EDGE_COLOUR_FILE };
      enum edge_size_t       { EDGE_SIZE_FIXED, EDGE_SIZE_FILE };
      enum edge_visibility_t { EDGE_VIS_ALL, EDGE_VIS_NONE, EDGE_VIS_NODES, EDGE_VIS_FILE };
      enum edge_alpha_t      { EDGE_ALPHA_FIXED, EDGE_ALPHA_FILE };

      }
    }
  }
}

#endif




