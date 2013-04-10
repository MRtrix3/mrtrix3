/*
   Copyright 2013 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt 10/04/2013

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
#include <QVBoxLayout>
#include <QPushButton>
#include <QImage>

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/screen_capture.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        ScreenCapture::ScreenCapture (Window& main_window, Dock* parent) :
          Base (main_window, parent)
        {
          QVBoxLayout* main_box = new QVBoxLayout (this);

          QGroupBox* group_box = new QGroupBox ("Rotate");
          QGridLayout* layout = new QGridLayout;
          layout->setContentsMargins (5, 5, 5, 5);
          layout->setSpacing (5);
          main_box->addWidget (group_box);
          group_box->setLayout (layout);

          layout->addWidget (new QLabel ("Axis X"), 0, 0);
          rotaton_axis_x = new AdjustButton (this);
          layout->addWidget (rotaton_axis_x, 0, 1);
          rotaton_axis_x->setValue(0.0);
          rotaton_axis_x->setRate(0.1);

          layout->addWidget (new QLabel ("Axis Y"), 1, 0);
          rotaton_axis_y = new AdjustButton (this);
          layout->addWidget (rotaton_axis_y, 1, 1);
          rotaton_axis_y->setValue(0.0);
          rotaton_axis_y->setRate(0.1);

          layout->addWidget (new QLabel ("Axis Z"), 2, 0);
          rotaton_axis_z = new AdjustButton (this);
          layout->addWidget (rotaton_axis_z, 2, 1);
          rotaton_axis_z->setValue(1.0);
          rotaton_axis_z->setRate(0.1);

          layout->addWidget (new QLabel ("Angle"), 3, 0);
          degrees_button = new AdjustButton (this);
          layout->addWidget (degrees_button, 3, 1);
          degrees_button->setValue(1.0);
          degrees_button->setRate(0.1);

          layout->addWidget (new QLabel ("Number"), 4, 0);
          num_captures = new QSpinBox (this);
          num_captures->setMinimum(0);
          num_captures->setMaximum(1000);
          num_captures->setValue(1);
          layout->addWidget (num_captures, 4, 1);

          layout->addWidget (new QLabel ("Prefix"), 5, 0);
          prefix_textbox = new QLineEdit (this);
          layout->addWidget (prefix_textbox, 5, 1);

          QPushButton* capture = new QPushButton ("Capture", this);
          connect (capture, SIGNAL (clicked()), this, SLOT (onScreenCapture()));

          QVBoxLayout* vlayout = new QVBoxLayout;
          vlayout->setContentsMargins (5, 5, 5, 5);
          vlayout->setSpacing (5);
          main_box->addLayout(vlayout);
          vlayout->addWidget (capture);

          main_box->addStretch ();
        }


        void ScreenCapture::onScreenCapture ()
        {
          float radians = degrees_button->value() * (M_PI / 180.0);
          std::string prefix (prefix_textbox->text().toUtf8().constData());
          for (int i = 0; i < num_captures->value(); ++i) {
            Math::Versor<float> orientation (this->window.orientation());
            Math::Vector<float> axis (3);
            axis[0] = rotaton_axis_x->value();
            axis[1] = rotaton_axis_y->value();
            axis[2] = rotaton_axis_z->value();
            Math::Versor<float> rotation (radians, axis.ptr());
            orientation *= rotation;
            this->window.set_orientation (orientation);
            this->window.updateGL();
            std::string filename = prefix + printf("%04d.png", i);
            this->window.captureGL (filename);
          }
        }


      }
    }
  }
}






