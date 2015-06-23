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


      class ShaderBase : public GL::Shader::Program {
        public:
          ShaderBase() : GL::Shader::Program () { }
          virtual ~ShaderBase() { }

          virtual bool need_update (const Connectome&) const = 0;
          virtual void update (const Connectome&) = 0;

          void start (const Connectome& parent) {
            if (*this == 0 || need_update (parent)) {
              recompile (parent);
            }
            GL::Shader::Program::start();
          }

        protected:
          std::string vertex_shader_source, geometry_shader_source, fragment_shader_source;
          bool crop_to_slab, is_3D, use_lighting;
          float slab_thickness;

        private:
          void recompile (const Connectome& parent);
      };



      class NodeShader : public ShaderBase
      {
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
      {
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




