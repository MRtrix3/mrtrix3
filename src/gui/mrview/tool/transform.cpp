/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
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
              "When active, all camera view manipulations will apply "
              "to the main image, rather than to the camera");
          label->setWordWrap (true);
          label->setAlignment (Qt::AlignHCenter);
          main_box->addWidget (label);

          activate_button = new QPushButton ("Activate",this);
          activate_button->setToolTip (tr ("Activate transform manipulation mode"));
          activate_button->setIcon (QIcon (":/rotate.svg"));
          activate_button->setCheckable (true);
          connect (activate_button, SIGNAL (clicked(bool)), this, SLOT (onActivate (bool)));
          main_box->addWidget (activate_button);

          main_box->addStretch ();
          show();
        }



        void Transform::setActive (bool onoff)
        {
          activate_button->setChecked (onoff);
          if (isVisible()) {
            if (onoff)
              activate_button->setText (tr ("on"));
            else
              activate_button->setText (tr ("off"));
          }
          window().register_camera_interactor ( (isVisible() && onoff) ? this : nullptr );
        }


        void Transform::onActivate (bool onoff)
        {
          setActive (onoff);
        }


        void Transform::showEvent (QShowEvent*)
        {
          setActive (false);
        }





        void Transform::closeEvent (QCloseEvent*)
        {
          setActive (false);
        }




        void Transform::hideEvent (QHideEvent*)
        {
          setActive (false);
        }







        bool Transform::slice_move_event (const ModelViewProjection& projection, float x)
        {
          const auto &header = window().image()->header();
          float increment = window().snap_to_image() ?
            x * header.spacing (window().plane()) :
            x * std::pow (header.spacing(0) * header.spacing(1) * header.spacing(2), 1.0f/3.0f);
          auto move = window().get_current_mode()->get_through_plane_translation (increment, projection);

          transform_type M = header.transform();
          M.translate (-move.cast<double>());

          window().image()->header().transform() = M;
          window().image()->image.buffer->transform() = M;
          window().updateGL();
          return true;
        }




        bool Transform::pan_event (const ModelViewProjection& projection)
        {
          auto move = projection.screen_to_model_direction (window().mouse_displacement(), window().target());

          transform_type M = window().image()->header().transform();
          M.pretranslate (move.cast<double>());

          window().image()->header().transform() = M;
          window().image()->image.buffer->transform() = M;
          window().updateGL();

          return true;
        }



        bool Transform::panthrough_event (const ModelViewProjection& projection)
        {
          auto move = window().get_current_mode()->get_through_plane_translation_FOV (window().mouse_displacement().y(), projection);

          transform_type M = window().image()->header().transform();
          M.pretranslate (-move.cast<double>());

          window().image()->header().transform() = M;
          window().image()->image.buffer->transform() = M;
          window().updateGL();

          return true;
        }





        bool Transform::tilt_event (const ModelViewProjection& projection)
        {
          if (window().snap_to_image())
            window().set_snap_to_image (false);

          const auto rot = window().get_current_mode()->get_tilt_rotation (projection).cast<double>().inverse();
          if (!rot.coeffs().allFinite())
            return true;

          const Eigen::Vector3d origin = window().focus().cast<double>();
          transform_type M = transform_type (rot).pretranslate (origin).translate (-origin) * window().image()->header().transform();

          window().image()->header().transform() = M;
          window().image()->image.buffer->transform() = M;
          window().updateGL();

          return true;
        }





        bool Transform::rotate_event (const ModelViewProjection& projection)
        {
          if (window().snap_to_image())
            window().set_snap_to_image (false);

          const auto rot = window().get_current_mode()->get_rotate_rotation (projection).cast<double>();
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






