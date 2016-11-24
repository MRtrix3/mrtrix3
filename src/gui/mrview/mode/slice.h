/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __gui_mrview_mode_slice_h__
#define __gui_mrview_mode_slice_h__

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

        class Slice : public Base
        { MEMALIGN(Slice)
          public:
            Slice () :
              Base (FocusContrast | MoveTarget | TiltRotate) { }
            virtual ~Slice ();

            virtual void paint (Projection& with_projection);

            class Shader : public Displayable::Shader { MEMALIGN(Shader)
              public:
                virtual std::string vertex_shader_source (const Displayable& object);
                virtual std::string fragment_shader_source (const Displayable& object);
            } slice_shader;

          protected:
            virtual void draw_plane_primitive (int axis, Displayable::Shader& shader_program, Projection& with_projection);
            void draw_plane (int axis, Displayable::Shader& shader_program, Projection& with_projection);
        };

      }
    }
  }
}

#endif




