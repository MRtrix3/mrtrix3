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
#include "gui/mrview/mode/slice.h"

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

        Slice::Slice (Window& parent, int flags) : 
          Base (parent, flags) {
            using namespace App;
            Options opt = get_options ("view");
            if (opt.size()) {
              FAIL ("TODO: apply view option");
            }

          }


        Slice::~Slice () { }



        void Slice::paint (Projection& projection)
        {
          // info for projection:
          float fov = FOV() / (float) (width()+height());
          float depth = 100.0;//std::max (std::max (image()->header().dim(0), image()->header().dim(1)), image()->header().dim(2));

          // set up projection & modelview matrices:
          GL::mat4 P = GL::ortho (
              -width()*fov, width()*fov, 
              -height()*fov, height()*fov, 
              -depth, depth);
          GL::mat4 M = snap_to_image() ? GL::mat4 (image()->interp.image2scanner_matrix()) : GL::mat4 (orientation());
          GL::mat4 MV = adjust_projection_matrix (M) * GL::translate (-target());
          projection.set (MV, P);

          // set up OpenGL environment:
          glDisable (GL_BLEND);
          glEnable (GL_TEXTURE_3D);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

          // render image:
          DEBUG_OPENGL;
          if (snap_to_image()) 
            image()->render2D (projection, plane(), slice());
          else 
            image()->render3D (projection, projection.depth_of (focus()));
          DEBUG_OPENGL;

          glDisable (GL_TEXTURE_3D);

          render_tools2D (projection);

          if (window.show_crosshairs()) 
            projection.render_crosshairs (focus());

          draw_orientation_labels();
        }



        void Slice::draw_orientation_labels ()
        {
          if (window.show_orientation_labels()) {
            glColor4f (1.0, 0.0, 0.0, 1.0);
            std::vector<OrientationLabel> labels;
            labels.push_back (OrientationLabel (projection.model_to_screen_direction (Point<> (-1.0, 0.0, 0.0)), 'L'));
            labels.push_back (OrientationLabel (projection.model_to_screen_direction (Point<> (1.0, 0.0, 0.0)), 'R'));
            labels.push_back (OrientationLabel (projection.model_to_screen_direction (Point<> (0.0, -1.0, 0.0)), 'P'));
            labels.push_back (OrientationLabel (projection.model_to_screen_direction (Point<> (0.0, 1.0, 0.0)), 'A'));
            labels.push_back (OrientationLabel (projection.model_to_screen_direction (Point<> (0.0, 0.0, -1.0)), 'I'));
            labels.push_back (OrientationLabel (projection.model_to_screen_direction (Point<> (0.0, 0.0, 1.0)), 'S'));

            projection.setup_render_text (1.0, 0.0, 0.0);
            std::sort (labels.begin(), labels.end());
            for (size_t i = 2; i < labels.size(); ++i) {
              float pos[] = { labels[i].dir[0], labels[i].dir[1] };
              float dist = std::min (projection.width()/Math::abs (pos[0]), projection.height()/Math::abs (pos[1])) / 2.0;
              int x = Math::round (projection.width() /2.0 + pos[0]*dist);
              int y = Math::round (projection.height() /2.0 + pos[1]*dist);
              projection.render_text_inset (x, y, std::string (labels[i].label));
            }
            projection.done_render_text();

          }
        }









        void Slice::slice_move_event (int x) 
        {
          move_in_out (x * std::min (std::min (image()->header().vox(0), image()->header().vox(1)), image()->header().vox(2)));
          updateGL();
        }




        void Slice::set_focus_event ()
        {
          set_focus (projection.screen_to_model (window.mouse_position(), focus()));
          updateGL();
        }




        void Slice::contrast_event ()
        {
          image()->adjust_windowing (window.mouse_displacement());
          window.on_scaling_changed();
          updateGL();
        }



        void Slice::pan_event ()
        {
          set_target (target() - projection.screen_to_model_direction (window.mouse_displacement(), target()));
          updateGL();
        }



        void Slice::panthrough_event ()
        {
          move_in_out_FOV (window.mouse_displacement().y());
          updateGL();
        }




        void Slice::tilt_event ()
        {
          QPoint dpos = window.mouse_displacement();
          if (dpos.x() == 0 && dpos.y() == 0)
            return;
          Point<> x = projection.screen_to_model_direction (dpos, target());
          Point<> z = projection.screen_normal();
          Point<> v (x.cross (z));
          float angle = -ROTATION_INC * Math::sqrt (float (Math::pow2 (dpos.x()) + Math::pow2 (dpos.y())));
          v.normalise();
          if (angle > M_PI_2) angle = M_PI_2;

          Math::Versor<float> q = Math::Versor<float> (angle, v) * orientation();
          q.normalise();
          set_orientation (q);
          updateGL();
        }





        void Slice::rotate_event ()
        {
          Point<> x1 (window.mouse_position().x() - projection.width()/2,
              window.mouse_position().y() - projection.height()/2,
              0.0);

          if (x1.norm() < 16) 
            return;

          Point<> x0 (window.mouse_displacement().x() - x1[0], 
              window.mouse_displacement().y() - x1[1],
              0.0);

          x1.normalise();
          x0.normalise();

          Point<> n = x1.cross (x0);

          Point<> v = projection.screen_normal();
          v.normalise();

          Math::Versor<float> q = Math::Versor<float> (n[2], v) * orientation();
          q.normalise();
          set_orientation (q);
          updateGL();
        }



        using namespace App;
        const App::OptionGroup Slice::options = OptionGroup ("single-slice mode") 
          + Option ("view", "specify initial angle of view")
          + Argument ("azimuth").type_float(-M_PI, 0.0, M_PI)
          + Argument ("elevation").type_float(0.0, 0.0, M_PI);

      }
    }
  }
}


