/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 01/02/13.

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

#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QGroupBox>
#include <QSlider>
#include <QStyle>

#include "gui/mrview/tool/tractography/scalar_file_options.h"
#include "math/vector.h"
#include "gui/color_button.h"
#include "gui/dialog/lighting.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        ScalarFileOptions::ScalarFileOptions (Window& main_window, Dock* parent) :
          Base (main_window, parent)
        {
          QVBoxLayout* main_box = new QVBoxLayout (this);
          main_box->setContentsMargins (0,0,0,0);
          main_box->setSpacing (0);

          button = new QPushButton (this);
          button->setToolTip (tr ("Open Tracks"));
          connect (button, SIGNAL (clicked()), this, SLOT (open_track_scalar_file_slot ()));
          main_box->addWidget (button);
        }

        void ScalarFileOptions::set_tractogram (Tractogram* selected_tractogram) {
          tractogram = selected_tractogram;
          if (tractogram->get_scalar_filename().length()) {
//            button->setText(tractogram->get_scalar_filename().c_str());
          } else {

          }
        }

        void ScalarFileOptions::open_track_scalar_file_slot ()
        {
          Dialog::File dialog (this, "Select track scalar to open", false, false);
          if (dialog.exec()) {
            std::vector<std::string> list;
            dialog.get_selection (list);
            button->setText(list[0].c_str());
            std::cout << tractogram->get_filename() << std::endl;
          }

        }


      }
    }
  }
}
