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

#ifndef __gui_mrview_tool_connectome_shaders_h__
#define __gui_mrview_tool_connectome_shaders_h__

#include "gui/opengl/shader.h"

#include "gui/mrview/tool/connectome/types.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


      class Connectome;


      class ShaderBase : public GL::Shader::Program { MEMALIGN(ShaderBase)
        public:
          ShaderBase() : GL::Shader::Program () { }
          virtual ~ShaderBase() { }

          virtual bool need_update (const Connectome&) const = 0;
          virtual void update (const Connectome&) = 0;

          void start (const Connectome& parent) {
            GL::assert_context_is_current();
            if (*this == 0 || need_update (parent)) {
              recompile (parent);
            }
            GL::Shader::Program::start();
            GL::assert_context_is_current();
          }

        protected:
          std::string vertex_shader_source, geometry_shader_source, fragment_shader_source;
          bool crop_to_slab, is_3D, use_lighting;
          float slab_thickness;

        private:
          void recompile (const Connectome& parent);
      };



      class NodeShader : public ShaderBase
      { MEMALIGN(NodeShader)
        public:
          NodeShader() : ShaderBase () { }
          ~NodeShader() { }
          bool need_update (const Connectome&) const override;
          void update (const Connectome&) override;
        private:
          node_geometry_t geometry;
          node_colour_t colour;
          size_t colourmap_index;
          bool use_alpha;
      };



      class EdgeShader : public ShaderBase
      { MEMALIGN(EdgeShader)
        public:
          EdgeShader() : ShaderBase () { }
          ~EdgeShader() { }
          bool need_update (const Connectome&) const override;
          void update (const Connectome&) override;
        private:
          edge_geometry_t geometry;
          edge_colour_t colour;
          size_t colourmap_index;
          bool use_alpha;
      };



      }
    }
  }
}

#endif




