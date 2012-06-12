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

#include "math/vector.h"
#include "gui/mrview/mode/mode3d.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        namespace
        {
          class OrientationLabel
          {
            public:
              OrientationLabel () { }
              OrientationLabel (const Point<>& direction, const char textlabel) :
                dir (direction), label (1, textlabel) { }
              Point<> dir;
              std::string label;
              bool operator< (const OrientationLabel& R) const {
                return dir.norm2() < R.dir.norm2();
              }
          };
        }

        Mode3D::Mode3D (Window& parent) : 
          Mode2D (parent) { 
            using namespace App;
            Options opt = get_options ("view");
            if (opt.size()) {
              error ("TODO: apply view option");
            }

          }
        Mode3D::~Mode3D () { }

        void Mode3D::paint ()
        {
          if (!focus()) reset_view();
          if (!target()) set_target (focus());

          // camera target:
          //Point<> F = target();

          // info for projection:
          int w = glarea()->width(), h = glarea()->height();
          float fov = FOV() / (float) (w+h);
          float depth = 100.0;

          // set up projection & modelview matrices:
          glMatrixMode (GL_PROJECTION);
          glLoadIdentity ();
          glOrtho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);

          glMatrixMode (GL_MODELVIEW);
          glLoadIdentity ();

          Math::Quaternion<float> Q = orientation();
          if (!Q) {
            Q = Math::Quaternion<float> (1.0, 0.0, 0.0, 0.0);
            set_orientation (Q);
          }

          float M[9];
          Q.to_matrix (M);
          float T [] = {
            M[0], M[1], M[2], 0.0,
            M[3], M[4], M[5], 0.0,
            M[6], M[7], M[8], 0.0,
            0.0, 0.0, 0.0, 1.0
          };
          float S[16];
          adjust_projection_matrix (S, T);
          glMultMatrixf (S);

          glTranslatef (-target() [0], -target() [1], -target() [2]);
          update_modelview_projection_viewport();

          // set up OpenGL environment:
          glDisable (GL_BLEND);
          glEnable (GL_TEXTURE_3D);
          glShadeModel (GL_FLAT);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

          // render image:
          DEBUG_OPENGL;
          image()->render3D (*this);
          DEBUG_OPENGL;

          glDisable (GL_TEXTURE_3D);

          draw_focus();
          draw_orientation_labels();
        }



        void Mode3D::draw_orientation_labels ()
        {
          if (show_orientation_action->isChecked()) {
            glColor4f (1.0, 0.0, 0.0, 1.0);
            std::vector<OrientationLabel> labels;
            labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (-1.0, 0.0, 0.0)), 'L'));
            labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (1.0, 0.0, 0.0)), 'R'));
            labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (0.0, -1.0, 0.0)), 'P'));
            labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (0.0, 1.0, 0.0)), 'A'));
            labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (0.0, 0.0, -1.0)), 'I'));
            labels.push_back (OrientationLabel (model_to_screen_direction (Point<> (0.0, 0.0, 1.0)), 'S'));

            std::sort (labels.begin(), labels.end());
            for (size_t i = 2; i < labels.size(); ++i) {
              float pos[] = { labels[i].dir[0], labels[i].dir[1] };
              float dist = std::min (width() /Math::abs (pos[0]), height() /Math::abs (pos[1])) / 2.0;
              int x = Math::round (width() /2.0 + pos[0]*dist);
              int y = Math::round (height() /2.0 + pos[1]*dist);
              renderTextInset (x, y, std::string (labels[i].label));
            }

          }
        }



        void Mode3D::set_cursor ()
        {
          if (mouse_edge() == (RightEdge | BottomEdge))
            glarea()->setCursor (Cursor::window);
          else if (mouse_edge() == (RightEdge | TopEdge))
            glarea()->setCursor (Cursor::throughplane_rotate);
          else if (mouse_edge() & RightEdge)
            glarea()->setCursor (Cursor::forward_backward);
          else if (mouse_edge() & LeftEdge)
            glarea()->setCursor (Cursor::zoom);
          else if (mouse_edge() & TopEdge)
            glarea()->setCursor (Cursor::inplane_rotate);
          else
            glarea()->setCursor (Cursor::crosshair);
        }






        bool Mode3D::mouse_click ()
        {
          if (mouse_modifiers() == Qt::NoModifier) {

            if (mouse_buttons() == Qt::LeftButton) {
              glarea()->setCursor (Cursor::crosshair);
              set_focus (screen_to_model (mouse_pos()));
              updateGL();
              return true;
            }

            else if (mouse_buttons() == Qt::RightButton) {
              if (!mouse_edge()) {
                glarea()->setCursor (Cursor::pan_crosshair);
                return true;
              }
            }
          }

          return false;
        }





        bool Mode3D::mouse_move ()
        {
          if (mouse_buttons() == Qt::NoButton) {
            set_cursor();
            return false;
          }


          if (mouse_modifiers() == Qt::NoModifier) {

            if (mouse_buttons() == Qt::LeftButton) {
              set_focus (screen_to_model());
              updateGL();
              return true;
            }

            if (mouse_buttons() == Qt::RightButton) {

              if (mouse_edge() == (RightEdge | BottomEdge)) {
                image()->adjust_windowing (mouse_dpos());
                window.scaling_updated();
                updateGL();
                return true;
              }

              if (mouse_edge() == (RightEdge | TopEdge)) {
                QPoint dpos = mouse_dpos();
                if (dpos.x() == 0 && dpos.y() == 0)
                  return true;
                Point<> x = screen_to_model_direction (Point<> (dpos.x(), -dpos.y(), 0.0));
                Point<> z = screen_to_model_direction (Point<> (0.0, 0.0, 1.0));
                Point<> v (x.cross (z));
                float angle = ROTATION_INC * Math::sqrt (float (Math::pow2 (dpos.x()) + Math::pow2 (dpos.y())));
                v.normalise();
                if (angle > M_PI_2) angle = M_PI_2;

                Math::Quaternion<float> q = Math::Quaternion<float> (angle, v) * orientation();
                q.normalise();
                set_orientation (q);
                updateGL();
                return true;
              }

              if (mouse_edge() == TopEdge) {
                float angle = ROTATION_INC * mouse_dpos().x();
                Point<> v = screen_to_model_direction (Point<> (0.0, 0.0, 1.0));
                v.normalise();

                Math::Quaternion<float> q = Math::Quaternion<float> (angle, v) * orientation();
                q.normalise();
                set_orientation (q);
                updateGL();
                return true;
              }

              if (mouse_edge() & RightEdge) {
                move_in_out_FOV (mouse_dpos().y());
                updateGL();
                return true;
              }

              if (mouse_edge() & LeftEdge) {
                change_FOV_fine (mouse_dpos().y());
                updateGL();
                return true;
              }


              set_target (target() - screen_to_model_direction (mouse_dpos()));
              updateGL();
              return true;
            }

          }
          return false;
        }






        bool Mode3D::mouse_release ()
        {
          set_cursor();
          return true;
        }




        void Mode3D::reset ()
        {
          Math::Quaternion<float> Q;
          set_orientation (Q);
          Mode2D::reset();
        }


        using namespace App;
        const App::OptionGroup Mode3D::options = OptionGroup ("3D reslice mode") 
          + Option ("view", "specify initial angle of view")
          + Argument ("azimuth").type_float(-M_PI, 0.0, M_PI)
          + Argument ("elevation").type_float(0.0, 0.0, M_PI);

      }
    }
  }
}


