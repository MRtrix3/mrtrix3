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


#include "gui/mrview/tool/transform.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Transform::Transform (Dock* parent) :
          Base (parent)
        {
          VBoxLayout* main_box = new VBoxLayout (this);
          QLabel* label = new QLabel (
              "<b><font color='red'>The transform tool is currently active</font><b><br><br>"
              "Close this tool to deactivate.<br><hr>"
              "All camera view manipulations will now apply "
              "to the main image, rather than to the camera");
          label->setWordWrap (true);
          label->setAlignment (Qt::AlignHCenter);

          main_box->addWidget (label, 0);

          main_box->addStretch ();
        }






        void Transform::showEvent (QShowEvent*)
        {
          if (isVisible())
            window().register_camera_interactor (this);
        }





        void Transform::closeEvent (QCloseEvent*)
        {
          if (window().active_camera_interactor() == this)
            window().register_camera_interactor();
        }




        void Transform::hideEvent (QHideEvent*)
        {
          if (window().active_camera_interactor() == this)
            window().register_camera_interactor();
        }







        bool Transform::slice_move_event (float x)
        {
          const Projection* proj = window().get_current_mode()->get_current_projection();
          if (!proj)
            return true;

          const auto &header = window().image()->header();
          float increment = window().snap_to_image() ?
            x * header.spacing (window().plane()) :
            x * std::pow (header.spacing(0) * header.spacing(1) * header.spacing(2), 1.0f/3.0f);
          auto move = window().get_current_mode()->get_through_plane_translation (increment, *proj);

          transform_type M = header.transform();
          M.translate (-move.cast<double>());

          window().image()->header().transform() = M;
          window().image()->image.buffer->transform() = M;
          window().updateGL();
          return true;
        }




        bool Transform::pan_event ()
        {
          const Projection* proj = window().get_current_mode()->get_current_projection();
          if (!proj)
            return true;

          auto move = proj->screen_to_model_direction (window().mouse_displacement(), window().target());

          transform_type M = window().image()->header().transform();
          M.pretranslate (move.cast<double>());

          window().image()->header().transform() = M;
          window().image()->image.buffer->transform() = M;
          window().updateGL();

          return true;
        }



        bool Transform::panthrough_event ()
        {
          const Projection* proj = window().get_current_mode()->get_current_projection();
          if (!proj)
            return true;

          auto move = window().get_current_mode()->get_through_plane_translation_FOV (window().mouse_displacement().y(), *proj);

          transform_type M = window().image()->header().transform();
          M.pretranslate (-move.cast<double>());

          window().image()->header().transform() = M;
          window().image()->image.buffer->transform() = M;
          window().updateGL();

          return true;
        }





        bool Transform::tilt_event ()
        {
          if (window().snap_to_image())
            window().set_snap_to_image (false);

          const auto rot = window().get_current_mode()->get_tilt_rotation().cast<double>();
          if (!rot.coeffs().allFinite())
            return true;

          const Eigen::Vector3d origin = window().focus().cast<double>();
          transform_type M = transform_type (rot).pretranslate (origin).translate (-origin) * window().image()->header().transform();

          window().image()->header().transform() = M;
          window().image()->image.buffer->transform() = M;
          window().updateGL();

          return true;
        }





        bool Transform::rotate_event ()
        {
          if (window().snap_to_image())
            window().set_snap_to_image (false);

          const auto rot = window().get_current_mode()->get_rotate_rotation().cast<double>();
          if (!rot.coeffs().allFinite())
            return true;

          const Eigen::Vector3d origin = window().target().cast<double>();
          transform_type M = transform_type (rot).inverse().pretranslate (origin).translate (-origin) * window().image()->header().transform();

          window().image()->header().transform() = M;
          window().image()->image.buffer->transform() = M;
          window().updateGL();

          return true;
        }




      }
    }
  }
}






