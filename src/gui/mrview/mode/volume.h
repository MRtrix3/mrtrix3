/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __gui_mrview_mode_volume_h__
#define __gui_mrview_mode_volume_h__

#include "app.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool { class View; }



      namespace Mode
      {

        class Volume : public Base
        {
          public:
            Volume (Window& parent) :
              Base (parent, FocusContrast | MoveTarget | TiltRotate | ShaderTransparency | ShaderThreshold | ShaderClipping),
              volume_shader (*this) { 
              }

            virtual void paint (Projection& projection);
            virtual void slice_move_event (int x);
            virtual void pan_event ();
            virtual void panthrough_event ();
            virtual void tilt_event ();
            virtual void rotate_event ();

          protected:
            GL::VertexBuffer volume_VB, volume_VI;
            GL::VertexArrayObject volume_VAO;
            GL::Texture depth_texture;
            std::vector<GL::vec4> clip;

            class Shader : public Displayable::Shader {
              public:
                Shader (const Volume& mode) : mode (mode), active_clip_planes (0) { }
                virtual std::string vertex_shader_source (const Displayable& object);
                virtual std::string fragment_shader_source (const Displayable& object);
                virtual bool need_update (const Displayable& object) const;
                virtual void update (const Displayable& object);
                const Volume& mode;
                size_t active_clip_planes;
            } volume_shader;

            Tool::View* get_view_tool () const;
            std::vector< std::pair<GL::vec4,bool> > get_active_clip_planes () const;
            std::vector<GL::vec4*> get_clip_planes_to_be_edited () const;


            void move_clip_planes_in_out (std::vector<GL::vec4*>& clip, float distance);
            void rotate_clip_planes (std::vector<GL::vec4*>& clip, const Math::Versor<float>& rot);
        };

      }
    }
  }
}

#endif





