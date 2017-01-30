/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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
      { MEMALIGN(NodeOverlay)
        public:
          NodeOverlay (MR::Header&&);

          void update_texture2D (const int, const int) override;
          void update_texture3D() override;

          MR::Image<float> data;

        private:
          bool need_update;

        public:
          class Shader : public Displayable::Shader { MEMALIGN(Shader)
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




