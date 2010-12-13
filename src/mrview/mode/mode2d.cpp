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
#include "cursor.h"
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
      }


      Mode2D::~Mode2D () { }

      void Mode2D::paint () 
      { 
        if (!focus()) reset_view();
        if (!target()) set_target (focus());

        // set up modelview matrix:
        const float* Q = image()->interp.image2scanner_matrix();
        float M[16];

        adjust_projection_matrix (M, Q);

        // image slice:
        Point<> voxel (image()->interp.scanner2voxel (focus()));
        int slice = Math::round (voxel[projection()]);

        // camera target:
        Point<> F = image()->interp.scanner2voxel (target());
        F[projection()] = slice;
        F = image()->interp.voxel2scanner (F);

        // info for projection:
        int w = glarea()->width(), h = glarea()->height();
        float fov = FOV() / (float) (w+h);
        float depth = image()->vox.dim (projection()) * image()->vox.vox (projection());

        // set up projection & modelview matrices:
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity ();
        glOrtho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);

        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity ();
        glMultMatrixf (M);
        glTranslatef (-F[0], -F[1], -F[2]);

        // set up OpenGL environment:
        glDisable (GL_BLEND);
        glEnable (GL_TEXTURE_2D);
        glShadeModel (GL_FLAT);
        glDisable (GL_DEPTH_TEST);
        glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glDepthMask (GL_FALSE);
        glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // render image:
        image()->render2D (projection(), slice);

        glDisable (GL_TEXTURE_2D);

        draw_focus();

        if (show_orientation_action->isChecked()) {
          glColor4f (1.0, 0.0, 0.0, 1.0);
          switch (projection()) {
            case 0: 
              renderText ("A", LeftEdge);
              renderText ("S", TopEdge);
              renderText ("P", RightEdge);
              renderText ("I", BottomEdge);
              break;
            case 1: 
              renderText ("R", LeftEdge);
              renderText ("S", TopEdge);
              renderText ("L", RightEdge);
              renderText ("I", BottomEdge);
              break;
            case 2: 
              renderText ("R", LeftEdge);
              renderText ("A", TopEdge);
              renderText ("L", RightEdge);
              renderText ("P", BottomEdge);
              break;
            default:
              assert (0);
          }
        }
      }




      bool Mode2D::mouse_click ()
      { 
        if (mouse_modifiers() == Qt::NoModifier) {

          if (mouse_buttons() == Qt::LeftButton) {
            if (!mouse_edge()) {
              set_focus (screen_to_model (mouse_pos()));
              updateGL();
              return true;
            }
          }

          else if (mouse_buttons() == Qt::RightButton) 
            glarea()->setCursor (Cursor::pan_crosshair);
        }

        return false;
      }



      bool Mode2D::mouse_move () 
      {
        if (mouse_buttons() == Qt::NoButton) {
          if (mouse_edge() == ( RightEdge | BottomEdge ))
            glarea()->setCursor (Cursor::window);
          else if (mouse_edge() & RightEdge) 
            glarea()->setCursor (Cursor::forward_backward);
          else if (mouse_edge() & LeftEdge) 
            glarea()->setCursor (Cursor::zoom);
          else
            glarea()->setCursor (Cursor::crosshair);
          return false;
        }

        if (mouse_modifiers() == Qt::NoModifier) {

          if (mouse_buttons() == Qt::LeftButton) {

            if (mouse_edge() == ( RightEdge | BottomEdge) ) {
              image()->adjust_windowing (mouse_dpos_static());
              updateGL();
              return true;
            }

            if (mouse_edge() & RightEdge) {
              move_in_out (-0.001*mouse_dpos().y()*FOV());
              updateGL();
              return true;
            }

            if (mouse_edge() & LeftEdge) {
              change_FOV_fine (mouse_dpos().y());
              updateGL();
              return true;
            }

            set_focus (screen_to_model());
            updateGL();
            return true;
          }

          if (mouse_buttons() == Qt::RightButton) {
            set_target (target() - screen_to_model_direction (Point<> (mouse_dpos().x(), mouse_dpos().y(), 0.0)));
            updateGL();
            return true;
          }

        }

        return false;
      }




      bool Mode2D::mouse_release () 
      {
        if (mouse_edge() == ( RightEdge | BottomEdge)) 
          glarea()->setCursor (Cursor::window);
        else if (mouse_edge() & RightEdge) 
          glarea()->setCursor (Cursor::forward_backward);
        else if (mouse_edge() & LeftEdge) 
          glarea()->setCursor (Cursor::zoom);
        else 
          glarea()->setCursor (Cursor::crosshair);
        return true; 
      }




      bool Mode2D::mouse_wheel (float delta, Qt::Orientation orientation) 
      {
        if (orientation == Qt::Vertical) {

          if (mouse_modifiers() == Qt::ControlModifier) {
            change_FOV_scroll (-delta);
            updateGL();
            return true;
          }

          if (mouse_modifiers() == Qt::ShiftModifier) delta *= 10.0;
          else if (mouse_modifiers() != Qt::NoModifier) 
            return false;

          move_in_out (delta);
          updateGL();
          return true;
        }
        // TODO: use horizontal scroll to go through volumes.

        return false;
      }




      void Mode2D::axial () { set_projection (2); updateGL(); }
      void Mode2D::sagittal () { set_projection (0); updateGL(); }
      void Mode2D::coronal () { set_projection (1); updateGL(); }



      void Mode2D::reset_view ()
      { 
        if (!image()) return;
        float dim[] = {
          image()->H.dim(0) * image()->H.vox(0), 
          image()->H.dim(1) * image()->H.vox(1), 
          image()->H.dim(2) * image()->H.vox(2)
        };
        if (dim[0] < dim[1] && dim[0] < dim[2])
          set_projection (0);
        else if (dim[1] < dim[0] && dim[1] < dim[2])
          set_projection (1);
        else 
          set_projection (2);
       
        Point<> p (image()->H.dim(0)/2.0, image()->H.dim(1)/2.0, image()->H.dim(2)/2.0);
        set_focus (image()->interp.voxel2scanner (p));

        int x, y;
        image()->get_axes (projection(), x, y);
        set_FOV (std::max (dim[x], dim[y]));

        set_target (Point<>());
      }


      void Mode2D::reset () 
      {
        reset_view();
        updateGL();
      }

    }
  }
}

