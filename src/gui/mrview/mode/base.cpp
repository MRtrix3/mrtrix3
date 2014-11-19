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
          update_overlays (false),
          painting (false) { } 


        Base::~Base ()
        {
          glarea()->setCursor (Cursor::crosshair);
        }

        const Projection* Base::get_current_projection () const { return &projection; }

        void Base::paintGL ()
        {
          painting = true;

          projection.set_viewport (window, 0, 0, width(), height());

          gl::Clear (gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
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

            gl::Disable (gl::MULTISAMPLE);

            projection.setup_render_text();
            if (window.show_voxel_info()) {
              Point<> voxel (image()->interp.scanner2voxel (focus()));
              Image::VoxelType& imvox (image()->voxel());
              ssize_t vox [] = { ssize_t(std::round (voxel[0])), ssize_t(std::round (voxel[1])), ssize_t(std::round (voxel[2])) };

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
              Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(tools[i])->dock;
              if (dock)
                dock->tool->drawOverlays (projection);
            }
          }

done_painting:
          painting = false;
          update_overlays = false;
        }


        void Base::paint (Projection&) { }
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
          set_focus (proj->screen_to_model (window.mouse_position(), focus()));
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






        Math::Versor<float> Base::get_tilt_rotation () const 
        {
          Math::Versor<float> rot;
          const Projection* proj = get_current_projection();
          if (!proj) {
            rot.invalidate(); 
            return rot;
          }

          QPoint dpos = window.mouse_displacement();
          if (dpos.x() == 0 && dpos.y() == 0) {
            rot.invalidate();
            return rot;
          }

          Point<> x = proj->screen_to_model_direction (dpos, target());
          Point<> z = proj->screen_normal();
          Point<> v (x.cross (z));
          float angle = -ROTATION_INC * std::sqrt (float (Math::pow2 (dpos.x()) + Math::pow2 (dpos.y())));
          v.normalise();
          if (angle > Math::pi_2) 
            angle = Math::pi_2;

          return Math::Versor<float> (angle, v);
        }






        Math::Versor<float> Base::get_rotate_rotation () const
        {
          Math::Versor<float> rot;
          rot.invalidate();

          const Projection* proj = get_current_projection();
          if (!proj) 
            return rot;

          QPoint dpos = window.mouse_displacement();
          if (dpos.x() == 0 && dpos.y() == 0) 
            return rot;

          Point<> x1 (window.mouse_position().x() - proj->x_position() - proj->width()/2,
              window.mouse_position().y() - proj->y_position() - proj->height()/2,
              0.0);

          if (x1.norm() < 16.0f) 
            return rot;

          Point<> x0 (dpos.x() - x1[0], dpos.y() - x1[1], 0.0);

          x1.normalise();
          x0.normalise();

          Point<> n = x1.cross (x0);

          Point<> v = proj->screen_normal();
          v.normalise();

          return Math::Versor<float> (n[2], v);
        }





        void Base::tilt_event ()
        {
          if (snap_to_image()) 
            window.set_snap_to_image (false);

          Math::Versor<float> rot = get_tilt_rotation();
          if (!rot) 
            return;
          Math::Versor<float> orient = rot * orientation();
          orient.normalise();
          set_orientation (orient);
          updateGL();
        }





        void Base::rotate_event ()
        {
          if (snap_to_image()) 
            window.set_snap_to_image (false);

          Math::Versor<float> rot = get_rotate_rotation();
          if (!rot) 
            return;
          Math::Versor<float> orient = rot * orientation();
          orient.normalise();
          set_orientation (orient);
          updateGL();
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

          Point<> p (
              floor ((image()->header().dim(0)-1)/2.0f),
              floor ((image()->header().dim(1)-1)/2.0f),
              floor ((image()->header().dim(2)-1)/2.0f)
              );

          set_focus (image()->interp.voxel2scanner (p));
          set_target (focus());
          reset_orientation();

          int x, y;
          image()->get_axes (plane(), x, y);
          set_FOV (std::max (dim[x], dim[y]));

          updateGL();
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


