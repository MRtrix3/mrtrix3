/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
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




