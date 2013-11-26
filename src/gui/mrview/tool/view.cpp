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

#include <QLabel>
#include <QGridLayout>

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/view.h"
#include "gui/mrview/adjust_button.h"

#define FOV_RATE_MULTIPLIER 0.01

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        View::View (Window& main_window, Dock* parent) : 
          Base (main_window, parent)
        {
          QVBoxLayout* main_box = new QVBoxLayout (this);

          plane_combobox = new QComboBox;
          plane_combobox->insertItem (0, "Sagittal");
          plane_combobox->insertItem (1, "Coronal");
          plane_combobox->insertItem (2, "Axial");
          main_box->addWidget (plane_combobox);
          connect (plane_combobox, SIGNAL (activated(int)), this, SLOT (onSetPlane(int)));

          QGridLayout* layout = new QGridLayout;
          layout->setContentsMargins (5, 5, 5, 5);
          layout->setSpacing (5);
          main_box->addLayout (layout);
          layout->addWidget (new QLabel ("FOV"), 0, 0);
          fov = new AdjustButton (this);
          connect (fov, SIGNAL (valueChanged()), this, SLOT (onSetFOV()));
          layout->addWidget (fov, 0, 1);

          QGroupBox* group_box = new QGroupBox ("Focus");
          layout = new QGridLayout;
          layout->setContentsMargins (5, 5, 5, 5);
          layout->setSpacing (5);
          main_box->addWidget (group_box);
          group_box->setLayout (layout);

          layout->addWidget (new QLabel ("x"), 0, 0);
          focus_x = new AdjustButton (this);
          connect (focus_x, SIGNAL (valueChanged()), this, SLOT (onSetFocus()));
          layout->addWidget (focus_x, 0, 1);

          layout->addWidget (new QLabel ("y"), 1, 0);
          focus_y = new AdjustButton (this);
          connect (focus_y, SIGNAL (valueChanged()), this, SLOT (onSetFocus()));
          layout->addWidget (focus_y, 1, 1);

          layout->addWidget (new QLabel ("z"), 2, 0);
          focus_z = new AdjustButton (this);
          connect (focus_z, SIGNAL (valueChanged()), this, SLOT (onSetFocus()));
          layout->addWidget (focus_z, 2, 1);



          group_box = new QGroupBox ("Scaling");
          layout = new QGridLayout;
          main_box->addWidget (group_box);
          group_box->setLayout (layout);

          layout->addWidget (new QLabel ("min"), 0, 0);
          min_entry = new AdjustButton (this);
          connect (min_entry, SIGNAL (valueChanged()), this, SLOT (onSetScaling()));
          layout->addWidget (min_entry, 0, 1);

          layout->addWidget (new QLabel ("max"), 1, 0);
          max_entry = new AdjustButton (this);
          connect (max_entry, SIGNAL (valueChanged()), this, SLOT (onSetScaling()));
          layout->addWidget (max_entry, 1, 1);



          transparency_box = new QGroupBox ("Transparency");
          layout = new QGridLayout;
          main_box->addWidget (transparency_box);
          transparency_box->setLayout (layout);

          layout->addWidget (new QLabel ("transparent"), 0, 0);
          transparent_intensity = new AdjustButton (this);
          connect (transparent_intensity, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
          layout->addWidget (transparent_intensity, 0, 1);

          layout->addWidget (new QLabel ("opaque"), 1, 0);
          opaque_intensity = new AdjustButton (this);
          connect (opaque_intensity, SIGNAL (valueChanged()), this, SLOT (onSetTransparency()));
          layout->addWidget (opaque_intensity, 1, 1);

          layout->addWidget (new QLabel ("alpha"), 2, 0);
          opacity = new QSlider (Qt::Horizontal);
          opacity->setRange (0, 255);
          opacity->setValue (255);
          connect (opacity, SIGNAL (valueChanged(int)), this, SLOT (onSetTransparency()));
          layout->addWidget (opacity, 2, 1);


          /*
             threshold_box = new QGroupBox ("Thresholds");
             threshold_box->setCheckable (true);
             threshold_box->setChecked (false);
             connect (threshold_box, SIGNAL (toggled(bool)), this, SLOT (onSetThreshold()));
             layout = new QGridLayout;
             main_box->addWidget (threshold_box);
             threshold_box->setLayout (layout);

             layout->addWidget (new QLabel (">"), 0, 0);
             lessthan = new AdjustButton (this);
             lessthan->setValue (window.image() ? window.image()->intensity_min() : 0.0);
             connect (lessthan, SIGNAL (valueChanged()), this, SLOT (onSetThreshold()));
             layout->addWidget (lessthan, 0, 1);

             layout->addWidget (new QLabel ("<"), 1, 0);
             greaterthan = new AdjustButton (this);
             greaterthan->setValue (window.image() ? window.image()->intensity_max() : 1.0);
             connect (greaterthan, SIGNAL (valueChanged()), this, SLOT (onSetThreshold()));
             layout->addWidget (greaterthan, 1, 1);
           */

          main_box->addStretch ();
          setMinimumSize (main_box->minimumSize());
        }





        void View::showEvent (QShowEvent* event) 
        {
          connect (&window, SIGNAL (imageChanged()), this, SLOT (onImageChanged()));
          connect (&window, SIGNAL (focusChanged()), this, SLOT (onFocusChanged()));
          connect (&window, SIGNAL (planeChanged()), this, SLOT (onPlaneChanged()));
          connect (&window, SIGNAL (scalingChanged()), this, SLOT (onScalingChanged()));
          connect (&window, SIGNAL (modeChanged()), this, SLOT (onModeChanged()));
          connect (&window, SIGNAL (fieldOfViewChanged()), this, SLOT (onFOVChanged()));
          onPlaneChanged();
          onFocusChanged();
          onScalingChanged();
          onModeChanged();
          onImageChanged();
          onFOVChanged();
        }





        void View::closeEvent (QCloseEvent* event) 
        {
          window.disconnect (this);
        }



        void View::onImageChanged () 
        {
          set_scaling_rate();
          set_focus_rate();
          set_transparency_from_image();
        }





        void View::onFocusChanged () 
        {
          focus_x->setValue (window.focus()[0]);
          focus_y->setValue (window.focus()[1]);
          focus_z->setValue (window.focus()[2]);
        }



        void View::onFOVChanged () 
        {
          fov->setValue (window.FOV());
          fov->setRate (FOV_RATE_MULTIPLIER * fov->value());
        }





        void View::onSetFocus () 
        {
          try {
            window.set_focus (Point<> (focus_x->value(), focus_y->value(), focus_z->value()));
            window.updateGL();
          }
          catch (Exception) { }
        }





        void View::onModeChanged () 
        {
          transparency_box->setEnabled (window.get_current_mode()->features & Mode::ShaderTransparency);
        }




        void View::onSetTransparency () 
        {
          assert (window.image()); 
          if (transparency_box->isEnabled()) {
            window.image()->set_transparency (
                transparent_intensity->value(), 
                opaque_intensity->value(),
                float (opacity->value()) / 255.0);
            window.updateGL();
          }
        }





        void View::onPlaneChanged () 
        {
          plane_combobox->setCurrentIndex (window.plane());
        }





        void View::onSetPlane (int index) 
        {
          window.set_plane (index);
          window.updateGL();
        }





        void View::set_scaling_rate () 
        {
          if (!window.image()) return;
          float rate = window.image()->scaling_rate();
          min_entry->setRate (rate);
          max_entry->setRate (rate);
        }





        void View::set_focus_rate () 
        {
          if (!window.image()) return;
          float rate = window.image()->focus_rate();
          focus_x->setRate (rate);
          focus_y->setRate (rate);
          focus_z->setRate (rate);
        }





        void View::set_transparency_from_image () 
        {
          if (!finite (window.image()->transparent_intensity) ||
              !finite (window.image()->opaque_intensity) ||
              !finite (window.image()->alpha) ) { // reset:
            if (!finite (window.image()->intensity_min()) || 
                !finite (window.image()->intensity_max()))
              return;
            window.image()->transparent_intensity = window.image()->intensity_min();
            window.image()->opaque_intensity = window.image()->intensity_max();
            window.image()->alpha = opacity->value() / 255.0;
          }

          assert (finite (window.image()->transparent_intensity));
          assert (finite (window.image()->opaque_intensity));
          assert (finite (window.image()->alpha));

          transparent_intensity->setValue (window.image()->transparent_intensity);
          opaque_intensity->setValue (window.image()->opaque_intensity);
          opacity->setValue (window.image()->alpha * 255.0);
          float rate = window.image() ? window.image()->scaling_rate() : 0.0;
          transparent_intensity->setRate (rate);
          opaque_intensity->setRate (rate);
        }




        void View::onSetScaling ()
        {
          if (window.image()) {
            window.image()->set_windowing (min_entry->value(), max_entry->value());
            window.updateGL();
          }
        }




        void View::onSetFOV ()
        {
          if (window.image()) {
            window.set_FOV (fov->value());
            fov->setRate (FOV_RATE_MULTIPLIER * fov->value());
            window.updateGL();
          }
        }



        void View::onScalingChanged ()
        {
          if (window.image()) {
            min_entry->setValue (window.image()->scaling_min());
            max_entry->setValue (window.image()->scaling_max());
            set_scaling_rate();
            if (transparency_box->isEnabled())
              set_transparency_from_image();
          }
        }

      }
    }
  }
}






