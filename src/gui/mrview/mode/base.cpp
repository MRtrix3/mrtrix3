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

#include <QDockWidget>

#include "file/config.h"
#include "gui/opengl/gl.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        Base::Base (Window& parent, int flags) :
          window (parent),
          projection (window.glarea, window.font),
          features (flags),
          painting (false) { } 


        Base::~Base ()
        {
          glarea()->setCursor (Cursor::crosshair);
        }

        Projection* Base::get_current_projection () { return &projection; }

        void Base::paintGL ()
        {
          painting = true;

          projection.set_viewport (0, 0, glarea()->width(), glarea()->height());

          glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
          if (!image()) {
            projection.setup_render_text();
            projection.render_text (10, 10, "No image loaded");
            projection.done_render_text();
            goto done_painting;
          }

          if (!focus() || !target()) 
            reset_view();

          {
            // call mode's draw method:
            paint (projection);

            glDisable (GL_MULTISAMPLE);
            glColor4f (1.0, 1.0, 0.0, 1.0);

            projection.setup_render_text();
            if (window.show_voxel_info()) {
              Point<> voxel (image()->interp.scanner2voxel (focus()));
              Image::VoxelType& imvox (image()->voxel());
              ssize_t vox [] = { Math::round<int> (voxel[0]), Math::round<int> (voxel[1]), Math::round<int> (voxel[2]) };

              std::string vox_str = printf ("voxel: [ %d %d %d ", vox[0], vox[1], vox[2]);
              for (size_t n = 3; n < imvox.ndim(); ++n)
                vox_str += str(imvox[n]) + " ";
              vox_str += "]";

              projection.render_text (printf ("position: [ %.4g %.4g %.4g ] mm", focus() [0], focus() [1], focus() [2]), LeftEdge | BottomEdge);
              projection.render_text (vox_str, LeftEdge | BottomEdge, 1);
              std::string value;
              if (vox[0] >= 0 && vox[0] < imvox.dim (0) &&
                  vox[1] >= 0 && vox[1] < imvox.dim (1) &&
                  vox[2] >= 0 && vox[2] < imvox.dim (2)) {
                imvox[0] = vox[0];
                imvox[1] = vox[1];
                imvox[2] = vox[2];
                cfloat val = imvox.value();
                value = "value: " + str (val);
              }
              else value = "value: ?";
              projection.render_text (value, LeftEdge | BottomEdge, 2);
            }

            if (window.show_comments()) {
              for (size_t i = 0; i < image()->header().comments().size(); ++i)
                projection.render_text (image()->header().comments() [i], LeftEdge | TopEdge, i);
            }

            projection.done_render_text();

            if (window.show_colourbar())
              window.colourbar_renderer.render (projection, *image(), window.colourbar_position_index, image()->scale_inverted());

            QList<QAction*> tools = window.tools()->actions();
            for (int i = 0; i < tools.size(); ++i) {
              Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(tools[i])->instance;
              if (dock)
                dock->tool->drawOverlays (projection);
            }
          }

done_painting:
          painting = false;
        }


        void Base::paint (Projection& projection) { }
        void Base::mouse_press_event () { }
        void Base::mouse_release_event () { }

        void Base::slice_move_event (int x) 
        {
          const Projection* proj = get_current_projection();
          if (!proj) return;
          move_in_out (x * std::min (std::min (image()->header().vox(0), image()->header().vox(1)), image()->header().vox(2)), *proj);
          move_target_to_focus_plane (*proj);
          updateGL();
        }




        void Base::set_focus_event ()
        {
          const Projection* proj = get_current_projection();
          if (!proj) return;
          set_focus (proj->screen_to_model (window.mouse_position(), focus()), *proj);
          updateGL();
        }




        void Base::contrast_event ()
        {
          image()->adjust_windowing (window.mouse_displacement());
          window.on_scaling_changed();
          updateGL();
        }



        void Base::pan_event ()
        {
          const Projection* proj = get_current_projection();
          if (!proj) return;
          set_target (target() - proj->screen_to_model_direction (window.mouse_displacement(), target()));
          updateGL();
        }



        void Base::panthrough_event ()
        {
          const Projection* proj = get_current_projection();
          if (!proj) return;
          move_in_out_FOV (window.mouse_displacement().y(), *proj);
          move_target_to_focus_plane (*proj);
          updateGL();
        }




        void Base::tilt_event ()
        {
          const Projection* proj = get_current_projection();
          if (!proj) return;
          QPoint dpos = window.mouse_displacement();
          if (dpos.x() == 0 && dpos.y() == 0)
            return;
          Point<> x = proj->screen_to_model_direction (dpos, target());
          Point<> z = proj->screen_normal();
          Point<> v (x.cross (z));
          float angle = -ROTATION_INC * Math::sqrt (float (Math::pow2 (dpos.x()) + Math::pow2 (dpos.y())));
          v.normalise();
          if (angle > M_PI_2) angle = M_PI_2;

          Math::Versor<float> q = Math::Versor<float> (angle, v) * orientation();
          q.normalise();
          set_orientation (q);
          updateGL();
        }





        void Base::rotate_event ()
        {
          const Projection* proj = get_current_projection();
          if (!proj) return;
          Point<> x1 (window.mouse_position().x() - proj->x_position() - proj->width()/2,
              window.mouse_position().y() - proj->y_position() - proj->height()/2,
              0.0);

          if (x1.norm() < 16) 
            return;

          Point<> x0 (window.mouse_displacement().x() - x1[0], 
              window.mouse_displacement().y() - x1[1],
              0.0);

          x1.normalise();
          x0.normalise();

          Point<> n = x1.cross (x0);

          Point<> v = proj->screen_normal();
          v.normalise();

          Math::Versor<float> q = Math::Versor<float> (n[2], v) * orientation();
          q.normalise();
          set_orientation (q);
          updateGL();
        }




        Tool::Dock* Base::get_extra_controls () { 
          return NULL;
        }




        void Base::reset_event () 
        { 
          reset_view();
          updateGL();
        }


        void Base::reset_view () 
        {
          if (!image()) return;
          const Projection* proj = get_current_projection();
          if (!proj) return;

          float dim[] = {
            image()->header().dim (0) * image()->header().vox (0),
            image()->header().dim (1) * image()->header().vox (1),
            image()->header().dim (2) * image()->header().vox (2)
          };
          if (dim[0] < dim[1] && dim[0] < dim[2])
            set_plane (0);
          else if (dim[1] < dim[0] && dim[1] < dim[2])
            set_plane (1);
          else
            set_plane (2);

          Point<> p (image()->header().dim (0)/2.0f, image()->header().dim (1)/2.0f, image()->header().dim (2)/2.0f);
          p = image()->interp.voxel2scanner (p);
          set_focus (p, *proj);
          set_target (p);
          Math::Versor<float> orient;
          orient.from_matrix (image()->header().transform());
          set_orientation (orient);

          int x, y;
          image()->get_axes (plane(), x, y);
          set_FOV (std::max (dim[x], dim[y]));

          updateGL();
        }



        void Base::register_extra_controls (Tool::Dock* controls) 
        {
          connect (controls, SIGNAL (visibilityChanged (bool)), window.extra_controls_action, SLOT (setChecked (bool)));
        }



        GL::mat4 Base::adjust_projection_matrix (const GL::mat4& Q, int proj) const
        {
          GL::mat4 M;
          M(3,0) = M(3,1) = M(3,2) = M(0,3) = M(1,3) = M(2,3) = 0.0f;
          M(3,3) = 1.0f;
          if (proj == 0) { // sagittal
            for (size_t n = 0; n < 3; n++) {
              M(0,n) = -Q(1,n);  // x: -y
              M(1,n) =  Q(2,n);  // y: z
              M(2,n) = -Q(0,n);  // z: -x
            }
          }
          else if (proj == 1) { // coronal
            for (size_t n = 0; n < 3; n++) {
              M(0,n) = -Q(0,n);  // x: -x
              M(1,n) =  Q(2,n);  // y: z
              M(2,n) =  Q(1,n);  // z: y
            }
          }
          else { // axial
            for (size_t n = 0; n < 3; n++) {
              M(0,n) = -Q(0,n);  // x: -x
              M(1,n) =  Q(1,n);  // y: y
              M(2,n) = -Q(2,n);  // z: -z
            }
          }
          return M;
        }

      }
    }
  }
}

#undef MODE


