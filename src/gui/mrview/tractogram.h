/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier & David Raffelt, 17/12/12.

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

#ifndef __gui_mrview_tool_tractogram_h__
#define __gui_mrview_tool_tractogram_h__

#include <QAction>
#include <string>

// necessary to avoid conflict with Qt4's foreach macro:
#ifdef foreach
# undef foreach
#endif

#include "gui/mrview/displayable.h"
#include "gui/mrview/shader.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/file.h"
#include "gui/opengl/shader.h"
#include "gui/mrview/tool/tractography.h"

namespace MR
{
  class ProgressBar;

  namespace GUI
  {
    class Projection;

    namespace MRView
    {
      class Window;

      namespace Tool
      {

        class Tractogram : public Displayable
        {
          Q_OBJECT

          public:
//            Tractogram (const std::string& filename);
            Tractogram (const std::string& filename, Tractography& parent);

            ~Tractogram ();

            void render2D (const Projection& transform);

            void render3D ();

          signals:
            void scalingChanged ();

          private:
            Tractography& parent_tool_window;
            std::string filename;
            std::vector<GLuint> vertex_buffers;
            DWI::Tractography::Reader<float> file;
            DWI::Tractography::Properties properties;
            std::vector<std::vector<GLint> > track_starts;
            std::vector<std::vector<GLint> > track_sizes;
            std::vector<size_t> num_tracks_per_buffer;
            GL::Shader::Program shader;
            GLuint VertexArrayID;
            bool use_default_line_thickness;
            float line_thickness;

            void set_color () {
            }

            void load_tracks();

        };

      }
    }
  }
}

#endif

