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
      namespace Mode
      {

        class Volume : public Base
        {
          public:
            Volume (Window& parent) :
              Base (parent, FocusContrast | MoveTarget | TiltRotate | ShaderTransparency | ShaderThreshold | ShaderClipping),
              volume_shader (*this) { 
                reset_clip_planes();
              }

            void reset_clip_planes();
            void invert_clip_plane (int n);

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
            GL::vec4 clip[3];

            class Shader : public Displayable::Shader {
              public:
                Shader (const Volume& mode) : mode (mode) { clip[0] = clip[1] = clip[2] = false; }
                virtual std::string vertex_shader_source (const Displayable& object);
                virtual std::string fragment_shader_source (const Displayable& object);
                virtual bool need_update (const Displayable& object) const;
                virtual void update (const Displayable& object);
                const Volume& mode;
                bool clip[3];
            } volume_shader;

            bool do_clip (int n) const;
            bool edit_clip (int n) const;
            bool editing () const;


            void move_clip_planes_in_out (float distance);
            void rotate_clip_planes (const Math::Versor<float>& rot);
        };

      }
    }
  }
}

#endif





