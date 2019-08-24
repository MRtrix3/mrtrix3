/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __gui_crosshair_h__
#define __gui_crosshair_h__

#include "types.h"

#include "gui/opengl/gl.h"
#include "gui/opengl/shader.h"

namespace MR
{
  namespace GUI
  {


    class ModelViewProjection;


    class Crosshair
    { NOMEMALIGN
      public:
          Crosshair() { }
          void render (const Eigen::Vector3f& focus,
                       const ModelViewProjection& MVP) const;
      protected:
        mutable GL::VertexBuffer VB;
        mutable GL::VertexArrayObject VAO;
        mutable GL::Shader::Program program;
    };




  }
}

#endif
