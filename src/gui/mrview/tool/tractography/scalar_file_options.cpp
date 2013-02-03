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

//#include <QPushButton>
//#include <QLabel>
//#include <QLayout>
//#include <QSlider>
//#include <QStyle>
#include <QGroupBox>
#include <QGridLayout>

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
        namespace Tractography
        {

          ScalarFileOptions::ScalarFileOptions (Window& main_window, Dock* parent) :
            Base (main_window, parent)
          {
            main_box = new QVBoxLayout (this);
            main_box->setContentsMargins (5, 5, 5, 5);
            main_box->setSpacing (5);

            button = new QPushButton (this);
            button->setToolTip (tr ("Open Tracks"));
            connect (button, SIGNAL (clicked()), this, SLOT (open_track_scalar_file_slot ()));
            main_box->addWidget (button);

            QGroupBox* group_box = new QGroupBox ("Scaling");
            QGridLayout* layout = new QGridLayout;
            main_box->addWidget (group_box);
            group_box->setLayout (layout);

            layout->addWidget (new QLabel ("min"), 0, 0);
            min_entry = new AdjustButton (this);
            connect (min_entry, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
            layout->addWidget (min_entry, 0, 1);

            layout->addWidget (new QLabel ("max"), 1, 0);
            max_entry = new AdjustButton (this);
            connect (max_entry, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
            layout->addWidget (max_entry, 1, 1);

            QGroupBox* threshold_box = new QGroupBox ("Thresholds");
            threshold_box->setCheckable (true);
            threshold_box->setChecked (false);
            connect (threshold_box, SIGNAL (toggled(bool)), this, SLOT (on_set_threshold_slot()));
            layout = new QGridLayout;
            main_box->addWidget (threshold_box);
            threshold_box->setLayout (layout);

            layout->addWidget (new QLabel (">"), 0, 0);
            lessthan = new AdjustButton (this);
            connect (lessthan, SIGNAL (valueChanged()), this, SLOT (on_set_threshold_slot()));
            layout->addWidget (lessthan, 0, 1);

            layout->addWidget (new QLabel ("<"), 1, 0);
            greaterthan = new AdjustButton (this);
            connect (greaterthan, SIGNAL (valueChanged()), this, SLOT (on_set_threshold_slot()));
            layout->addWidget (greaterthan, 1, 1);

            main_box->addStretch ();
            setMinimumSize (main_box->minimumSize());
          }

          void ScalarFileOptions::set_tractogram (Tractogram* selected_tractogram) {
            tractogram = selected_tractogram;
            if (tractogram && tractogram->get_colour_type() == ScalarFile) {
              button->setEnabled (true);
              min_entry->setEnabled (true);
              max_entry->setEnabled (true);
              lessthan->setEnabled (true);
              greaterthan->setEnabled (true);
              if (tractogram->get_scalar_filename().length()) {
                button->setText (shorten (tractogram->get_scalar_filename(), 20, 0).c_str());
              } else {
                button->setText ("");
              }
            } else {
              button->setText ("");
              button->setEnabled (false);
              min_entry->setEnabled (false);
              max_entry->setEnabled (false);
              lessthan->setEnabled (false);
              greaterthan->setEnabled (false);
            }
          }

          void ScalarFileOptions::open_track_scalar_file_slot ()
          {
            Dialog::File dialog (this, "Select track scalar to open", false, false);
            if (dialog.exec()) {
              std::vector<std::string> list;
              dialog.get_selection (list);
              CONSOLE(list[0]);
              try {
                if (!tractogram->load_track_scalars (list[0])) {
                  button->setText (shorten (list[0], 20, 0).c_str());
                } else {
                  //            lessthan->setValue (window.image() ? window.image()->intensity_min() : 0.0); TODO
                }
              }
              catch (Exception& E) {
                E.display();
              }
            }

          }

          void ScalarFileOptions::on_set_scaling_slot ()
          {
            if (window.image()) {
              window.image()->set_windowing (min_entry->value(), max_entry->value());
              window.updateGL();
            }
          }

          void ScalarFileOptions::on_set_threshold_slot ()
          {
            TEST;
            if (tractogram) {
              tractogram->set_windowing (min_entry->value(), max_entry->value());
              window.updateGL();
            }
          }



  //        void ScalarFileOptions::on_scaling_changed_slot ()
  //        {
  //          if (window.image()) {
  //            min_entry->setValue (window.image()->scaling_min());
  //            max_entry->setValue (window.image()->scaling_max());
  //            set_scaling_rate();
  //          }
  //        }
        }
      }
    }
  }
}
