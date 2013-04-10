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
#include "gui/mrview/tool/screencapture.h"
#include "gui/mrview/adjust_button.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        ScreenCapture::ScreenCapture (Window& main_window, Dock* parent) :
          Base (main_window, parent),
          axis (2),
          degrees (1.0)
        {
          QVBoxLayout* main_box = new QVBoxLayout (this);



          QGroupBox* group_box = new QGroupBox ("Rotate");
          QGridLayout* layout = new QGridLayout;
          layout->setContentsMargins (5, 5, 5, 5);
          layout->setSpacing (5);
          main_box->addWidget (group_box);
          group_box->setLayout (layout);

          axis_combobox = new QComboBox;
          axis_combobox->insertItem (0, "X-Axis");
          axis_combobox->insertItem (1, "Y-Axis");
          axis_combobox->insertItem (2, "Z-Axis");
          layout->addWidget (axis_combobox);
          connect (axis_combobox, SIGNAL (activated(int)), this, SLOT (onSetRotationAxis(int)));

//          layout->addWidget (new QLabel ("Degrees"), 0, 0);
//          AdjustButton *degrees_button = new AdjustButton (this);
//          connect (focus_x, SIGNAL (valueChanged()), this, SLOT (onSetRotationDegree(float)));
//          layout->addWidget (focus_x, 0, 1);

//          layout->addWidget (new QLabel ("y"), 1, 0);
//          focus_y = new AdjustButton (this);
//          connect (focus_y, SIGNAL (valueChanged()), this, SLOT (onSetFocus()));
//          layout->addWidget (focus_y, 1, 1);
//
//          layout->addWidget (new QLabel ("z"), 2, 0);
//          focus_z = new AdjustButton (this);
//          connect (focus_z, SIGNAL (valueChanged()), this, SLOT (onSetFocus()));
//          layout->addWidget (focus_z, 2, 1);

          main_box->addStretch ();
        }


        void ScreenCapture::onSetRotationAxis (int the_axis)
        {
          axis = the_axis;
          TEST;
        }

        void ScreenCapture::onScreenCapture ()
        {

        }
        void ScreenCapture::onSetRotationDegree (float degrees){

        }


      }
    }
  }
}






