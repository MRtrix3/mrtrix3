/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 2014.

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

#ifndef __gui_mrview_tool_roi_analysis_undoentry_h__
#define __gui_mrview_tool_roi_analysis_undoentry_h__

#include <array>
#include <vector>

#include "point.h"

#include "gui/opengl/shader.h"
#include "gui/opengl/gl.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class ROI_Item;


        struct ROI_UndoEntry {

          ROI_UndoEntry (ROI_Item&, int, int);
          void draw_line (ROI_Item&, const Point<>&, const Point<>&, bool);
          void draw_circle (ROI_Item&, const Point<>&, bool, float);
          void draw_rectangle (ROI_Item&, const Point<>&, const Point<>&, bool);
          void draw_fill (ROI_Item&, const Point<>, bool);

          void undo (ROI_Item& roi);
          void redo (ROI_Item& roi);
          void copy (ROI_Item& roi, ROI_UndoEntry& source);

          std::array<GLint,3> from, size;
          std::array<GLint,2> tex_size, slice_axes;
          std::vector<GLubyte> before, after;

          static GL::Shader::Program copy_program;
          static GL::VertexBuffer copy_vertex_buffer;
          static GL::VertexArrayObject copy_vertex_array_object;
        };





      }
    }
  }
}

#endif


