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

#ifndef __gui_mrview_tool_connectome_nodeoverlay_h__
#define __gui_mrview_tool_connectome_nodeoverlay_h__

#include "header.h"
#include "types.h"

#include "gui/mrview/displayable.h"
#include "gui/mrview/gui_image.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

      // Class to handle the node image overlay
      class NodeOverlay : public MR::GUI::MRView::ImageBase
      {
        public:
          NodeOverlay (MR::Header&&);

          void update_texture2D (const int, const int) override;
          void update_texture3D() override;

          MR::Image<float> data;

        private:
          bool need_update;

        public:
          class Shader : public Displayable::Shader {
            public:
            virtual std::string vertex_shader_source (const Displayable&);
            virtual std::string fragment_shader_source (const Displayable&);
          } slice_shader;
      };


      }
    }
  }
}

#endif




