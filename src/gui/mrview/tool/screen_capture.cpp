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
#include <QFileDialog>

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

          QGroupBox* rotate_group_box = new QGroupBox ("Rotate");
          QGridLayout* rotate_layout = new QGridLayout;
          rotate_layout->setContentsMargins (5, 5, 5, 5);
          rotate_layout->setSpacing (5);
          main_box->addWidget (rotate_group_box);
          rotate_group_box->setLayout (rotate_layout);

          rotate_layout->addWidget (new QLabel ("Axis X"), 0, 0);
          rotaton_axis_x = new AdjustButton (this);
          rotate_layout->addWidget (rotaton_axis_x, 0, 1);
          rotaton_axis_x->setValue (0.0);
          rotaton_axis_x->setRate (0.1);

          rotate_layout->addWidget (new QLabel ("Axis Y"), 1, 0);
          rotaton_axis_y = new AdjustButton (this);
          rotate_layout->addWidget (rotaton_axis_y, 1, 1);
          rotaton_axis_y->setValue (0.0);
          rotaton_axis_y->setRate (0.1);

          rotate_layout->addWidget (new QLabel ("Axis Z"), 2, 0);
          rotaton_axis_z = new AdjustButton (this);
          rotate_layout->addWidget (rotaton_axis_z, 2, 1);
          rotaton_axis_z->setValue (1.0);
          rotaton_axis_z->setRate (0.1);

          rotate_layout->addWidget (new QLabel ("Angle"), 3, 0);
          degrees_button = new AdjustButton (this);
          rotate_layout->addWidget (degrees_button, 3, 1);
          degrees_button->setValue (0.0);
          degrees_button->setRate (0.1);


          QGroupBox* output_group_box = new QGroupBox ("Output");
          main_box->addWidget (output_group_box);
          QGridLayout* output_grid_layout = new QGridLayout;
          output_group_box->setLayout (output_grid_layout);


          output_grid_layout->addWidget (new QLabel ("Prefix"), 0, 0);
          prefix_textbox = new QLineEdit ("screenshot", this);
          output_grid_layout->addWidget (prefix_textbox, 0, 1);

          folder_button = new QPushButton ("Select output folder", this);
          folder_button->setToolTip (tr ("Output Folder"));
          connect (folder_button, SIGNAL (clicked()), this, SLOT (select_output_folder_slot ()));
          output_grid_layout->addWidget (folder_button, 1, 0, 1, 2);

          QGroupBox* capture_group_box = new QGroupBox ("Capture");
          main_box->addWidget (capture_group_box);
          QGridLayout* capture_grid_layout = new QGridLayout;
          capture_group_box->setLayout (capture_grid_layout);

          capture_grid_layout->addWidget (new QLabel ("Start Index"), 0, 0);
          start_index = new QSpinBox (this);
          start_index->setMinimum (0);
          start_index->setMaximum (std::numeric_limits<int>::max());
          start_index->setValue (0);
          capture_grid_layout->addWidget (start_index, 0, 1);

          capture_grid_layout->addWidget (new QLabel ("Frames"), 1, 0);
          frames = new QSpinBox (this);
          frames->setMinimum (0);
          frames->setMaximum (std::numeric_limits<int>::max());
          frames->setValue (1);
          capture_grid_layout->addWidget (frames, 1, 1);


          QPushButton* capture = new QPushButton ("Go", this);
          connect (capture, SIGNAL (clicked()), this, SLOT (on_screen_capture()));

          capture_grid_layout->addWidget (capture, 2, 0, 1, 2);

          main_box->addStretch ();

          directory = new QDir();
        }


        void ScreenCapture::on_screen_capture ()
        {
          if (window.snap_to_image () && degrees_button->value() > 0.0)
            window.set_snap_to_image (false);
          float radians = degrees_button->value() * (M_PI / 180.0);
          std::string folder (directory->path().toUtf8().constData());
          std::string prefix (prefix_textbox->text().toUtf8().constData());
          int first_index = start_index->value();
          int i = first_index;
          for (; i < first_index + frames->value(); ++i) {
            this->window.captureGL (folder + "/" + prefix + printf ("%04d.png", i));
            Math::Versor<float> orientation (this->window.orientation());
            Math::Vector<float> axis (3);
            axis[0] = rotaton_axis_x->value();
            axis[1] = rotaton_axis_y->value();
            axis[2] = rotaton_axis_z->value();
            Math::Versor<float> rotation (radians, axis.ptr());
            rotation *= orientation;
            this->window.set_orientation (rotation);
            this->window.updateGL();
            start_index->setValue (i + 1);
          }
        }

        bool ScreenCapture::select_output_folder_slot ()
        {
          directory->setPath(QFileDialog::getExistingDirectory (this, tr("Directory"), directory->path()));
          QString path (shorten(directory->path().toUtf8().constData(), 20, 0).c_str());
          folder_button->setText(path);
          return true;
        }


      }
    }
  }
}






