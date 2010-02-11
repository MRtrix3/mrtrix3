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

#include "opengl/gl.h"
#include "mrview/glarea.h"
#include "mrview/mode/base.h"


namespace MR {
  namespace Viewer {

    GLArea::GLArea (QWidget *parent) : 
      QGLWidget (QGLFormat (QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba), parent),
      mode (Mode::create (*this, 0))
    { }

    GLArea::~GLArea () { delete mode; }

    QSize GLArea::minimumSizeHint() const { return QSize (256, 256); }
    QSize GLArea::sizeHint() const { return QSize (256, 256); }

    void GLArea::initializeGL () 
    {
      GL::init ();

      CHECK_GL_EXTENSION (ARB_fragment_shader);
      CHECK_GL_EXTENSION (ARB_vertex_shader);
      CHECK_GL_EXTENSION (ARB_geometry_shader4);
      CHECK_GL_EXTENSION (EXT_texture3D);
      CHECK_GL_EXTENSION (ARB_texture_non_power_of_two);

      glClearColor (0.0, 0.0, 0.0, 0.0);
      glEnable (GL_DEPTH_TEST);
    }

    void GLArea::paintGL () 
    {
      glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glLoadIdentity();
      mode->paint();
    }

    void GLArea::resizeGL (int width, int height) 
    {
      glViewport (0, 0, width, height);
    }

    void GLArea::mousePressEvent (QMouseEvent *event) { mode->mousePressEvent (event); }

    void GLArea::mouseMoveEvent (QMouseEvent *event) { mode->mouseMoveEvent (event); }

    void GLArea::mouseDoubleClickEvent (QMouseEvent* event) { mode->mouseDoubleClickEvent (event); }
    void GLArea::mouseReleaseEvent (QMouseEvent* event) { mode->mouseReleaseEvent (event); }
    void GLArea::wheelEvent (QWheelEvent* event) { mode->wheelEvent (event); }

  }
}


