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

#include <QAction>

#include "mrtrix.h"
#include "mrview/mode/mode2d.h"

namespace MR {
  namespace Viewer {
    namespace Mode {

      Mode2D::Mode2D (Window& parent) : Base (parent) 
      { 
        axial_action = new QAction(tr("&Axial"), this);
        axial_action->setShortcut (tr("A"));
        axial_action->setStatusTip (tr("Switch to axial projection"));
        connect (axial_action, SIGNAL (triggered()), this, SLOT (axial()));
        add_action (axial_action);

        sagittal_action = new QAction(tr("&Sagittal"), this);
        sagittal_action->setShortcut (tr("S"));
        sagittal_action->setStatusTip (tr("Switch to sagittal projection"));
        connect (sagittal_action, SIGNAL (triggered()), this, SLOT (sagittal()));
        add_action (sagittal_action);

        coronal_action = new QAction(tr("&Coronal"), this);
        coronal_action->setShortcut (tr("C"));
        coronal_action->setStatusTip (tr("Switch to coronal projection"));
        connect (coronal_action, SIGNAL (triggered()), this, SLOT (coronal()));
        add_action (coronal_action);

        QAction* separator = new QAction (this);
        separator->setSeparator (true);
        add_action (separator);

        show_focus_action = new QAction(tr("Show &Focus"), this);
        show_focus_action->setShortcut (tr("F"));
        show_focus_action->setStatusTip (tr("Show focus with the crosshairs"));
        connect (show_focus_action, SIGNAL (triggered()), this, SLOT (show_focus()));
        add_action (show_focus_action);

        reset_action = new QAction(tr("Reset &View"), this);
        reset_action->setShortcut (tr("Crtl+R"));
        reset_action->setStatusTip (tr("Reset image projection & zoom"));
        connect (reset_action, SIGNAL (triggered()), this, SLOT (reset()));
        add_action (reset_action);

        reset();
      }


      Mode2D::~Mode2D () { }

      void Mode2D::paint () 
      { 
        if (!image()) return;

        // set up projection matrix:
        int w = window.width(), h = window.height();
        float fov = FOV() / (float) (w+h);
        float depth = image()->vox.dim (projection()) * image()->vox.vox (projection());

        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        glOrtho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);

        // set up modelview matrix:
        float M[16];
        const float* Q = image()->interp.image2scanner_matrix();
        M[0]  = -Q[0]; M[1]  = Q[1]; M[2]  =  -Q[2]; M[3]  = 0.0;
        M[4]  = -Q[4]; M[5]  = Q[5]; M[6]  =  -Q[6]; M[7]  = 0.0;
        M[8]  = -Q[8]; M[9]  = Q[9]; M[10] = -Q[10]; M[11] = 0.0;
        M[12] =   0.0; M[13] =  0.0; M[14] =    0.0; M[15] = 1.0;

        if (projection() == 0) {
          for (size_t n = 0; n < 3; n++) {
            float f = M[4*n];
            M[4*n] = -M[4*n+1];
            M[4*n+1] = -M[4*n+2];
            M[4*n+2] = f;
          }
        }
        else if (projection() == 1) {
          for (size_t n = 0; n < 3; n++) {
            float f = M[4*n+1];
            M[4*n+1] = -M[4*n+2];
            M[4*n+2] = f;
          }
        }

        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity ();
        glMultMatrixf (M);

        // move to focus:
        VAR (focus());
        Point voxel (image()->interp.scanner2voxel (focus()));
        VAR (voxel);
        int slice = voxel[projection()] = Math::round (voxel[projection()]);
        const Point n (M[2],M[6],M[10]);
        VAR (n);
        VAR (slice);

        float a = n.dot (focus() - image()->interp.voxel2scanner (voxel));
        voxel = a * n - focus();
        glTranslatef (voxel[0], voxel[1], voxel[2]);

        glDisable(GL_BLEND);
        glEnable (GL_TEXTURE_2D);
        glShadeModel (GL_FLAT);
        glDisable (GL_DEPTH_TEST);
        glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glDepthMask (GL_FALSE);
        glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        image()->render2D (projection(), slice);

        glDisable (GL_TEXTURE_2D);

        if (window.show_focus()) {
          Point F = model_to_screen (focus());

          glMatrixMode (GL_PROJECTION);
          glPushMatrix ();
          glLoadIdentity ();
          glOrtho (0, w, 0, h, -1.0, 1.0);
          glMatrixMode (GL_MODELVIEW);
          glPushMatrix ();
          glLoadIdentity ();

          float alpha = 0.5;

          glColor4f (1.0, 1.0, 0.0, alpha);
          glLineWidth (1.0);
          glEnable (GL_BLEND);
          glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

          glBegin (GL_LINES);
          glVertex2f (0.0, F[1]);
          glVertex2f (w, F[1]);
          glVertex2f (F[0], 0.0);
          glVertex2f (F[0], h);
          glEnd ();

          glDisable(GL_BLEND);
          glPopMatrix ();
          glMatrixMode (GL_PROJECTION);
          glPopMatrix ();
          glMatrixMode (GL_MODELVIEW);

        }
        glDepthMask (GL_TRUE);
      }

      void Mode2D::mousePressEvent (QMouseEvent* event) { }
      void Mode2D::mouseMoveEvent (QMouseEvent* event) { }
      void Mode2D::mouseDoubleClickEvent (QMouseEvent* event) { }
      void Mode2D::mouseReleaseEvent (QMouseEvent* event) { }
      void Mode2D::wheelEvent (QWheelEvent* event) 
      {
        if (!image()) return;
        if (event->orientation() != Qt::Vertical) return;
        Point F (focus());
        Point D (0.0, 0.0, 0.0);
        D[projection()] = event->delta() / 120.0;
        F += image()->interp.voxel2scanner_dir (D);
        set_focus (F);
        event->accept();
      }

      void Mode2D::axial () { set_projection (2); }
      void Mode2D::sagittal () { set_projection (0); }
      void Mode2D::coronal () { set_projection (1); }
      void Mode2D::show_focus () { TEST; }
      void Mode2D::reset ()
      { 
      }
    }
  }
}

