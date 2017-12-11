/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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

        Base::Base (int flags) :
          projection (window().glarea, window().font),
          features (flags),
          update_overlays (false),
          visible (true) { }

        Base::~Base ()
        {
          glarea()->setCursor (Cursor::crosshair);
        }

        const Projection* Base::get_current_projection () const { return &projection; }

        void Base::paintGL ()
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          GL_CHECK_ERROR;

          projection.set_viewport (window(), 0, 0, width(), height());

          GL_CHECK_ERROR;
          gl::Clear (gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
          if (!image()) {
            projection.setup_render_text();
            projection.render_text (10, 10, "No image loaded");
            projection.done_render_text();
            goto done_painting;
          }

          GL_CHECK_ERROR;
          if (!std::isfinite (focus().squaredNorm()) || !std::isfinite (target().squaredNorm()))
            reset_view();

          {
            GL_CHECK_ERROR;
            // call mode's draw method:
            paint (projection);

            gl::Disable (gl::MULTISAMPLE);
            GL_CHECK_ERROR;

            projection.setup_render_text();
            if (window().show_voxel_info()) {
              Eigen::Vector3f voxel (image()->scanner2voxel() * focus());
              ssize_t vox [] = { ssize_t(std::round (voxel[0])), ssize_t(std::round (voxel[1])), ssize_t(std::round (voxel[2])) };

              std::string vox_str = printf ("voxel: [ %d %d %d ", vox[0], vox[1], vox[2]);
              for (size_t n = 3; n < image()->header().ndim(); ++n)
                vox_str += str(image()->image.index(n)) + " ";
              vox_str += "]";

              projection.render_text (printf ("position: [ %.4g %.4g %.4g ] mm", focus() [0], focus() [1], focus() [2]), LeftEdge | BottomEdge);
              projection.render_text (vox_str, LeftEdge | BottomEdge, 1);
              std::string value_str = "value: ";
              cfloat value = image()->interpolate() ?
                image()->trilinear_value (window().focus()) :
                image()->nearest_neighbour_value (window().focus());
              if (std::isfinite (std::abs (value)))
                value_str += str(value);
              else
                value_str += "?";

              projection.render_text (value_str, LeftEdge | BottomEdge, 2);

              // Draw additional labels from tools
              QList<QAction*> tools = window().tools()->actions();
              for (size_t i = 0, line_num = 3, N = tools.size(); i < N; ++i) {
                Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(tools[i])->dock;
                if (dock)
                  line_num += dock->tool->draw_tool_labels (LeftEdge | BottomEdge, line_num, projection);
              }
            }
            GL_CHECK_ERROR;

            if (window().show_comments()) {
              for (size_t line = 0; line != image()->comments().size(); ++line)
                projection.render_text (image()->comments()[line], LeftEdge | TopEdge, line);
            }

            projection.done_render_text();

            GL_CHECK_ERROR;
            if (window().show_colourbar()) {

              auto &colourbar_renderer = window().colourbar_renderer;

              colourbar_renderer.begin_render_colourbars (&projection, window().colourbar_position, 1);
              colourbar_renderer.render (*image(), image()->scale_inverted());
              colourbar_renderer.end_render_colourbars ();

              QList<QAction*> tools = window().tools()->actions();
              size_t num_tool_colourbars = 0;
              for (size_t i = 0, N = tools.size(); i < N; ++i) {
                Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(tools[i])->dock;
                if (dock)
                  num_tool_colourbars += dock->tool->visible_number_colourbars ();
              }


              colourbar_renderer.begin_render_colourbars (&projection, window().tools_colourbar_position, num_tool_colourbars);

              for (size_t i = 0, N = tools.size(); i < N; ++i) {
                Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(tools[i])->dock;
                if (dock)
                  dock->tool->draw_colourbars ();
              }

              colourbar_renderer.end_render_colourbars ();

            }
            GL_CHECK_ERROR;

          }

done_painting:
          update_overlays = false;
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void Base::paint (Projection&) { }
        void Base::mouse_press_event () { }
        void Base::mouse_release_event () { }

        void Base::slice_move_event (float x)
        {
          if (window().active_camera_interactor() && window().active_camera_interactor()->slice_move_event (x))
            return;

          const Projection* proj = get_current_projection();
          if (!proj) return;
          const auto &header = image()->header();
          float increment = snap_to_image() ?
            x * header.spacing (plane()) :
            x * std::pow (header.spacing(0) * header.spacing(1) * header.spacing(2), 1/3.f);
          auto move = get_through_plane_translation (increment, *proj);

          set_focus (focus() + move);
          move_target_to_focus_plane (*proj);
          updateGL();
        }




        void Base::set_focus_event ()
        {
          const Projection* proj = get_current_projection();
          if (!proj) return;
          set_focus (proj->screen_to_model (window().mouse_position(), focus()));
          updateGL();
        }




        void Base::contrast_event ()
        {
          image()->adjust_windowing (window().mouse_displacement());
          window().on_scaling_changed();
          updateGL();
        }



        void Base::pan_event ()
        {
          if (window().active_camera_interactor() && window().active_camera_interactor()->pan_event())
            return;

          const Projection* proj = get_current_projection();
          if (!proj) return;

          auto move = -proj->screen_to_model_direction (window().mouse_displacement(), target());
          set_target (target() + move);
          updateGL();
        }



        void Base::panthrough_event ()
        {
          if (window().active_camera_interactor() && window().active_camera_interactor()->panthrough_event())
            return;

          const Projection* proj = get_current_projection();
          if (!proj) return;
          auto move = get_through_plane_translation_FOV (window().mouse_displacement().y(), *proj);

          set_focus (focus() + move);
          move_target_to_focus_plane (*proj);
          updateGL();
        }




        void Base::setup_projection (const int axis, Projection& with_projection) const
        {
          const GL::mat4 M = snap_to_image() ? GL::mat4 (image()->image2scanner().matrix()) : GL::mat4 (orientation());
          setup_projection (adjust_projection_matrix (GL::transpose (M), axis), with_projection);
        }

        void Base::setup_projection (const Eigen::Quaternionf& V, Projection& with_projection) const
        {
          setup_projection (adjust_projection_matrix (GL::transpose (GL::mat4 (V))), with_projection);
        }

        void Base::setup_projection (const GL::mat4& M, Projection& with_projection) const
        {
          // info for projection:
          const int w = with_projection.width(), h = with_projection.height();
          const float fov = FOV() / (float)(w+h);
          const float depth = std::sqrt ( Math::pow2 (image()->header().spacing(0) * image()->header().size(0))
                                        + Math::pow2 (image()->header().spacing(1) * image()->header().size(1))
                                        + Math::pow2 (image()->header().spacing(2) * image()->header().size(2)));
          // set up projection & modelview matrices:
          const GL::mat4 P = GL::ortho (-w*fov, w*fov, -h*fov, h*fov, -depth, depth);
          const GL::mat4 MV = M * GL::translate (-target());
          with_projection.set (MV, P);
        }






        Eigen::Quaternionf Base::get_tilt_rotation () const
        {
          const Projection* proj = get_current_projection();
          if (!proj)
            return Eigen::Quaternionf();

          QPoint dpos = window().mouse_displacement();
          if (dpos.x() == 0 && dpos.y() == 0)
            return Eigen::Quaternionf();

          const Eigen::Vector3f x = proj->screen_to_model_direction (dpos, target());
          const Eigen::Vector3f z = proj->screen_normal();
          const Eigen::Vector3f v (x.cross (z).normalized());
          float angle = -ROTATION_INC * std::sqrt (float (Math::pow2 (dpos.x()) + Math::pow2 (dpos.y())));
          if (angle > Math::pi_2)
            angle = Math::pi_2;
          return Eigen::Quaternionf (Eigen::AngleAxisf (angle, v));
        }






        Eigen::Quaternionf Base::get_rotate_rotation () const
        {
          const Projection* proj = get_current_projection();
          if (!proj)
            return Eigen::Quaternionf();

          QPoint dpos = window().mouse_displacement();
          if (dpos.x() == 0 && dpos.y() == 0)
            return Eigen::Quaternionf();

          Eigen::Vector3f x1 (window().mouse_position().x() - proj->x_position() - proj->width()/2,
                              window().mouse_position().y() - proj->y_position() - proj->height()/2,
                              0.0);

          if (x1.norm() < 16.0f)
            return Eigen::Quaternionf();

          Eigen::Vector3f x0 (dpos.x() - x1[0], dpos.y() - x1[1], 0.0);

          x1.normalize();
          x0.normalize();

          const Eigen::Vector3f n = x1.cross (x0);
          const float angle = n[2];
          Eigen::Vector3f v = (proj->screen_normal()).normalized();
          return Eigen::Quaternionf (Eigen::AngleAxisf (angle, v));
        }





        void Base::tilt_event ()
        {
          if (window().active_camera_interactor() && window().active_camera_interactor()->tilt_event())
            return;

          if (snap_to_image())
            window().set_snap_to_image (false);

          const Eigen::Quaternionf rot = get_tilt_rotation();
          if (!rot.coeffs().allFinite())
            return;

          Eigen::Quaternionf orient = rot * orientation();
          set_orientation (orient);
          updateGL();
        }





        void Base::rotate_event ()
        {
          if (window().active_camera_interactor() && window().active_camera_interactor()->rotate_event())
            return;

          if (snap_to_image())
            window().set_snap_to_image (false);

          const Eigen::Quaternionf rot = get_rotate_rotation();
          if (!rot.coeffs().allFinite())
            return;

          Eigen::Quaternionf orient = rot * orientation();
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
            float(image()->header().size (0) * image()->header().spacing (0)),
            float(image()->header().size (1) * image()->header().spacing (1)),
            float(image()->header().size (2) * image()->header().spacing (2))
          };
          if (dim[0] < dim[1] && dim[0] < dim[2])
            set_plane (0);
          else if (dim[1] < dim[0] && dim[1] < dim[2])
            set_plane (1);
          else
            set_plane (2);

          Eigen::Vector3f p (
              std::floor ((image()->header().size(0)-1)/2.0f),
              std::floor ((image()->header().size(1)-1)/2.0f),
              std::floor ((image()->header().size(2)-1)/2.0f)
              );

          set_focus (image()->voxel2scanner() * p);
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


