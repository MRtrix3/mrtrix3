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


#ifndef __gui_mrview_mode_volume_h__
#define __gui_mrview_mode_volume_h__

#include "app.h"
#include "math/versor.h"
#include "gui/mrview/mode/base.h"
#include "gui/opengl/transformation.h"

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
        { MEMALIGN(Volume)
          public:
            Volume () :
              Base (FocusContrast | MoveTarget | TiltRotate | ShaderTransparency | ShaderThreshold | ShaderClipping),
              volume_shader (*this) { 
              }

            virtual void paint (Projection& projection);
            virtual void slice_move_event (float x);
            virtual void pan_event ();
            virtual void panthrough_event ();
            virtual void tilt_event ();
            virtual void rotate_event ();

          protected:
            GL::VertexBuffer volume_VB, volume_VI;
            GL::VertexArrayObject volume_VAO;
            GL::Texture depth_texture;
            vector<GL::vec4> clip;

            class Shader : public Displayable::Shader { MEMALIGN(Shader)
              public:
                Shader (const Volume& mode) : mode (mode), active_clip_planes (0), cliphighlight (true), clipintersectionmode (false) { }
                virtual std::string vertex_shader_source (const Displayable& object);
                virtual std::string fragment_shader_source (const Displayable& object);
                virtual bool need_update (const Displayable& object) const;
                virtual void update (const Displayable& object);
                const Volume& mode;
                size_t active_clip_planes;
                bool cliphighlight;
                bool clipintersectionmode;
            } volume_shader;

            Tool::View* get_view_tool () const;
            vector< std::pair<GL::vec4,bool> > get_active_clip_planes () const;
            vector<GL::vec4*> get_clip_planes_to_be_edited () const;
            bool get_cliphighlightstate () const;
            bool get_clipintersectionmodestate () const;

            void move_clip_planes_in_out (vector<GL::vec4*>& clip, float distance);
            void rotate_clip_planes (vector<GL::vec4*>& clip, const Math::Versorf& rot);
        };

      }
    }
  }
}

#endif





