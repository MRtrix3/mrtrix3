/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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


    24-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * scale plot by SH(0,0) coefficient when normalise option is set

*/

#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>

#include <fstream>

#include "app.h"
#include "gui/disp_profile/render_frame.h"

#define ROTATION_INC 0.004
#define D2R 0.01745329252

#define DIST_INC 0.005
#define DIST_MIN 0.1
#define DIST_MAX 10.0

#define SCALE_INC 1.05
#define SCALE_MIN 0.01
#define SCALE_MAX 10.0

#define ANGLE_INC 0.1
#define ANGLE_MIN 1.0
#define ANGLE_MAX 90.0

namespace MR
{
  namespace GUI
  {
    namespace DWI
    {

      RenderFrame::RenderFrame (QWidget* parent) :
        QGLWidget (QGLFormat (QGL::FormatOptions (QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba)), parent),
        view_angle (40.0), distance (0.3), line_width (1.0), scale (1.0), l0_term (NAN),
        show_axes (true), color_by_dir (true), use_lighting (true), projection (this),
        focus (0.0, 0.0, 0.0), framebuffer (NULL), OS (0), OS_x (0), OS_y (0)
      {
        lighting = new GL::Lighting (this);
        lighting->set_background = true;
        connect (lighting, SIGNAL (changed()), this, SLOT (updateGL()));
      }



      void RenderFrame::set_rotation (const GLdouble* rotation)
      {
        float p[9];
        p[0] = rotation[0]; p[1] = rotation[1]; p[2] = rotation[2];
        p[3] = rotation[4]; p[4] = rotation[5]; p[5] = rotation[6];
        p[6] = rotation[8]; p[7] = rotation[9]; p[8] = rotation[10];
        Math::Matrix<float> M (p, 3, 3);
        orientation.from_matrix (M);
        updateGL();
      }



      void RenderFrame::initializeGL ()
      {
        GL::init();
        renderer.init();
        glEnable (GL_DEPTH_TEST);
        lighting->set();
        INFO ("DWI renderer successfully initialised");
      }



      void RenderFrame::resizeGL (int w, int h)
      {
        glViewport (0, 0, w, h);
      }



      void RenderFrame::paintGL ()
      {
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float dist (1.0 / (distance * view_angle * D2R));
        float near = (dist-3.0 > 0.001 ? dist-3.0 : 0.001);
        float horizontal = 2.0 * near * tan (0.5*view_angle*D2R) * float (width()) / float (width()+height());
        float vertical = 2.0 * near * tan (0.5*view_angle*D2R) * float (height()) / float (width()+height());

        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        if (OS > 0) {
          float incx = 2.0*horizontal/float (OS);
          float incy = 2.0*vertical/float (OS);
          glFrustum (-horizontal+OS_x*incx, -horizontal+ (1+OS_x) *incx, -vertical+OS_y*incy, -vertical+ (1+OS_y) *incy, near, dist+3.0);
        }
        else glFrustum (-horizontal, horizontal, -vertical, vertical, near, dist+3.0);
        glMatrixMode (GL_MODELVIEW);

        glLoadIdentity ();
        lighting->set();

        glTranslatef (0.0, 0.0, -dist);

        float T[16];
        Math::Matrix<float> M (T, 3, 3, 4);
        orientation.to_matrix (M);
        T[3] = T[7] = T[11] = T[12] = T[13] = T[14] = 0.0;
        T[15] = 1.0;
        glMultMatrixf (T);

        glTranslatef (focus[0], focus[1], focus[2]);

        glDepthMask (GL_TRUE);

        projection.update();

        if (finite (l0_term)) {
          glPushMatrix();
          glDisable (GL_BLEND);

          if (use_lighting) glEnable (GL_LIGHTING);

          glScalef (scale, scale, scale);

          renderer.draw (use_lighting, color_by_dir ? NULL : lighting->object_color);

          if (use_lighting) glDisable (GL_LIGHTING);

          glPopMatrix();
        }

        glLineWidth (line_width);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable (GL_BLEND);
        glEnable (GL_LINE_SMOOTH);
        glDisable (GL_MULTISAMPLE);

        if (show_axes) {
          glColor3f (1.0, 0.0, 0.0);
          glBegin (GL_LINES);
          glVertex3f (-1.0, -1.0, -1.0);
          glVertex3f (1.0, -1.0, -1.0);
          glEnd();

          glColor3f (0.0, 1.0, 0.0);
          glBegin (GL_LINES);
          glVertex3f (-1.0, -1.0, -1.0);
          glVertex3f (-1.0, 1.0, -1.0);
          glEnd ();

          glColor3f (0.0, 0.0, 1.0);
          glBegin (GL_LINES);
          glVertex3f (-1.0, -1.0, -1.0);
          glVertex3f (-1.0, -1.0, 1.0);
          glEnd ();
        }

        glDisable (GL_BLEND);
        glDisable (GL_LINE_SMOOTH);
        glEnable (GL_MULTISAMPLE);



        if (OS > 0) snapshot();

        DEBUG_OPENGL;

      }








      void RenderFrame::mouseDoubleClickEvent (QMouseEvent* event)
      {
        if (event->modifiers() == Qt::NoModifier) {
          if (event->buttons() == Qt::LeftButton) {
            orientation = Math::Versor<float>();
            updateGL();
          }
          else if (event->buttons() == Qt::MidButton) {
            focus.set (0.0, 0.0, 0.0);
            updateGL();
          }
        }
      }


      void RenderFrame::mousePressEvent (QMouseEvent* event)
      {
        last_pos = event->pos();
      }

      void RenderFrame::mouseMoveEvent (QMouseEvent* event)
      {
        int dx = event->x() - last_pos.x();
        int dy = event->y() - last_pos.y();
        last_pos = event->pos();
        if (dx == 0 && dy == 0) return;

        if (event->modifiers() == Qt::NoModifier) {
          if (event->buttons() == Qt::LeftButton) {
            Point<> x = projection.screen_to_model_direction (QPoint (-dx, dy), focus);
            Point<> z = projection.screen_normal();
            Point<> v = x.cross (z);
            v.normalise();
            float angle = ROTATION_INC * Math::sqrt (float (Math::pow2 (dx) + Math::pow2 (dy)));
            if (angle > M_PI_2) angle = M_PI_2;

            Math::Versor<float> rot (angle, v);
            orientation = rot * orientation;
            updateGL();
          }
          else if (event->buttons() == Qt::MidButton) {
            focus += projection.screen_to_model_direction (QPoint (dx, -dy), focus);
            updateGL();
          }
          else if (event->buttons() == Qt::RightButton) {
            distance *= 1.0 - DIST_INC*dy;
            if (distance < DIST_MIN) distance = DIST_MIN;
            if (distance > DIST_MAX) distance = DIST_MAX;
            updateGL();
          }
        }
        else if (event->modifiers() == Qt::ControlModifier) {
          if (event->buttons() == Qt::RightButton) {
            view_angle -= ANGLE_INC*dy;
            if (view_angle < ANGLE_MIN) view_angle = ANGLE_MIN;
            if (view_angle > ANGLE_MAX) view_angle = ANGLE_MAX;
            updateGL();
          }
        }
      }

      void RenderFrame::wheelEvent (QWheelEvent* event)
      {
        int scroll = event->delta() / 120;
        for (int n = 0; n < scroll; n++) scale *= SCALE_INC;
        for (int n = 0; n > scroll; n--) scale /= SCALE_INC;
        if (scale > SCALE_MAX) scale = SCALE_MAX;
        if (scale < SCALE_MIN) scale = SCALE_MIN;
        updateGL();
      }


      void RenderFrame::screenshot (int oversampling, const std::string& image_name)
      {
        QApplication::setOverrideCursor (Qt::BusyCursor);
        screenshot_name = image_name;
        OS = oversampling;
        OS_x = OS_y = 0;
        framebuffer = new GLubyte [3*projection.width()*projection.height()];
        pix = new QImage (OS*projection.width(), OS*projection.height(), QImage::Format_RGB32);
        updateGL();
      }



      void RenderFrame::snapshot ()
      {
        makeCurrent();
        glPixelStorei (GL_PACK_ALIGNMENT, 1);
        glReadPixels (0, 0, projection.width(), projection.height(), GL_RGB, GL_UNSIGNED_BYTE, framebuffer);

        int start_i = projection.width()*OS_x;
        int start_j = projection.height()* (OS-OS_y-1);
        for (int j = 0; j < projection.height(); j++) {
          int j2 = projection.height()-j-1;
          for (int i = 0; i < projection.width(); i++) {
            GLubyte* p = framebuffer + 3* (i+projection.width()*j);
            pix->setPixel (start_i + i, start_j + j2, qRgb (p[0], p[1], p[2]));
          }
        }

        if (OS_x == OS-1 && OS_y == OS-1)
          pix->save (screenshot_name.c_str(), "PNG");

        OS_x++;
        if (OS_x >= OS) {
          OS_x = 0;
          OS_y++;
          if (OS_y >= OS) {
            pix = NULL;
            delete [] framebuffer;
            OS = OS_x = OS_y = 0;
            QApplication::restoreOverrideCursor();
          }
        }

        updateGL();
      }



    }
  }
}

