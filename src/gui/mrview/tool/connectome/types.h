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

#include "connectome/connectome.h"
#include "connectome/lut.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

      typedef MR::Connectome::node_t    node_t;
      typedef MR::Connectome::Node_info Node_info;
      typedef MR::Connectome::Node_map  Node_map;

      enum class node_visibility_t { ALL, NONE, DEGREE, VECTOR_FILE, MATRIX_FILE };
      enum class node_geometry_t   { SPHERE, CUBE, OVERLAY, MESH };
      enum class node_colour_t     { FIXED, RANDOM, FROM_LUT, VECTOR_FILE, MATRIX_FILE };
      enum class node_size_t       { FIXED, NODE_VOLUME, VECTOR_FILE, MATRIX_FILE };
      enum class node_alpha_t      { FIXED, FROM_LUT, VECTOR_FILE, MATRIX_FILE };

      enum class edge_visibility_t { ALL, NONE, VISIBLE_NODES, MATRIX_FILE };
      enum class edge_geometry_t   { LINE, CYLINDER, STREAMLINE, STREAMTUBE };
      enum class edge_colour_t     { FIXED, DIRECTION, MATRIX_FILE };
      enum class edge_size_t       { FIXED, MATRIX_FILE };
      enum class edge_alpha_t      { FIXED, MATRIX_FILE };

      }
    }
  }
}

#endif




