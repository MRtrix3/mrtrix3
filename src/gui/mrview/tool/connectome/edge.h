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

#ifndef __gui_mrview_tool_connectome_edge_h__
#define __gui_mrview_tool_connectome_edge_h__

#include "point.h"

#include "dwi/tractography/connectomics/connectomics.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

      class Connectome;

      // Stores all information relating to the drawing of individual edges, both fixed and variable
      // Try to store more than would otherwise be optimal in here, in order to simplify the drawing process
      class Edge
      {

          typedef MR::DWI::Tractography::Connectomics::node_t node_t;

        public:
          Edge (const node_t, const node_t, const Point<float>&, const Point<float>&);
          Edge (Edge&&);
          Edge ();
          ~Edge();

          void render_line() const;

          node_t get_node_index (const size_t i) const { assert (i==0 || i==1); return node_indices[i]; }
          const Point<float> get_node_centre (const size_t i) const { assert (i==0 || i==1); return node_centres[i]; }
          Point<float> get_com() const { return (node_centres[0] + node_centres[1]) * 0.5; }

          const GLfloat* get_rot_matrix() const { return rot_matrix; }

          const Point<float>& get_dir() const { return dir; }
          void set_size (const float i) { size = i; }
          float get_size() const { return size; }
          void set_colour (const Point<float>& i) { colour = i; }
          const Point<float>& get_colour() const { return colour; }
          void set_alpha (const float i) { alpha = i; }
          float get_alpha() const { return alpha; }
          void set_visible (const bool i) { visible = i; }
          bool is_visible() const { return visible; }
          bool is_diagonal() const { return (node_indices[0] == node_indices[1]); }

        private:
          const node_t node_indices[2];
          const Point<float> node_centres[2];
          const Point<float> dir;

          GLfloat* rot_matrix;

          float size;
          Point<float> colour;
          float alpha;
          bool visible;

      };



      }
    }
  }
}

#endif




